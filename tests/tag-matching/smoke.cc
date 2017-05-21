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
 TR_END
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
#include "enum.h"

#ifdef HAVE_INFINIBAND_VERBS_EXP_H
#include <infiniband/verbs_exp.h>

#define ibv_poll_cq(a,b,c) ibv_exp_poll_cq(a,b,c,sizeof(*(c)))
#define ibv_post_send ibv_exp_post_send
#define ibv_send_wr ibv_exp_send_wr
#define ibv_wc ibv_exp_wc
#define IBV_WC_BIND_MW IBV_EXP_WC_BIND_MW
#define IBV_WC_COMP_SWAP IBV_EXP_WC_COMP_SWAP
#define IBV_WC_FETCH_ADD IBV_EXP_WC_FETCH_ADD
#define IBV_WC_LOCAL_INV IBV_EXP_WC_LOCAL_INV
#define ibv_wc_opcode ibv_exp_wc_opcode
#define ibv_wc_opcode_str ibv_exp_wc_opcode_str
#define IBV_WC_RDMA_READ IBV_EXP_WC_RDMA_READ
#define IBV_WC_RDMA_WRITE IBV_EXP_WC_RDMA_WRITE
#define IBV_WC_RECV IBV_EXP_WC_RECV
#define IBV_WC_RECV_RDMA_WITH_IMM IBV_EXP_WC_RECV_RDMA_WITH_IMM
#define IBV_WC_SEND IBV_EXP_WC_SEND
#define ibv_wr_opcode ibv_exp_wr_opcode
#define IBV_WR_TAG_APPEND IBV_EXP_WR_TAG_APPEND
#define IBV_WR_TAG_REMOVE IBV_EXP_WR_TAG_REMOVE
#define IBV_WR_TAG_SEND_EAGER IBV_EXP_WR_TAG_SEND_EAGER
#define IBV_WR_TAG_SEND_EAGER_WITH_IMM IBV_EXP_WR_TAG_SEND_EAGER_WITH_IMM
#define IBV_WR_TAG_SEND_NO_TAG IBV_EXP_WR_TAG_SEND_NO_TAG
#define IBV_WR_TAG_SEND_RNDV IBV_EXP_WR_TAG_SEND_RNDV
#define opcode exp_opcode
#define send_flags exp_send_flags
#define IBV_WC_TM_RECV IBV_EXP_WC_TM_RECV
#define IBV_WC_TM_UNEXP IBV_EXP_WC_TM_UNEXP
#define IBV_WC_TM_CONSUMED_EAGER IBV_EXP_WC_TM_CONSUMED_EAGER
#define IBV_WC_TM_CONSUMED_RNDV IBV_EXP_WC_TM_CONSUMED_RNDV
#define IBV_WC_TM_CONSUMED_SW_RNDV IBV_EXP_WC_TM_CONSUMED_SW_RNDV
#define IBV_WC_TM_RECV_CONSUMED_EAGER IBV_EXP_WC_TM_RECV_CONSUMED_EAGER
#define IBV_WC_TM_RECV_CONSUMED_RNDV IBV_EXP_WC_TM_RECV_CONSUMED_RNDV
#define IBV_WC_TM_RECV_CONSUMED_SW_RNDV IBV_EXP_WC_TM_RECV_CONSUMED_SW_RNDV
#define IBV_WC_TM_NO_TAG IBV_EXP_WC_TM_NO_TAG
#define IBV_WC_TM_APPEND IBV_EXP_WC_TM_APPEND
#define IBV_WC_TM_REMOVE IBV_EXP_WC_TM_REMOVE
#define IBV_WC_TM_NOOP IBV_EXP_WC_TM_NOOP
#endif

#define DEF_ibv_wc_opcode \
	DEF_ENUM_ELEM(IBV_WC_SEND) \
	DEF_ENUM_ELEM(IBV_WC_RDMA_WRITE) \
	DEF_ENUM_ELEM(IBV_WC_RDMA_READ) \
	DEF_ENUM_ELEM(IBV_WC_COMP_SWAP) \
	DEF_ENUM_ELEM(IBV_WC_FETCH_ADD) \
	DEF_ENUM_ELEM(IBV_WC_BIND_MW) \
	DEF_ENUM_ELEM(IBV_WC_LOCAL_INV) \
	DEF_ENUM_ELEM(IBV_WC_RECV) \
	DEF_ENUM_ELEM(IBV_WC_RECV_RDMA_WITH_IMM) \
	DEF_ENUM_ELEM(IBV_WC_TM_RECV) \
	DEF_ENUM_ELEM(IBV_WC_TM_NO_TAG) \
	DEF_ENUM_ELEM(IBV_WC_TM_APPEND) \
	DEF_ENUM_ELEM(IBV_WC_TM_REMOVE) \
	DEF_ENUM_ELEM(IBV_WC_TM_NOOP)
#define OLD_TM_ARCH_ibv_wc_opcode \
	DEF_ENUM_ELEM(IBV_WC_TM_UNEXP) \
	DEF_ENUM_ELEM(IBV_WC_TM_CONSUMED_EAGER) \
	DEF_ENUM_ELEM(IBV_WC_TM_CONSUMED_RNDV) \
	DEF_ENUM_ELEM(IBV_WC_TM_CONSUMED_SW_RNDV) \
	DEF_ENUM_ELEM(IBV_WC_TM_RECV_CONSUMED_EAGER) \
	DEF_ENUM_ELEM(IBV_WC_TM_RECV_CONSUMED_RNDV) \
	DEF_ENUM_ELEM(IBV_WC_TM_RECV_CONSUMED_SW_RNDV)

#define DEF_ENUM_ELEM DEF_ENUM_ELEM_TO_STR

DEF_ENUM_TO_STR_BEGIN(ibv_wc_opcode)
DEF_ibv_wc_opcode
DEF_ENUM_TO_STR_END

//#define max(a, b) ((a) > (b) ? (a) : (b))

struct tag_matching_base : public ibvt_env {
	int phase_cnt;
};

struct ibvt_srq : public ibvt_obj {
	struct ibv_srq *srq;

	ibvt_pd &pd;
	ibvt_cq &cq;

	tag_matching_base &tm;

	ibvt_srq(tag_matching_base &e, ibvt_pd &p, ibvt_cq &c) :
		 ibvt_obj(e), srq(NULL), pd(p), cq(c), tm(e) {}

	~ibvt_srq() {
		FREE(ibv_destroy_srq, srq);
	}

	virtual void init() {
		struct ibv_srq_init_attr_ex attr;

		EXEC(pd.init());
		EXEC(cq.init());

		attr.comp_mask =
			IBV_SRQ_INIT_ATTR_TYPE |
			IBV_SRQ_INIT_ATTR_PD |
			IBV_SRQ_INIT_ATTR_CQ |
			IBV_SRQ_INIT_ATTR_TAG_MATCHING;
		attr.srq_type = IBV_SRQT_TAG_MATCHING;
		attr.pd = pd.pd;
		attr.cq = cq.cq;
		attr.tm_cap.max_tm_ops = 10;
		attr.tm_cap.max_num_tags = 64;
		attr.attr.max_wr  = 128;
		attr.attr.max_sge = 1;

		SET(srq, ibv_create_srq_ex(pd.ctx.ctx, &attr));
	}

	virtual void unexp(ibvt_mr &mr, int start, int length) {
		struct ibv_recv_wr wr;
		struct ibv_sge sge = mr.sge(start, length);
		struct ibv_recv_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0x56789;
		wr.sg_list = &sge;
		wr.num_sge = 1;
		DO(ibv_post_srq_recv(srq, &wr, &bad_wr));
	}

	virtual void append(ibvt_mr &mr, int start, int length,
			    uint64_t tag, int *idx)
	{
		struct ibv_sge sge = mr.sge(start, length);
		struct ibv_ops_wr wr;
		struct ibv_ops_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.sg_list = &sge;
		wr.num_sge = 1;
		wr.flags = IBV_SEND_SIGNALED;
		wr.opcode = IBV_WR_TAG_APPEND;

		wr.tm.tag.tag = tag;
		wr.tm.mask.tag	= 0xffffffffffffffff;
		wr.tm.unexpected_cnt = tm.phase_cnt;

		DO(ibv_post_srq_ops(srq, &wr, &bad_wr));

		if (idx)
			*idx = wr.tm.handle;
	}

	virtual void remove(int idx)
	{
		struct ibv_ops_wr wr;
		struct ibv_ops_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.flags = IBV_SEND_SIGNALED;
		wr.opcode = IBV_WR_TAG_REMOVE;
		wr.tm.handle = idx;

		DO(ibv_post_srq_ops(srq, &wr, &bad_wr));
	}

};

struct ibvt_qp_tm : public ibvt_qp_rc {
	ibvt_ctx &ctx;

	ibvt_qp_tm(ibvt_env &e, ibvt_ctx &d, ibvt_pd &p, ibvt_cq &c) :
		    ibvt_qp_rc(e, p, c), ctx(d) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr)
	{
		ibvt_qp_rc::init_attr(attr);
		attr.cap.max_inline_data = 0x80;
	}

	virtual void send(ibv_sge sge,
			  uint64_t tag,
			  enum ibv_tm_op op)
	{
		struct ibv_send_wr wr;
		struct ibv_send_wr *bad_wr = NULL;
		struct ibv_tm_info tm;

		memset(&tm, 0 ,sizeof(tm));
		tm.op = op;
		tm.tag.tag = tag;
		ASSERT_GE(ibv_pack_tm_info(ctx.ctx, (void*)sge.addr, &tm), 1);

		memset(&wr, 0, sizeof(wr));
		wr.send_flags = IBV_SEND_SIGNALED;
		wr.opcode = IBV_WR_SEND;
		wr.num_sge = 1;
		wr.sg_list = &sge;
		wr.wr_id = 0x12345;

		DO(ibv_post_send(qp, &wr, &bad_wr));
	}

	virtual void rndv(ibv_sge sge, uint64_t tag)
	{
		struct ibv_send_wr wr;
		struct ibv_send_wr *bad_wr = NULL;
		ibvt_mr hdr(env, pd, 0x20);
		struct ibv_sge sge_hdr;
		struct ibv_tm_info tm;

		hdr.init();

		memset(&tm, 0 ,sizeof(tm));
		tm.op = IBV_TM_OP_RNDV;
		tm.tag.tag = tag;
		tm.rndv.remote_mkey = sge.lkey;
		tm.rndv.buffer_vaddr = sge.addr;
		tm.rndv.buffer_len = sge.length;
		ASSERT_GE(ibv_pack_tm_info(ctx.ctx, (void*)hdr.buff, &tm), 1);
		//hdr.dump();

		memset(&wr, 0, sizeof(wr));
		wr.send_flags = IBV_SEND_SIGNALED;
		wr.opcode = IBV_WR_SEND;
		wr.num_sge = 1;
		sge_hdr = hdr.sge();
		wr.sg_list = &sge_hdr;
		wr.wr_id = 0x12345;

		DO(ibv_post_send(qp, &wr, &bad_wr));
	}
};

struct ibvt_qp_srq : public ibvt_qp_rc {
	ibvt_srq &srq;

	ibvt_qp_srq(ibvt_env &e, ibvt_pd &p, ibvt_cq &c, ibvt_srq &s) :
		    ibvt_qp_rc(e, p, c), srq(s) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr)
	{
		ibvt_qp_rc::init_attr(attr);
		attr.recv_cq = NULL;
		attr.srq = srq.srq;
	}

};

struct ibvt_qp_rndv : public ibvt_qp_srq {
	ibvt_qp_rndv(ibvt_env &e, ibvt_pd &p, ibvt_cq &c, ibvt_srq &s) :
		    ibvt_qp_srq(e, p, c, s) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr)
	{
		ibvt_qp_srq::init_attr(attr);
		attr.cap.max_send_wr = 0;
	}
};

struct ibvt_cq_tm : public ibvt_cq {
	tag_matching_base &tm;

	ibvt_cq_tm(tag_matching_base &e, ibvt_ctx &c) : ibvt_cq(e, c), tm(e) {}

	virtual void poll(int n) {
		struct ibv_wc wc = {};
		int result = 0, retries = POLL_RETRIES;

		VERBS_TRACE("%d.%p polling...\n", __LINE__, this);

		while (!result && --retries) {
			result = ibv_poll_cq(cq, 1, &wc);
			ASSERT_GE(result,0);
		}
		ASSERT_GT(retries,0) << "errno: " << errno;

		if (wc.opcode == IBV_WC_TM_RECV && !(wc.wc_flags & (IBV_WC_TM_MATCH | IBV_WC_TM_DATA_VALID)))
			tm.phase_cnt ++;

		VERBS_INFO("poll status %s(%d) opcode %s(%d) len %d flags %x lid %x wr_id %lx\n",
				ibv_wc_status_str(wc.status), wc.status,
				ibv_wc_opcode_str(wc.opcode), wc.opcode,
				wc.byte_len, wc.wc_flags, wc.slid,
				wc.wr_id);
		ASSERT_FALSE(wc.status) << ibv_wc_status_str(wc.status);
		if (n && !wc.byte_len)
			EXEC(poll(0));
	}
};

struct tag_matching : public testing::TestWithParam<int>, public tag_matching_base {
	struct ibvt_ctx ctx;
	struct ibvt_pd pd;
	struct ibvt_cq_tm srq_cq;
	struct ibvt_srq srq;
	struct ibvt_cq send_cq;
	struct ibvt_qp_tm send_qp;
	struct ibvt_cq recv_cq;
	struct ibvt_qp_rndv recv_qp;
	struct ibvt_mr src_mr;
	struct ibvt_mr dst_mr;

	tag_matching() :
		ctx(*this, NULL),
		pd(*this, ctx),
		srq_cq(*this, ctx),
		srq(*this, pd, srq_cq),
		send_cq(*this, ctx),
		send_qp(*this, ctx, pd, send_cq),
		recv_cq(*this, ctx),
		recv_qp(*this, pd, recv_cq, srq),
		src_mr(*this, pd, SZ()),
		dst_mr(*this, pd, SZ())
	{ }

	int SZ() { return GetParam(); }

	void eager(int start, int length, uint64_t tag) {
		EXEC(send_qp.send(src_mr.sge(start, length), tag, IBV_TM_OP_EAGER));

		EXEC(send_cq.poll(1));
		EXEC(srq_cq.poll(1));
	}

	void rndv(int start, int length, uint64_t tag) {
		ibvt_mr fin(*this, pd, 0x20);
		fin.init();
		EXEC(send_qp.recv(fin.sge(0, 0x20)));
		EXEC(send_qp.rndv(src_mr.sge(start, length), tag));
		EXEC(send_cq.poll(1));
		EXEC(srq_cq.poll(1));
	}

	void append(int start, int length, uint64_t tag, int *idx = NULL) {
		EXEC(srq.append(dst_mr, start, length, tag, idx));
		EXEC(srq_cq.poll(0));
	}

	void remove(int idx) {
		EXEC(srq.remove(idx));
		EXEC(srq_cq.poll(0));
	}

	void recv(int start, int length) {
		EXEC(srq.unexp(dst_mr, start, length));
	}

	virtual void SetUp() {
		INIT(ctx.init());
		INIT(srq.init());
		INIT(send_qp.init());
		INIT(recv_qp.init());
		INIT(send_qp.connect(&recv_qp));
		INIT(recv_qp.connect(&send_qp));
		INIT(src_mr.fill());
		INIT(dst_mr.init());
	}

	virtual void fix_uwq() {
		for(int i = 0; i<33; i++)
			EXEC(recv(0, SZ()));
		phase_cnt = 0;
	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());
	}
};


TEST_P(tag_matching, e0_unexp) {
	CHK_SUT(tag-matching);
	EXEC(recv(0, SZ()));
	EXEC(fix_uwq());
	EXEC(eager(0, SZ(), 0x12345));
	EXEC(dst_mr.check(0x10));
}

TEST_P(tag_matching, e1_match) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, SZ(), 1));
	EXEC(eager(0, SZ(), 1));
	EXEC(dst_mr.check(0, 0x10));
}

#if 0
TEST_P(tag_matching, u0_short) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, SZ(), 1));
	EXEC(send_qp.send(this->src_mr.sge(0, 0x40), 1,
			  IBV_SEND_INLINE));

	EXEC(send_cq.poll(1));
	EXEC(srq_cq.poll(1));
	//EXEC(dst_mr.dump());
}

TEST_P(tag_matching, u2_rndv) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, SZ(), 1));
	EXEC(send_qp.rndv(this->src_mr.sge(0, SZ()), 1));
	EXEC(send_cq.poll(1));
	EXEC(srq_cq.poll(1));
	EXEC(dst_mr.check());
}

TEST_P(tag_matching, u3_rndv_unexp) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(send_qp.rndv(this->src_mr.sge(0, SZ()), 1));
	EXEC(send_cq.poll(1));
	EXEC(srq_cq.poll(1));
	//EXEC(dst_mr.dump());
}

TEST_P(tag_matching, u4_rndv_sw) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, SZ()/2, 1));
	EXEC(send_qp.rndv(this->src_mr.sge(0, SZ()), 1));
	EXEC(send_cq.poll(1));
	EXEC(srq_cq.poll(1));
	//EXEC(dst_mr.dump());
}

TEST_P(tag_matching, u1_imm) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, SZ(), 1));
	EXEC(send_qp.send(this->src_mr.sge(0, 0x20), 1,
			  IBV_WR_TAG_SEND_EAGER_WITH_IMM,
			  IBV_SEND_INLINE));

	EXEC(send_cq.poll(1));
	EXEC(srq_cq.poll(1));
}
#endif

TEST_P(tag_matching, e2_matchN) {
	int i, N = std::min(SZ()/0x20, 8);
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	for (i=0; i<N; i++)
		EXEC(append(SZ()/N*i, SZ()/N, i));
	EXEC(append(0, SZ(), 0x333));
	for (i=0; i<N; i++)
		EXEC(eager(SZ()/N*i, SZ()/N, i));
	EXEC(eager(0, SZ(), 0x333));
	EXEC(dst_mr.check(0, 0x10, N));
}

TEST_P(tag_matching, e3_remove) {
	int i3, i4;
	CHK_SUT(tag-matching);

	EXEC(fix_uwq());
	EXEC(append(0, SZ()/2, 3, &i3));
	EXEC(append(0, SZ()/2, 1));
	EXEC(remove(i3));
	EXEC(append(SZ()/2, SZ()/2, 4, &i4));
	EXEC(remove(i4));
	EXEC(append(SZ()/2, SZ()/2, 2));
	EXEC(eager(SZ()/2, SZ()/2, 2));
	EXEC(eager(0, SZ()/2, 1));
	EXEC(dst_mr.check(0, 0x10, 2));
}

TEST_P(tag_matching, e4_repeat) {
	CHK_SUT(tag-matching);

	EXEC(fix_uwq());

	EXEC(recv(0, SZ()));
	EXEC(eager(0, SZ(), 5));
	EXEC(dst_mr.check(0x10, 0));

	EXEC(append(0, SZ(), 5));
	EXEC(eager(0, SZ(), 5));
	EXEC(dst_mr.check(0, 0x10));

	EXEC(recv(0, SZ()));
	EXEC(eager(0, SZ(), 5));
	EXEC(dst_mr.check(0x10, 0));

	EXEC(append(0, SZ(), 5));
	EXEC(eager(0, SZ(), 5));
	EXEC(dst_mr.check(0, 0x10));

	EXEC(recv(0, SZ()));
	EXEC(eager(0, SZ(), 5));
	EXEC(dst_mr.check(0x10, 0));
}

#undef N
#define N 17
#define R 15

TEST_P(tag_matching, e5_mix) {
	int i, idx[N], j, s;
	CHK_SUT(tag-matching);

	EXEC(fix_uwq());
	for (j=0; j<R; j++) {
		s = j&1;
		VERBS_INFO("round %d\n", j);
		for (i=s; i<N; i++)
			EXEC(append(0, SZ(), i, idx + i));
		for (i=s; i<N; i++)
			if (i&1) {
				EXEC(remove(idx[i]));
				EXEC(recv(0, SZ()));
			}
		for (i=s; i<N; i++) {
			EXEC(eager(0, SZ(), i));
			if (i&1)
				EXEC(dst_mr.check(0x10, 0));
			else
				EXEC(dst_mr.check(0, 0x10));
		}
	}
}

TEST_P(tag_matching, e6_unexp_inline) {
	ibvt_mr src(*this, this->pd, 0x20),
		dst(*this, this->pd, 0x20);
	CHK_SUT(tag-matching);

	src.fill();
	dst.init();
	EXEC(srq.unexp(dst, 0, 0x20));
	EXEC(fix_uwq());
	EXEC(send_qp.send(src.sge(0x10, 0x10), 0x12345, IBV_TM_OP_EAGER));
	EXEC(send_cq.poll(1));
	EXEC(srq_cq.poll(1));
}

TEST_P(tag_matching, e7_no_tag) {
	CHK_SUT(tag-matching);
	EXEC(recv(0, SZ()));
	EXEC(fix_uwq());
	EXEC(send_qp.send(this->src_mr.sge(0, SZ()), 0, IBV_TM_OP_NO_TAG));
	EXEC(send_cq.poll(1));
	EXEC(srq_cq.poll(1));
	EXEC(dst_mr.check());
}

TEST_P(tag_matching, e8_unexp_long) {
	int i;
	int n = SZ() / 0x40;
	if (n > 128)
		SKIP(0);

	CHK_SUT(tag-matching);
	for(i = 0; i < n; i++)
		EXEC(recv(SZ() / n * i, SZ() / n));
	EXEC(fix_uwq());
	for(i = 0; i < n; i++)
		EXEC(eager(SZ() / n * i,
			   SZ() / n,
			   1ULL << i));
	EXEC(dst_mr.check(0x10, 0, n));
}

TEST_P(tag_matching, r0_unexp) {
	ibvt_mr dst(*this, this->pd, 0x80);

	CHK_SUT(tag-matching);

	dst.init();
	EXEC(srq.unexp(dst, 0, 0x80));
	EXEC(fix_uwq());
	EXEC(send_qp.rndv(this->src_mr.sge(0, SZ()), 1));
	EXEC(send_cq.poll(1));
	EXEC(srq_cq.poll(1));
	//dst.dump();
}

TEST_P(tag_matching, r1_match) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, SZ(), 0x1234567890));
	EXEC(rndv(0, SZ(), 0x1234567890));
	//EXEC(dst_mr.dump());
	EXEC(dst_mr.check());

}

TEST_P(tag_matching, r2_match2) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, SZ()/2, 1));
	EXEC(append(SZ()/2, SZ()/2, 2));
	EXEC(rndv(SZ()/2, SZ()/2, 2));
	EXEC(rndv(0, SZ()/2, 1));
	EXEC(dst_mr.check());
}

TEST_P(tag_matching, r3_remove) {
	int i3, i4;
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, SZ()/2, 3, &i3));
	EXEC(append(0, SZ()/2, 1));
	EXEC(remove(i3));
	EXEC(append(SZ()/2, SZ()/2, 4, &i4));
	EXEC(remove(i4));
	EXEC(append(SZ()/2, SZ()/2, 2));
	EXEC(rndv(SZ()/2, SZ()/2, 2));
	EXEC(rndv(0, SZ()/2, 1));
	EXEC(dst_mr.check());
}

INSTANTIATE_TEST_CASE_P(tm, tag_matching, ::testing::Values(0x40, 0x2000, 0x40000));

