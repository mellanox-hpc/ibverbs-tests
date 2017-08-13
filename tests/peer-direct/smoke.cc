/**
 * Copyright (C) 2016	   Mellanox Technologies Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <infiniband/verbs.h>

#include "env.h"

#include <infiniband/peer_ops.h>
#include <list>

#define SZ 1024
#define MAX_WR 6

struct ibvt_peer: public ibvt_obj {
	struct ibv_peer_direct_attr attr;
	ibvt_pd &pd;

	struct peer_mr {
		ibvt_peer &ctx;
		struct ibv_mr *region;
		uintptr_t start;
		uintptr_t end;

		peer_mr(ibvt_peer &c, void* base, size_t length) : ctx(c) {
			start = (uintptr_t)base;
			end = start+length-1;
		}
	};

	struct queue_buf {
		struct ibv_peer_buf pb;
		ibvt_peer * ctx;
		size_t length;
	};

	std::list<struct peer_mr *> mr_list;
	std::set<struct queue_buf *> wq_set;

	ibvt_peer(ibvt_env &e, ibvt_pd &p) : ibvt_obj(e), pd(p)
	{
		attr.peer_id = (uint64_t)this;
		attr.register_va = register_va;
		attr.unregister_va = unregister_va;
		attr.buf_alloc = buf_alloc;
		attr.buf_release = buf_release;
		attr.caps = IBV_PEER_OP_FENCE_CAP |
			IBV_PEER_OP_STORE_DWORD_CAP | IBV_PEER_OP_STORE_QWORD_CAP |
			IBV_PEER_OP_POLL_AND_DWORD_CAP |
			IBV_PEER_OP_POLL_NOR_DWORD_CAP;
			//IBV_PEER_OP_POLL_GEQ_DWORD_CAP;
		attr.comp_mask = IBV_EXP_PEER_DIRECT_VERSION;
		attr.version = 1;
	}

	virtual ~ibvt_peer()
	{
		EXPECT_EQ(0U, wq_set.size());
		EXPECT_EQ(0U, mr_list.size());
	}

	virtual void init() {}

	static struct ibv_peer_buf * buf_alloc(struct ibv_peer_buf_alloc_attr *attr) {
		ibvt_peer *ctx = (ibvt_peer *)attr->peer_id;
		struct queue_buf *qb = new queue_buf;
		if (!qb) {
			EXPECT_TRUE(false) << "struct alloc failed";
			ctx->env.fatality = true;
			return NULL;
		}
		qb->pb.addr = mmap(NULL, attr->length, PROT_READ|PROT_WRITE,
				   MAP_SHARED|MAP_ANONYMOUS, -1, 0);
		if (qb->pb.addr == MAP_FAILED) {
			VERBS_INFO("mmap failed errno %d\n", errno);
			ctx->env.fatality = true;
			delete qb;
			return NULL;
		}
		qb->pb.length = attr->length;
		qb->pb.comp_mask = 0;
		qb->length = attr->length;
		qb->ctx = ctx;
		VERBS_TRACE("buf_alloc %p[%lx]\n", qb->pb.addr, qb->length);
		ctx->wq_set.insert(qb);
		return &qb->pb;
	}

	static int buf_release(struct ibv_peer_buf *pb) {
		struct queue_buf *qb = (struct queue_buf*)pb;
		ibvt_peer *ctx = qb->ctx;
		VERBS_TRACE("buf_release %p[%lx]\n", qb->pb.addr, qb->length);
		munmap(qb->pb.addr, qb->length);
		if (ctx->wq_set.erase(qb) != 1) {
			VERBS_TRACE("unexpected qb\n");
			ctx->env.fatality = true;
		}
		delete qb;
		return 1;
	}

	static uint64_t register_va(void *start, size_t length, uint64_t peer_id,
				    struct ibv_peer_buf *pb)
	{
		ibvt_peer *ctx = (ibvt_peer *)peer_id;
		peer_mr *reg_h = new peer_mr(*ctx, start, length);

		reg_h->region = ibv_reg_mr(ctx->pd.pd, start, length,
					IBV_ACCESS_LOCAL_WRITE |
					IBV_ACCESS_REMOTE_READ |
					IBV_ACCESS_REMOTE_WRITE);
		VERBS_TRACE("register_va %p [%lx] %p\n", start, length, reg_h->region);
		if (!reg_h->region) {
			VERBS_TRACE("ibv_reg_mr on peer memory failed\n");
			reg_h->ctx.env.fatality = true;
			return 0;
		}

		reg_h->ctx.mr_list.push_back(reg_h);

		return (uint64_t)reg_h;
	}

	static int unregister_va(uint64_t registration_id, uint64_t peer_id) {
		peer_mr *reg_h = (peer_mr *)registration_id;
		VERBS_TRACE("unregister_va %p\n", reg_h->region);
		if(ibv_dereg_mr(reg_h->region)) {
			VERBS_TRACE("ibv_dereg_mr on peer memory failed\n");
			reg_h->ctx.env.fatality = true;
		}
		reg_h->ctx.mr_list.remove(reg_h);
		delete reg_h;
		return 1;
	}
};

struct ibvt_peer_op : public ibvt_obj {
	struct {
		struct ibv_send_wr wr1[MAX_WR], wr2[MAX_WR];
		struct ibv_sge sge1[MAX_WR], sge2[MAX_WR];
		int peek_op_type;
		uint32_t peek_op_data;
		uint64_t peek_id;
		uint64_t rollback_id;
		struct {
			struct {
				uint32_t db_record;
				uint64_t db_ring;
			} commit;
			struct {
				uint32_t owner;
				uint32_t cons_index;
				uint32_t db_record;
			} peek;
		} ctrl;
	} ctx;

	ibvt_peer &peer;
	ibvt_pd &pd;
	ibvt_qp &qp;
	ibvt_cq &cq;
	ibvt_pd &pd_peer;
	ibvt_qp &qp_peer;
	ibvt_cq &cq_peer;

	ibv_mr *ctrl_mr;

	int id;

	ibvt_peer_op( ibvt_env &e,
		      ibvt_peer &pe,
		      ibvt_pd &p,
		      ibvt_qp &q,
		      ibvt_cq &c,
		      ibvt_pd &p2,
		      ibvt_qp &q2,
		      ibvt_cq &c2,
		      int i) :
		ibvt_obj(e),
		peer(pe),
		pd(p),
		qp(q),
		cq(c),
		pd_peer(p2),
		qp_peer(q2),
		cq_peer(c2),
		ctrl_mr(NULL),
		id(i)
	{
		memset(&ctx, 0, sizeof(ctx));
	}

	virtual ~ibvt_peer_op() {
		FREE(ibv_dereg_mr, ctrl_mr);
	}

	void ops2wr(struct peer_op_wr *op, int entries, void* buff,
			struct ibv_send_wr *wr, struct ibv_sge *sge) {
		char *ptr = (char *)buff;
		int i = 0;
		ibvt_peer::peer_mr *reg_h;

		memset(wr, 0, MAX_WR*sizeof(*wr));
		memset(sge, 0, MAX_WR*sizeof(*sge));
		for(; op; op = op->next) {
			wr[i]._wr_opcode = IBV_WR_RDMA_WRITE;
			if (op->type == IBV_PEER_OP_STORE_DWORD ||
					op->type == IBV_PEER_OP_POLL_AND_DWORD ||
					op->type == IBV_PEER_OP_POLL_NOR_DWORD ||
					op->type == IBV_PEER_OP_POLL_GEQ_DWORD) {
				reg_h = (ibvt_peer::peer_mr *)op->wr.dword_va.target_id;
				sge[i].addr = (uintptr_t)ptr;
				wr[i].wr.rdma.rkey = reg_h->region->rkey;
				wr[i].wr.rdma.remote_addr = reg_h->start + op->wr.dword_va.offset;
				sge[i].length = 4;
				if (op->type != IBV_PEER_OP_STORE_DWORD)
					wr[i]._wr_opcode = IBV_WR_RDMA_READ;
				else
					memcpy(ptr, &op->wr.dword_va.data, 4);
				ptr += 4;
			} else if (op->type == IBV_PEER_OP_STORE_QWORD) {
				reg_h = (ibvt_peer::peer_mr *)op->wr.qword_va.target_id;
				sge[i].addr = (uintptr_t)ptr;
				wr[i].wr.rdma.rkey = reg_h->region->rkey;
				wr[i].wr.rdma.remote_addr = reg_h->start + op->wr.qword_va.offset;
				sge[i].length = 8;
				memcpy(ptr, &op->wr.qword_va.data, 8);
				ptr += 8;
			} else if (op->type == IBV_PEER_OP_FENCE) {
				continue;
			} else {
				FAIL() << "unknown type: " << op->type;
			}

			ASSERT_TRUE(wr[i].wr.rdma.rkey);
			sge[i].lkey = ctrl_mr->lkey;
			wr[i].sg_list = &sge[i];
			wr[i].num_sge = 1;
			if (i && wr[i-1]._wr_opcode != IBV_WR_RDMA_READ)
				wr[i-1].next = &wr[i];
			wr[i].next = NULL;
			i++;
		}
	}

	virtual void init() {}

	void xmit_peer() {
		EXEC(peer_prep(1));
		EXEC(peer_exec());
		EXEC(peer_poll());
	}

	void peer_prep(int offset) {
		int n = MAX_WR;
		struct peer_op_wr op_buff[2][MAX_WR];
		struct ibv_peer_commit commit_ops;
		commit_ops.storage = op_buff[0];
		commit_ops.entries = MAX_WR;
		struct ibv_peer_peek peek_ops;
		peek_ops.storage = op_buff[1];
		peek_ops.entries = MAX_WR;
		peek_ops.whence = IBV_EXP_PEER_PEEK_RELATIVE;
		peek_ops.offset = offset;

		SET(ctrl_mr, ibv_reg_mr(pd_peer.pd, &ctx.ctrl, sizeof(ctx.ctrl),
					IBV_ACCESS_LOCAL_WRITE |
					IBV_ACCESS_REMOTE_READ |
					IBV_ACCESS_REMOTE_WRITE));

		memset(&op_buff, 0, sizeof(op_buff));
		while(--n) {
			op_buff[0][n-1].next = &op_buff[0][n];
			op_buff[1][n-1].next = &op_buff[1][n];
		}

		DO(ibv_peer_commit_qp(qp.qp, &commit_ops));
		VERBS_INFO("Op%d got %d descriptors to commit send\n", id, commit_ops.entries);
		DO(ibv_peer_peek_cq(cq.cq, &peek_ops));
		VERBS_INFO("Op%d got %d descriptors to peek send result\n", id, peek_ops.entries);
		EXEC(ops2wr(commit_ops.storage, commit_ops.entries, &ctx.ctrl.commit, ctx.wr1, ctx.sge1));
		EXEC(ops2wr(peek_ops.storage, peek_ops.entries, &ctx.ctrl.peek, ctx.wr2, ctx.sge2));
		ctx.rollback_id = commit_ops.rollback_id;
		ctx.peek_op_type = peek_ops.storage->type;
		ctx.peek_op_data = peek_ops.storage->wr.dword_va.data;
		ctx.peek_id = peek_ops.peek_id;
	}

	void peer_exec() {
		struct ibv_send_wr *bad_wr = NULL;
		long retries = POLL_RETRIES;

		ctx.ctrl.peek.owner = ~ctx.peek_op_data;

		DO(ibv_post_send(qp_peer.qp, ctx.wr1, &bad_wr));
		EXEC(cq_peer.poll(2));
		VERBS_INFO("Op%d executed commit descriptors\n", id);

		while(--retries) {
			DO(ibv_post_send(qp_peer.qp, ctx.wr2, &bad_wr));
			EXEC(cq_peer.poll(1));
			if (ctx.peek_op_type == IBV_PEER_OP_POLL_AND_DWORD) {
				if (ctx.ctrl.peek.owner & ctx.peek_op_data)
					break;
			} else if (ctx.peek_op_type == IBV_PEER_OP_POLL_NOR_DWORD) {
				if (~(ctx.ctrl.peek.owner | ctx.peek_op_data))
					break;
			} else if (ctx.peek_op_type == IBV_PEER_OP_POLL_GEQ_DWORD) {
				if ((int32_t)ctx.ctrl.peek.owner >= (int32_t)ctx.peek_op_data)
					break;
			} else {
				FAIL() << "unknown type: " << ctx.peek_op_type;
			}
		}
		ASSERT_TRUE(retries);
	}

	void peer_poll() {
		struct ibv_send_wr *bad_wr = NULL;
		DO(ibv_post_send(qp_peer.qp, ctx.wr2+1, &bad_wr));
		EXEC(cq_peer.poll(1));
		EXEC(cq.poll(1));
		VERBS_INFO("Op%d completed polling\n", id);
	}

	void peer_abort() {
		struct ibv_peer_abort_peek abort_ops;
		abort_ops.peek_id = ctx.peek_id;
		ibv_peer_abort_peek_cq(cq.cq, &abort_ops);
		EXEC(cq.poll(1));
		VERBS_INFO("Op%d aborted polling\n", id);
	}

	void peer_fail(int flags) {
		struct ibv_rollback_ctx rb_ctx;
		rb_ctx.rollback_id = ctx.rollback_id;
		rb_ctx.flags = flags;
		DO(ibv_rollback_qp(qp.qp, &rb_ctx));

		struct ibv_peer_abort_peek abort_ops;
		abort_ops.peek_id = ctx.peek_id;
		ibv_peer_abort_peek_cq(cq.cq, &abort_ops);
		VERBS_INFO("Op%d aborted commit\n", id);
	}
};

template <typename T1, typename T2, typename T3>
struct types {
	typedef T1 QP;
	typedef T2 CQ;
	typedef T3 MR;
};

template <typename T>
struct peerdirect_test : public testing::Test, public ibvt_env {
	struct qp_peerdirect : public T::QP {
		ibvt_peer &peer;

		qp_peerdirect(ibvt_env &e, ibvt_pd &p, ibvt_cq &c, ibvt_peer &pe) :
			T::QP(e, p, c), peer(pe) {}

		virtual void init_attr(struct ibv_qp_init_attr_ex &attr) {
			T::QP::init_attr(attr);
			attr.comp_mask |= IBV_QP_INIT_ATTR_PEER_DIRECT;
			attr.peer_direct_attrs = &peer.attr;
		}
	};

	struct cq_peerdirect : public T::CQ {
		ibvt_peer &peer;

		cq_peerdirect(ibvt_env &e, ibvt_ctx &c, ibvt_peer &pe) :
			T::CQ(e, c), peer(pe) {}

		virtual void init_attr(struct ibv_create_cq_attr_ex &attr,
				       int &cqe) {
			T::CQ::init_attr(attr, cqe);
			attr.comp_mask = IBV_CREATE_CQ_ATTR_PEER_DIRECT;
			attr.peer_direct_attrs = &peer.attr;
			cqe = 20;
		}
	};

	struct ibvt_ctx ctx;
	struct ibvt_ctx ctx_peer;
	struct ibvt_pd pd;
	struct ibvt_pd pd_peer;
	struct ibvt_peer peer;
	struct cq_peerdirect cq;
	struct ibvt_cq cq_peer;
	struct qp_peerdirect send_qp;
	struct qp_peerdirect recv_qp;
	struct T::MR src_mr;
	struct T::MR dst_mr;
	struct ibvt_qp_rc qp_peer;

	peerdirect_test() :
		ctx(*this, &ctx_peer),
		ctx_peer(*this, NULL),
		pd(*this, ctx),
		pd_peer(*this, ctx_peer),
		peer(*this, pd_peer),
		cq(*this, ctx, peer),
		cq_peer(*this, ctx_peer),
		send_qp(*this, pd, cq, peer),
		recv_qp(*this, pd, cq, peer),
		src_mr(*this, pd, SZ),
		dst_mr(*this, pd, SZ),
		qp_peer(*this, pd_peer, cq_peer)
	{
		flags |= ACTIVE;
	}

	~peerdirect_test() {
		for (std::vector<ibvt_peer_op *>::iterator it = ops.begin();
							   it != ops.end();
							   it++)
				delete *it;
	}

	std::vector<struct ibvt_peer_op *> ops;

	ibvt_peer_op &op(size_t i)
	{
		while (i >= ops.size())
			ops.push_back(new struct ibvt_peer_op(*this,
							      this->peer,
							      this->pd,
							      this->send_qp,
							      this->cq,
							      this->pd_peer,
							      this->qp_peer,
							      this->cq_peer,
							      ops.size()));
		return *(ops[i]);
	}

	void send(intptr_t start, size_t length) {
		VERBS_INFO("Preparing send operation %ld-%ld\n",
			   start, start + length);
		EXEC(send_qp.send(src_mr.sge(start, length)));
	}

	void recv(intptr_t start, size_t length) {
		EXEC(recv_qp.recv(dst_mr.sge(start, length)));
	}

	void send_peer_prep(size_t n) {
		for(size_t i = 0; i < n; i++) {
			EXEC(send(SZ/n*i, SZ/n));
			EXEC(op(i).peer_prep(i+1));
			EXEC(recv(SZ/n*i, SZ/n));
		}
	}

	virtual void SetUp() {
		INIT(ctx_peer.init());
		INIT(qp_peer.init());
		INIT(qp_peer.connect(&qp_peer));
		INIT(ctx.init());
		INIT(send_qp.init());
		INIT(recv_qp.init());
		INIT(send_qp.connect(&recv_qp));
		INIT(recv_qp.connect(&send_qp));
		INIT(src_mr.fill());
		INIT(dst_mr.init());
		INIT(cq.arm());
	}

	virtual void TearDown() {
		if (skip)
			return;
		ASSERT_FALSE(HasFailure());
		EXEC(dst_mr.check());
	}
};


typedef testing::Types<
	types<ibvt_qp_rc, ibvt_cq, ibvt_mr>,
	types<ibvt_qp_rc, ibvt_cq_event, ibvt_mr>,
	types<ibvt_qp_ud, ibvt_cq, ibvt_mr_ud>,
	types<ibvt_qp_ud, ibvt_cq_event, ibvt_mr_ud>
> ibvt_env_list;

TYPED_TEST_CASE(peerdirect_test, ibvt_env_list);

TYPED_TEST(peerdirect_test, t1_1_send) {
	CHK_SUT(peer-direct);
	EXEC(recv(0, SZ));
	EXEC(send(0, SZ));
	EXEC(op(0).xmit_peer());
}

TYPED_TEST(peerdirect_test, t2_2_sends) {
	CHK_SUT(peer-direct);

	EXEC(recv(0, SZ/2));
	EXEC(recv(SZ/2, SZ/2));
	EXEC(send(0, SZ/2));
	EXEC(send(SZ/2, SZ/2));
	EXEC(op(0).peer_prep(2));
	EXEC(op(0).peer_exec());
	EXEC(op(0).peer_poll());
}

TYPED_TEST(peerdirect_test, t3_2_sends_2_pd) {
	CHK_SUT(peer-direct);
	EXEC(send_peer_prep(2));
	EXEC(op(0).peer_exec());
	EXEC(op(0).peer_poll());
	EXEC(op(1).peer_exec());
	EXEC(op(1).peer_poll());
}

TYPED_TEST(peerdirect_test, t4_rollback) {
	CHK_SUT(peer-direct);
	EXEC(send_peer_prep(2));

	EXEC(op(0).peer_exec());
	EXEC(op(0).peer_poll());
	EXEC(op(1).peer_fail(0));

	EXEC(send(SZ/2, SZ/2));
	EXEC(op(2).xmit_peer());
}

TYPED_TEST(peerdirect_test, t5_poll_abort) {
	CHK_SUT(peer-direct);
	EXEC(send_peer_prep(4));

	EXEC(op(0).peer_exec());
	EXEC(op(0).peer_poll());
	EXEC(op(1).peer_exec());
	EXEC(op(1).peer_abort());
	EXEC(op(2).peer_exec());
	EXEC(op(2).peer_poll());
	EXEC(op(3).peer_exec());
	EXEC(op(3).peer_abort());
}

TYPED_TEST(peerdirect_test, t6_pool_abort_16a) {
	CHK_SUT(peer-direct);
	EXEC(send_peer_prep(16));

	for (int i = 0; i < 16; i+=2) {
		EXEC(op(i).peer_exec());
		EXEC(op(i).peer_poll());
		EXEC(op(i+1).peer_exec());
		EXEC(op(i+1).peer_abort());
	}
}

TYPED_TEST(peerdirect_test, t7_pool_abort_16b) {
	CHK_SUT(peer-direct);
	EXEC(send_peer_prep(16));

	for (int i = 0; i < 16; i+=2) {
		EXEC(op(i).peer_exec());
		EXEC(op(i).peer_abort());
		EXEC(op(i+1).peer_exec());
		EXEC(op(i+1).peer_poll());
	}
}

TYPED_TEST(peerdirect_test, t8_pool16) {
	CHK_SUT(peer-direct);
	EXEC(send_peer_prep(16));

	for (int i = 0; i < 16; i++) {
		EXEC(op(i).peer_exec());
		EXEC(op(i).peer_poll());
	}
}

TYPED_TEST(peerdirect_test, t9_abort16) {
	CHK_SUT(peer-direct);
	EXEC(send_peer_prep(16));

	for (int i = 0; i < 16; i++) {
		EXEC(op(i).peer_exec());
		EXEC(op(i).peer_abort());
	}
}


