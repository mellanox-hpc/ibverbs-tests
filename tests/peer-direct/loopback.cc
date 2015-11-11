/**
 * Copyright (C) 2015      Mellanox Technologies Ltd. All rights reserved.
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

#ifdef PEER_DIRECT_EXP
#include <infiniband/verbs_exp.h>

#define ibv_create_qp_ex                 ibv_exp_create_qp

#define ibv_peer_commit              	 ibv_exp_peer_commit
#define ibv_peer_commit_qp               ibv_exp_peer_commit_qp

#define ibv_qp_init_attr_ex              ibv_exp_qp_init_attr
#define ibv_create_cq_attr_ex            ibv_exp_cq_init_attr

#define IBV_QP_INIT_ATTR_PEER_DIRECT     IBV_EXP_QP_INIT_ATTR_PEER_DIRECT_SYNC
#define IBV_QP_INIT_ATTR_PD              IBV_EXP_QP_INIT_ATTR_PD
#define IBV_CREATE_CQ_ATTR_PEER_DIRECT   IBV_EXP_CQ_INIT_ATTR_PEER_DIRECT_SYNC

#define IBV_PEER_OP_FENCE                IBV_EXP_PEER_OP_FENCE
#define IBV_PEER_OP_STORE_DWORD          IBV_EXP_PEER_OP_STORE_DWORD
#define IBV_PEER_OP_STORE_QWORD          IBV_EXP_PEER_OP_STORE_QWORD
#define IBV_PEER_OP_COPY_BLOCK           IBV_EXP_PEER_OP_COPY_BLOCK
#define IBV_PEER_OP_POLL_EQ_DWORD        IBV_EXP_PEER_OP_POLL_EQ_DWORD

#define IBV_PEER_OP_POLL_GEQ_DWORD       IBV_EXP_PEER_OP_POLL_GEQ_DWORD
#define IBV_PEER_OP_POLL_AND_DWORD       IBV_EXP_PEER_OP_POLL_AND_DWORD
#define IBV_PEER_OP_POLL_NOR_DWORD       IBV_EXP_PEER_OP_POLL_NOR_DWORD

#define IBV_PEER_FENCE_CPU_TO_HCA        IBV_EXP_PEER_FENCE_CPU_TO_HCA
#define IBV_PEER_FENCE_PEER_TO_HCA       IBV_EXP_PEER_FENCE_PEER_TO_HCA
#define IBV_PEER_FENCE_PEER_TO_CPU       IBV_EXP_PEER_FENCE_PEER_TO_CPU
#define IBV_PEER_FENCE_HCA_TO_PEER       IBV_EXP_PEER_FENCE_HCA_TO_PEER

#define ibv_peer_direct_attr             ibv_exp_peer_direct_attr
#define ibv_peer_direction               ibv_exp_peer_direction
#define ibv_peer_op                      ibv_exp_peer_op

#define IBV_ROLLBACK_ABORT_UNCOMMITED    IBV_EXP_ROLLBACK_ABORT_UNCOMMITED
#define IBV_ROLLBACK_ABORT_LATE          IBV_EXP_ROLLBACK_ABORT_LATE

#define ibv_rollback_ctx                 ibv_exp_rollback_ctx
#define ibv_rollback_qp                  ibv_exp_rollback_qp
#define ibv_peer_peek                    ibv_exp_peer_peek
#define ibv_peer_peek_cq                 ibv_exp_peer_peek_cq
#define ibv_peer_ack_peek                ibv_exp_peer_ack_peek
#define ibv_peer_ack_peek_cq             ibv_exp_peer_ack_peek_cq

#define IBV_PEER_DIRECTION_FROM_CPU	 IBV_EXP_PEER_DIRECTION_FROM_CPU
#define IBV_PEER_DIRECTION_FROM_HCA	 IBV_EXP_PEER_DIRECTION_FROM_HCA

#endif

#include <infiniband/peer_ops.h>
#include <infiniband/arch.h>

#include <list>

#include "common.h"

#define DO(x) do { \
		VERBS_TRACE("%d.%d: doing " #x "\n", __LINE__, i); \
		ASSERT_EQ(x, 0); \
	} while(0)

#define SET(x,y) do { \
		VERBS_TRACE("%d.%d: doing " #y "\n", __LINE__, i); \
		x=y; \
		ASSERT_TRUE(x) << #y; \
	} while(0)

class peerdirect_base_test : public testing::Test {
protected:
	enum {
		PAIR = 2,
		SZ = 512,
		MAX_CHANELS = 2,
		MAX_WC = 6,
		MAX_WR = 8,
	};

	struct ibv_port_attr port_attr[PAIR];
	struct ibv_context *context[PAIR];
	struct ibv_pd *domain[PAIR];
	struct ibv_cq *queue[PAIR*MAX_CHANELS];
	struct ibv_qp *queue_pair[PAIR*MAX_CHANELS];
	struct ibv_mr *region[PAIR*MAX_CHANELS];
	char buff[PAIR*MAX_CHANELS][SZ];
	bool fatality;

	void SetUp() {
		int num_devices;
		struct ibv_device **dev_list = NULL;
		int i = 0;

		SET(dev_list, ibv_get_device_list(&num_devices));
		ASSERT_EQ(num_devices, PAIR);

		for (i=0; i<PAIR; i++) {
			SET(context[i], ibv_open_device(dev_list[i]));
			DO(ibv_query_port(context[i], 1, &port_attr[i]));
			SET(domain[i], ibv_alloc_pd(context[i]));
		}

		ibv_free_device_list(dev_list);
	}

	void TearDown() {
		int i;

		for (i = 0 ; i < PAIR; i++) {
			if (domain[i])
				ibv_dealloc_pd(domain[i]);
			if (context[i])
				ibv_close_device(context[i]);
		}
	}

	void poll(int i) {
		struct ibv_wc wc[MAX_WC];
		int result = 0;

		VERBS_INFO("polling %d...\n", i);

		while (!result) {
			result = ibv_poll_cq(queue[i], MAX_WC, wc);
			ASSERT_FALSE(result<0);
		}

		for (i=0; i<result; i++) 
			if (wc[i].status)
				VERBS_INFO("poll status %s(%d) opcode %d len %d qp %d lid %d\n", 
						ibv_wc_status_str(wc[i].status),
						wc[i].status, wc[i].opcode, wc[i].byte_len, wc[i].qp_num, wc[i].slid);
	}

	void xmit(int i, int start, int length) {
		struct ibv_send_wr wr;
		struct ibv_sge sge;
		struct ibv_send_wr *bad_wr = NULL;

		memset(&sge, 0, sizeof(sge));
		sge.addr = (uintptr_t)buff[i]+start;
		sge.length = length;
		sge.lkey = region[0]->lkey;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0;
		wr.sg_list = &sge;
		wr.num_sge = 1;
		wr.opcode = IBV_WR_SEND;
		wr.send_flags = IBV_SEND_SIGNALED;
		DO(ibv_post_send(queue_pair[i], &wr, &bad_wr));
	}
		
	void ops2wr(struct peer_op_wr *op, int entries, int peer, void* buff,
			struct ibv_send_wr *wr, struct ibv_sge *sge) {
		char * ptr = (char *)buff;
		int i;
			
		memset(wr, 0, MAX_WR*sizeof(*wr));
		memset(sge, 0, MAX_WR*sizeof(*sge));
		for(i=0; i < entries; op = op->next, i++) {
			wr[i].opcode = IBV_WR_RDMA_WRITE;
			if (op->type == IBV_PEER_OP_STORE_DWORD ||
					op->type == IBV_PEER_OP_POLL_AND_DWORD ||
					op->type == IBV_PEER_OP_POLL_NOR_DWORD) {
				sge[i].addr = (uintptr_t)ptr;
				memcpy(ptr, &op->wr.dword_va.data, 4);
				ptr += 4;
				wr[i].wr.rdma.remote_addr = (uintptr_t)op->wr.dword_va.target_va;
				sge[i].length = 4;
				if (op->type != IBV_PEER_OP_STORE_DWORD) 
					wr[i].opcode = IBV_WR_RDMA_READ;
			} else if (op->type == IBV_PEER_OP_STORE_QWORD) {
				sge[i].addr = (uintptr_t)ptr;
				memcpy(ptr, &op->wr.qword_va.data, 8);
				ptr += 8;
				wr[i].wr.rdma.remote_addr = (uintptr_t)op->wr.qword_va.target_va;
				sge[i].length = 8;
			} else {
				ASSERT_TRUE(0) << "unknown type: " << op->type;
			}

			for (std::list<peer_mr *>::iterator it=mr_list.begin(); it!=mr_list.end(); it++) {
				uintptr_t start = wr[i].wr.rdma.remote_addr, end = start + sge[i].length - 1;
				if (start >= (*it)->start && end <= (*it)->end) {
					wr[i].wr.rdma.rkey = (*it)->region->rkey;
					break;
				}
			}
			ASSERT_TRUE(wr[i].wr.rdma.rkey);
			sge[i].lkey = region[peer]->lkey;
			wr[i].sg_list = &sge[i];
			wr[i].num_sge = 1;
			if (i)
				wr[i-1].next = &wr[i];
			wr[i].next = NULL;
		}
	}

	struct xmit_peer_ctx {
		struct ibv_send_wr wr1[MAX_WR], wr2[MAX_WR];
		struct ibv_sge sge1[MAX_WR], sge2[MAX_WR];
		int peek_op_type;
		uint32_t peek_op_data;
		uint64_t peek_id;
		uint64_t rollback_id;
		union {
			struct {
				struct {
					uint32_t db_record;	
					uint64_t db_ring;	
				} commit;
				struct {
					uint32_t owner;	
				} peek;
			} * f;
			char * buff;
		} ctrl;
	};

	void xmit_peer(int i, int peer) {
		xmit_peer_ctx ctx;
		ASSERT_NO_FATAL_FAILURE(peer_prep(i, peer, &ctx));
		ASSERT_NO_FATAL_FAILURE(peer_exec(i, peer, &ctx));
	}

	void peer_prep(int i, int peer, xmit_peer_ctx* ctx, int offset = 0) {
		int n = MAX_WR;
		struct peer_op_wr op_buff1[MAX_WR], op_buff2[MAX_WR];
		struct ibv_peer_commit commit_ops;
		commit_ops.storage = op_buff1;
		commit_ops.entries = MAX_WR;
		commit_ops.peer_queue_token = (uintptr_t)queue_pair[0];
		struct ibv_peer_peek peek_ops;
		peek_ops.storage = op_buff2;
		peek_ops.entries = MAX_WR;
		peek_ops.cqe_offset_from_head = offset;

		ctx->ctrl.buff = buff[peer];

		memset(&op_buff1, 0, sizeof(op_buff1));
		memset(&op_buff2, 0, sizeof(op_buff2));
		while(--n) {
			op_buff1[n-1].next = &op_buff1[n];
			op_buff2[n-1].next = &op_buff2[n];
		}

		DO(ibv_peer_commit_qp(queue_pair[i], &commit_ops));
		DO(ibv_peer_peek_cq(queue[i], &peek_ops));
		ASSERT_NO_FATAL_FAILURE(ops2wr(commit_ops.storage, commit_ops.entries, peer, &ctx->ctrl.f->commit, ctx->wr1, ctx->sge1));
		ASSERT_NO_FATAL_FAILURE(ops2wr(peek_ops.storage, peek_ops.entries, peer, &ctx->ctrl.f->peek, ctx->wr2, ctx->sge2));
		ctx->rollback_id = commit_ops.rollback_abort_identifier;
		ctx->peek_op_type = peek_ops.storage->type;
		ctx->peek_op_data = peek_ops.storage->wr.dword_va.data;
		ctx->peek_id = peek_ops.peek_id;
	} 

	void peer_exec(int i, int peer, xmit_peer_ctx* ctx) {
		struct ibv_send_wr *bad_wr = NULL;

		DO(ibv_post_send(queue_pair[peer], ctx->wr1, &bad_wr));
		poll(peer);

		VERBS_INFO("peer polling...\n");
		while(1) {
			DO(ibv_post_send(queue_pair[peer], ctx->wr2, &bad_wr));
			poll(peer);
			if (ctx->peek_op_type == IBV_PEER_OP_POLL_AND_DWORD) {
				if (ctx->ctrl.f->peek.owner & ctx->peek_op_data)
					break;
			} else if (ctx->peek_op_type == IBV_PEER_OP_POLL_NOR_DWORD) {
				if (~(ctx->ctrl.f->peek.owner | ctx->peek_op_data))
					break;
			} else {
				ASSERT_TRUE(0) << "unknown type: " << ctx->peek_op_type;
			}
		}

		struct ibv_peer_ack_peek ack_ops;
		ack_ops.peek_id = ctx->peek_id;
		DO(ibv_peer_ack_peek_cq(queue[i], &ack_ops));
		
	}

	void peer_fail(int i, int flags, xmit_peer_ctx* ctx) {
		struct ibv_rollback_ctx rb_ctx;
		rb_ctx.rollback_abort_identifier = ctx->rollback_id;
		rb_ctx.flags = flags;
		DO(ibv_rollback_qp(queue_pair[i], &rb_ctx));
	}

	void recv(int i, int start, int length) {
		struct ibv_recv_wr wr;
		struct ibv_sge sge;
		struct ibv_recv_wr *bad_wr = NULL;

		memset(&sge, 0, sizeof(sge));
		sge.addr = (uintptr_t)buff[i]+start;
		sge.length = length;
		sge.lkey = region[i]->lkey;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0;
		wr.sg_list = &sge;
		wr.num_sge = 1;
		DO(ibv_post_recv(queue_pair[i], &wr, &bad_wr));
	}

	struct peer_ctx {
		peerdirect_base_test& t;
		struct ibv_pd *domain;

		peer_ctx(peerdirect_base_test& _t, struct ibv_pd *d) : t(_t), domain(d) {}
	};

	struct peer_mr {
		peer_ctx &ctx;
		struct ibv_mr *region;
		uintptr_t start;
		uintptr_t end;
		peer_mr(peer_ctx &c, void* base, size_t length) : ctx(c) {
			start = (uintptr_t)base;
			end = start+length-1;
		}
	};

	std::list<peer_mr *> mr_list;

	static inline int qp2dev(int i) {
	       return i/2;
	}

	static uint64_t register_va_for_peer(void *start, size_t length, uint64_t peer_id) {
		/* TODO mutex */
		peer_ctx *ctx = (peer_ctx *)peer_id;
		peer_mr *reg_h = new peer_mr(*ctx, start, length);
		       
		reg_h->region = ibv_reg_mr(ctx->domain, start, length,
					IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
		if (!reg_h->region) {
			EXPECT_TRUE(false) << "ibv_reg_mr on peer memory failed";
			reg_h->ctx.t.fatality = true;
		}
		
		reg_h->ctx.t.mr_list.push_back(reg_h);

		return (uint64_t)reg_h;
	}

	static void unregister_va_for_peer(uint64_t registration_id, uint64_t peer_id) {
		peer_mr *reg_h = (peer_mr *)registration_id;
		if(ibv_dereg_mr(reg_h->region)) {
			EXPECT_TRUE(false) << "ibv_dereg_mr on peer memory failed";
			reg_h->ctx.t.fatality = true;
		}
		reg_h->ctx.t.mr_list.remove(reg_h);
		delete reg_h;
	}

	static void* work_queue_alloc(size_t sz, enum ibv_peer_direction expected_direction, uint64_t peer_id) {
		void* buf = mmap(NULL, sz, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
		if (buf == MAP_FAILED)
			return 0;

		return buf;
	}

	static void work_queue_release(void *queue, size_t sz, uint64_t peer_id) {
		munmap(queue, sz);
	}

	void init_pd_qp(int i, int peer) {		
		struct ibv_qp_init_attr_ex qp_attr;
		struct ibv_create_cq_attr_ex cq_attr;
		struct ibv_peer_direct_attr pd_attr;

		memset(&qp_attr, 0, sizeof(qp_attr));
		memset(&cq_attr, 0, sizeof(cq_attr));
		memset(&pd_attr, 0, sizeof(pd_attr));

		peer_ctx * ctx = new peer_ctx(*this, domain[peer]);

		pd_attr.peer_id = (uint64_t)ctx;
		pd_attr.register_va_for_peer = register_va_for_peer;
		pd_attr.unregister_va_for_peer = unregister_va_for_peer;
		pd_attr.work_queue_alloc = work_queue_alloc;
		pd_attr.work_queue_release = work_queue_release;
		pd_attr.caps = IBV_PEER_OP_STORE_DWORD|IBV_PEER_OP_STORE_QWORD
			|IBV_PEER_OP_POLL_AND_DWORD|IBV_PEER_OP_POLL_NOR_DWORD;

		qp_attr.comp_mask = IBV_QP_INIT_ATTR_PEER_DIRECT|IBV_QP_INIT_ATTR_PD;
		qp_attr.qp_type = IBV_QPT_RC;
		qp_attr.sq_sig_all = 1;
		qp_attr.cap.max_send_wr = 10;
		qp_attr.cap.max_recv_wr = 10;
		qp_attr.cap.max_send_sge = 1;
		qp_attr.cap.max_recv_sge = 1;
		qp_attr.pd = domain[qp2dev(i)];
		qp_attr.peer_direct_attrs = &pd_attr;

		cq_attr.comp_mask = IBV_CREATE_CQ_ATTR_PEER_DIRECT;
		cq_attr.peer_direct_attrs = &pd_attr;

#ifdef PEER_DIRECT_EXP
		SET(queue[i], ibv_exp_create_cq(context[qp2dev(i)], 1, NULL, NULL, 0, &cq_attr));
#else
		cq_attr.cqe = 1;
		SET(queue[i], ibv_create_cq_ex(context[qp2dev(i)], &cq_attr));
#endif
		qp_attr.send_cq = qp_attr.recv_cq = queue[i];

		SET(region[i], ibv_reg_mr(domain[qp2dev(i)], buff[i], SZ, 
					IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE));
		SET(queue_pair[i], ibv_create_qp_ex(context[qp2dev(i)], &qp_attr));
		ASSERT_FALSE(fatality);
	}

	void init_qp(int i) {		
		struct ibv_qp_init_attr attr;
		SET(queue[i], ibv_create_cq(context[qp2dev(i)], 1, NULL, NULL, 0));
		SET(region[i], ibv_reg_mr(domain[qp2dev(i)], buff[i], SZ, 
					IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ));

		memset(&attr, 0, sizeof(attr));
		attr.qp_type = IBV_QPT_RC;
		attr.sq_sig_all = 1;
		attr.send_cq = queue[i];
		attr.recv_cq = queue[i];
		attr.cap.max_send_wr = 10;
		attr.cap.max_recv_wr = 10;
		attr.cap.max_send_sge = 1;
		attr.cap.max_recv_sge = 1;
		SET(queue_pair[i], ibv_create_qp(domain[qp2dev(i)], &attr));
	}

	void to_init(int i) {
		struct ibv_qp_attr attr;
		int flags;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_INIT;
		attr.port_num = 1;
		attr.pkey_index = 0;
		attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
		flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
		DO(ibv_modify_qp(queue_pair[i], &attr, flags));
	}

	void to_rtr(int i, int r) {
		struct ibv_qp_attr attr;
		int flags;
		uint32_t remote_qpn = queue_pair[r]->qp_num;
		uint16_t dlid = port_attr[qp2dev(r)].lid;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTR;
		attr.path_mtu = IBV_MTU_512;
		attr.dest_qp_num = remote_qpn;
		attr.rq_psn = 0;
		attr.max_dest_rd_atomic = 1;
		attr.min_rnr_timer = 12;
		attr.ah_attr.is_global = 0;
		attr.ah_attr.dlid = dlid;
		attr.ah_attr.sl = 0;
		attr.ah_attr.src_path_bits = 0;
		attr.ah_attr.port_num = 1;
		flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
			IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
		DO(ibv_modify_qp(queue_pair[i], &attr, flags));
	}

	void to_rts(int i) {
		struct ibv_qp_attr attr;
		int flags;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTS;
		attr.timeout = 14;
		attr.retry_cnt = 7;
		attr.rnr_retry = 7;
		attr.sq_psn = 0;
		attr.max_rd_atomic = 1;
		flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
			IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;

		DO(ibv_modify_qp(queue_pair[i], &attr, flags));
	}

	void init() {
		ASSERT_NO_FATAL_FAILURE(init_pd_qp(0, qp2dev(2)));
		ASSERT_NO_FATAL_FAILURE(init_pd_qp(1, qp2dev(3)));

		ASSERT_NO_FATAL_FAILURE(init_qp(2));
		ASSERT_NO_FATAL_FAILURE(init_qp(3));

		ASSERT_NO_FATAL_FAILURE(to_init(0));
		ASSERT_NO_FATAL_FAILURE(to_init(1));
		ASSERT_NO_FATAL_FAILURE(to_init(2));
		ASSERT_NO_FATAL_FAILURE(to_init(3));

		VERBS_INFO("connect lid %d-%d qp0 %d qp1 %d qp2 %d qp3 %d\n", 
				port_attr[0].lid, port_attr[1].lid,
				queue_pair[0]->qp_num, queue_pair[1]->qp_num,
				queue_pair[2]->qp_num, queue_pair[3]->qp_num);

		ASSERT_NO_FATAL_FAILURE(to_rtr(1, 0));
		ASSERT_NO_FATAL_FAILURE(to_rtr(0, 1));
		ASSERT_NO_FATAL_FAILURE(to_rtr(3, 2));
		ASSERT_NO_FATAL_FAILURE(to_rtr(2, 3));

		ASSERT_NO_FATAL_FAILURE(to_rts(0));
		ASSERT_NO_FATAL_FAILURE(to_rts(2));

		memset(buff[0], 0x5a, SZ);
		memset(buff[1], 0, SZ);

	}

	void fini() {
		int i;
		ASSERT_FALSE(HasFailure());
		ASSERT_NO_FATAL_FAILURE(poll(1));

		for (i = 0; i < SZ; i++)
			ASSERT_EQ(buff[1][i], 0x5a);

		for (i = 0 ; i < PAIR*MAX_CHANELS; i++) {
			if (queue_pair[i])
				ASSERT_FALSE(ibv_destroy_qp(queue_pair[i]));
			queue_pair[i] = NULL;
			if (region[i])
				ASSERT_FALSE(ibv_dereg_mr(region[i]));
			region[i] = NULL;
			if (queue[i]) 
				ASSERT_FALSE(ibv_destroy_cq(queue[i]));
			queue[i] = NULL;
		}
	}
};

class peerdirect_test : public peerdirect_base_test {
protected:
	virtual void SetUp() {
		ASSERT_NO_FATAL_FAILURE(peerdirect_base_test::SetUp());
		ASSERT_NO_FATAL_FAILURE(peerdirect_base_test::init());
	}

	virtual void TearDown() {
		ASSERT_NO_FATAL_FAILURE(peerdirect_base_test::fini());
		ASSERT_NO_FATAL_FAILURE(peerdirect_base_test::TearDown());
	}

};

TEST_F(peerdirect_base_test, t0_no_pd) {
	ASSERT_NO_FATAL_FAILURE(init_qp(0));
	ASSERT_NO_FATAL_FAILURE(init_qp(1));

	ASSERT_NO_FATAL_FAILURE(to_init(0));
	ASSERT_NO_FATAL_FAILURE(to_init(1));

	ASSERT_NO_FATAL_FAILURE(to_rtr(1, 0));
	ASSERT_NO_FATAL_FAILURE(to_rtr(0, 1));

	ASSERT_NO_FATAL_FAILURE(to_rts(0));

	memset(buff[0], 0x5a, SZ);
	memset(buff[1], 0, SZ);

	ASSERT_NO_FATAL_FAILURE(recv(1, 0, SZ/2));
	ASSERT_NO_FATAL_FAILURE(xmit(0, 0, SZ/2));
	ASSERT_NO_FATAL_FAILURE(poll(0));

	ASSERT_NO_FATAL_FAILURE(recv(1, SZ/2, SZ/2));
	ASSERT_NO_FATAL_FAILURE(xmit(0, SZ/2, SZ/2));
	ASSERT_NO_FATAL_FAILURE(poll(0));

	ASSERT_NO_FATAL_FAILURE(fini());
}

TEST_F(peerdirect_test, t1_1_send) {
	ASSERT_NO_FATAL_FAILURE(recv(1, 0, SZ));
	ASSERT_NO_FATAL_FAILURE(xmit(0, 0, SZ));
	ASSERT_NO_FATAL_FAILURE(xmit_peer(0, 2));
}

TEST_F(peerdirect_test, t2_2_sends) {
	xmit_peer_ctx ctx;
	ASSERT_NO_FATAL_FAILURE(recv(1, 0, SZ/2));
	ASSERT_NO_FATAL_FAILURE(recv(1, SZ/2, SZ/2));
	ASSERT_NO_FATAL_FAILURE(xmit(0, 0, SZ/2));
	ASSERT_NO_FATAL_FAILURE(xmit(0, SZ/2, SZ/2));
	ASSERT_NO_FATAL_FAILURE(peer_prep(0, 2, &ctx, 1));
	ASSERT_NO_FATAL_FAILURE(peer_exec(0, 2, &ctx));
}

TEST_F(peerdirect_test, t3_2_sends_2_pd) {
	xmit_peer_ctx ctx1, ctx2;
	ASSERT_NO_FATAL_FAILURE(xmit(0, 0, SZ/2));
	ASSERT_NO_FATAL_FAILURE(peer_prep(0, 2, &ctx1));
	ASSERT_NO_FATAL_FAILURE(xmit(0, SZ/2, SZ/2));
	ASSERT_NO_FATAL_FAILURE(peer_prep(0, 2, &ctx2));

	ASSERT_NO_FATAL_FAILURE(recv(1, 0, SZ/2));
	ASSERT_NO_FATAL_FAILURE(recv(1, SZ/2, SZ/2));
	ASSERT_NO_FATAL_FAILURE(peer_exec(0, 2, &ctx1));
	ASSERT_NO_FATAL_FAILURE(peer_exec(0, 2, &ctx2));
}

TEST_F(peerdirect_test, t4_rollback) {
	xmit_peer_ctx ctx1, ctx2;
	ASSERT_NO_FATAL_FAILURE(xmit(0, 0, SZ/2));
	ASSERT_NO_FATAL_FAILURE(peer_prep(0, 2, &ctx1));
	ASSERT_NO_FATAL_FAILURE(xmit(0, SZ/2, SZ/2));
	ASSERT_NO_FATAL_FAILURE(peer_prep(0, 2, &ctx2));

	ASSERT_NO_FATAL_FAILURE(recv(1, 0, SZ/2));
	ASSERT_NO_FATAL_FAILURE(recv(1, SZ/2, SZ/2));

	ASSERT_NO_FATAL_FAILURE(peer_exec(0, 2, &ctx1));
	ASSERT_NO_FATAL_FAILURE(peer_fail(0, 0, &ctx2));

	ASSERT_NO_FATAL_FAILURE(xmit(0, SZ/2, SZ/2));
	ASSERT_NO_FATAL_FAILURE(xmit_peer(0, 2));
}

