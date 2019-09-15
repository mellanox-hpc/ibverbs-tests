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

#define __STDC_LIMIT_MACROS
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
#define IBV_WR_SEND IBV_EXP_WR_SEND
#define IBV_WR_SEND_WITH_IMM IBV_EXP_WR_SEND_WITH_IMM
#define IBV_WR_TAG_ADD IBV_EXP_WR_TAG_ADD
#define IBV_WR_TAG_DEL IBV_EXP_WR_TAG_DEL
#define IBV_WR_TAG_SEND_EAGER IBV_EXP_WR_TAG_SEND_EAGER
#define IBV_WR_TAG_SEND_EAGER_WITH_IMM IBV_EXP_WR_TAG_SEND_EAGER_WITH_IMM
#define IBV_WR_TAG_SEND_NO_TAG IBV_EXP_WR_TAG_SEND_NO_TAG
#define IBV_WR_TAG_SEND_RNDV IBV_EXP_WR_TAG_SEND_RNDV
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
#define IBV_WC_TM_ADD IBV_EXP_WC_TM_ADD
#define IBV_WC_TM_DEL IBV_EXP_WC_TM_DEL
#define IBV_WC_TM_SYNC IBV_EXP_WC_TM_SYNC
#define IBV_WC_TM_NOOP IBV_EXP_WC_TM_NOOP
#define ibv_ops_wr ibv_exp_ops_wr
#define IBV_OPS_SIGNALED IBV_EXP_OPS_SIGNALED
#define IBV_OPS_TM_SYNC IBV_EXP_OPS_TM_SYNC
#define ibv_post_srq_ops ibv_exp_post_srq_ops
#define ibv_tmh ibv_exp_tmh
#define ibv_tmh_rvh ibv_exp_tmh_rvh
#define ibv_tmh_ravh ibv_exp_tmh_ravh
#define IBV_TMH_EAGER IBV_EXP_TMH_EAGER
#define IBV_TMH_RNDV IBV_EXP_TMH_RNDV
#define IBV_TMH_NO_TAG IBV_EXP_TMH_NO_TAG
#define IBV_WC_TM_MATCH IBV_EXP_WC_TM_MATCH
#define IBV_WC_TM_DATA_VALID IBV_EXP_WC_TM_DATA_VALID
#define ibv_rvh ibv_tmh_rvh
#define IBV_SRQ_INIT_ATTR_TM IBV_EXP_CREATE_SRQ_TM
#define IBV_SRQT_TM IBV_EXP_SRQT_TAG_MATCHING

#else
#define exp_opcode opcode
#include <infiniband/tm_types.h>
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
	DEF_ENUM_ELEM(IBV_WC_TM_ADD) \
	DEF_ENUM_ELEM(IBV_WC_TM_DEL) \
	DEF_ENUM_ELEM(IBV_WC_TM_SYNC)
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

struct tag_matching_base : public ibvt_env {
	int phase_cnt;
};

struct ibvt_srq_tm : public ibvt_srq {
	tag_matching_base &tm;
#if HAVE_DECL_IBV_EXP_CREATE_SRQ_DC_OFFLOAD_PARAMS
	struct ibv_exp_srq_dc_offload_params dc_op;
	void init_attr_dc(struct ibv_srq_init_attr_ex &attr) {
		memset(&dc_op, 0, sizeof(dc_op));
		dc_op.timeout = 12;
		dc_op.path_mtu = IBV_MTU_512;
		dc_op.pkey_index = 0;
		dc_op.sl = 0;
		dc_op.dct_key = DC_KEY;
		attr.comp_mask |= IBV_EXP_CREATE_SRQ_DC_OFFLOAD_PARAMS;
		attr.dc_offload_params = &dc_op;
	}
#else
	void init_attr_dc(struct ibv_srq_init_attr_ex &attr) {}
#endif

	ibvt_srq_tm(tag_matching_base &e, ibvt_pd &p, ibvt_cq &c) :
		 ibvt_srq(e, p, c), tm(e) {}

	virtual void init_attr(struct ibv_srq_init_attr_ex &attr) {
		ibvt_srq::init_attr(attr);
		attr.comp_mask |= IBV_SRQ_INIT_ATTR_TM;
		attr.srq_type = IBV_SRQT_TM;
		attr.tm_cap.max_ops = 10;
		attr.tm_cap.max_num_tags = 63;

		init_attr_dc(attr);
	}

	virtual void unexp(ibvt_mr &mr, int start, int length) {
		recv(mr.sge(start, length));
	}

	virtual void append(ibvt_mr &mr, int start, int length,
			    uint64_t tag, int *idx = NULL)
	{
		struct ibv_sge sge = mr.sge(start, length);
		EXEC(append(&sge, tag, idx));
	}

	virtual void append(ibv_sge *sge, uint64_t tag, int *idx = NULL)
	{
		struct ibv_ops_wr wr;
		struct ibv_ops_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.flags = IBV_OPS_SIGNALED | IBV_OPS_TM_SYNC;
		wr.opcode = IBV_WR_TAG_ADD;

		wr.tm.add.recv_wr_id = tag;
		if (sge) {
			wr.tm.add.sg_list = sge;
			wr.tm.add.num_sge = 1;
		} else {
			wr.tm.add.num_sge = 0;
		}
		wr.tm.add.tag = tag;
		wr.tm.add.mask = 0xffffffffffffffff;
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
		wr.flags = IBV_OPS_SIGNALED;
		wr.opcode = IBV_WR_TAG_DEL;
		wr.tm.handle = idx;

		DO(ibv_post_srq_ops(srq, &wr, &bad_wr));
	}

	virtual void arm()
	{
		struct ibv_srq_attr srq_attr;
		srq_attr.max_wr = 10;
		srq_attr.max_sge = 1;
		srq_attr.srq_limit = 16;
		DO(ibv_modify_srq (srq, &srq_attr, IBV_SRQ_LIMIT));
	}

};

struct ibvt_qp_tm_rc : public ibvt_qp_rc {
	ibvt_ctx &ctx;

	ibvt_qp_tm_rc(ibvt_env &e, ibvt_ctx &d, ibvt_pd &p, ibvt_cq &c) :
		ibvt_qp_rc(e, p, c), ctx(d) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr)
	{
		ibvt_qp_rc::init_attr(attr);
		attr.cap.max_inline_data = 0x80;
	}

	virtual void send(ibv_sge sge, uint64_t tag, int tm_op,
			  ibv_wr_opcode op = IBV_WR_SEND,
			  int flags = IBV_SEND_SIGNALED)
	{
		struct ibv_tmh *tmh = (ibv_tmh *)sge.addr;

		tmh->opcode = tm_op;
		tmh->tag = htobe64(tag);

		this->post_send(sge, op, flags);
	}

	virtual void rndv(ibv_sge sge, uint64_t tag)
	{
		ibvt_mr hdr(this->env, this->pd, 0x20);
		struct ibv_tmh *tmh;
		struct ibv_rvh *rvh;

		hdr.init();
		tmh = (ibv_tmh *)hdr.sge().addr;
		rvh = (ibv_rvh *)(tmh + 1);

		tmh->opcode = IBV_TMH_RNDV;
		tmh->tag = htobe64(tag);
		rvh->rkey = htobe32(sge.lkey);
		rvh->va = htobe64(sge.addr);
		rvh->len = htobe32(sge.length);

		this->post_send(hdr.sge(), IBV_WR_SEND);
	}
};

struct ibvt_qp_rndv : public ibvt_qp_rc {
	ibvt_srq &srq;

	ibvt_qp_rndv(ibvt_env &e, ibvt_pd &p, ibvt_cq &c, ibvt_srq_tm &s) :
		    ibvt_qp_rc(e, p, c), srq(s) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr)
	{
		ibvt_qp_rc::init_attr(attr);
		attr.srq = srq.srq;
		attr.cap.max_send_wr = 0;
	}
};

template <typename Base>
struct ibvt_cq_tm : public Base {
	tag_matching_base &tm;

	ibvt_cq_tm(tag_matching_base &e, ibvt_ctx &c) : Base(e, c), tm(e) {}

	virtual void poll(int n) {
		int len;
		do {
			struct ibvt_wc wc(*this);

			EXEC(do_poll(wc));
#ifndef HAVE_INFINIBAND_VERBS_EXP_H
			struct ibv_wc_tm_info tm_info = {};
			ibv_wc_read_tm_info(this->cq2(), &tm_info);
#endif
			if (wc().exp_opcode == IBV_WC_TM_RECV && !(wc().wc_flags & (IBV_WC_TM_MATCH | IBV_WC_TM_DATA_VALID)))
				tm.phase_cnt ++;

			VERBS_INFO("poll status %s(%d) opcode %s(%d) len %d flags %lx lid %x wr_id %lx\n",
					ibv_wc_status_str(wc().status), wc().status,
					ibv_wc_opcode_str(wc().exp_opcode), wc().exp_opcode,
					wc().byte_len, (uint64_t)wc().wc_flags, wc().slid,
					wc().wr_id);
			ASSERT_FALSE(wc().status) << ibv_wc_status_str(wc().status);
			len = wc().byte_len;
		} while (n-- && !len);
	}
};

struct tag_matching_rc : public tag_matching_base {
	struct ibvt_ctx ctx;
	struct ibvt_pd pd;
	struct ibvt_cq_tm<ibvt_cq> srq_cq;
	struct ibvt_srq_tm srq;
	struct ibvt_cq send_cq;
	struct ibvt_qp_tm_rc send_qp;
	struct ibvt_cq recv_cq;
	struct ibvt_qp_rndv recv_qp;
	struct ibvt_mr src_mr;
	struct ibvt_mr dst_mr;
	struct ibvt_mr fin;

	tag_matching_rc(int sz) :
		ctx(*this, NULL),
		pd(*this, ctx),
		srq_cq(*this, ctx),
		srq(*this, pd, srq_cq),
		send_cq(*this, ctx),
		send_qp(*this, ctx, pd, send_cq),
		recv_cq(*this, ctx),
		recv_qp(*this, pd, recv_cq, srq),
		src_mr(*this, pd, sz),
		dst_mr(*this, pd, sz),
		fin(*this, pd, 0x20)
	{}

	virtual void init() {
		INIT(ctx.init());
		INIT(srq.init());
		INIT(send_qp.init());
		INIT(recv_qp.init());
		INIT(recv_qp.connect(&send_qp));
		INIT(send_qp.connect(&recv_qp));
		INIT(src_mr.fill());
		INIT(dst_mr.init());
		INIT(fin.init());
		INIT(srq_cq.arm());
	}

	void rndv(int start, int length, uint64_t tag) {
		EXEC(send_qp.recv(fin.sge()));
		EXEC(send_qp.rndv(src_mr.sge(start, length), tag));
		EXEC(send_cq.poll());
		EXEC(send_cq.poll());
		EXEC(srq_cq.poll(1));
	}
};

#if HAVE_STRUCT_IBV_EXP_TMH_RAVH

struct ibvt_qp_tm_dc : public ibvt_qp_dc {
	ibvt_ctx &ctx;
	ibvt_dct *dlocal;

	ibvt_qp_tm_dc(ibvt_env &e, ibvt_ctx &d, ibvt_pd &p, ibvt_cq &c) :
		    ibvt_qp_dc(e, p, c), ctx(d) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr)
	{
		ibvt_qp_dc::init_attr(attr);
		attr.cap.max_inline_data = 0x80;
	}

	virtual void send(ibv_sge sge, uint64_t tag, int tm_op,
			  ibv_wr_opcode op = IBV_WR_SEND,
			  int flags = IBV_SEND_SIGNALED)
	{
		struct ibv_tmh *tmh = (ibv_tmh *)sge.addr;

		tmh->opcode = tm_op;
		tmh->tag = htobe64(tag);

		this->post_send(sge, op, flags);
	}

	virtual void connect(ibvt_dct *r, ibvt_dct *r2) {
		ibvt_qp_dc::connect(r);
		dlocal = r2;
	}

	virtual void rndv(ibv_sge sge, uint64_t tag)
	{
		ibvt_mr hdr(this->env, this->pd, 0x30);
		struct ibv_tmh *tmh;
		struct ibv_tmh_rvh *rvh;
		struct ibv_tmh_ravh *ravh;

		hdr.init();
		tmh = (ibv_tmh *)hdr.sge().addr;
		rvh = (ibv_tmh_rvh *)(tmh + 1);

		tmh->opcode = IBV_TMH_RNDV;
		tmh->tag = htobe64(tag);
		rvh->rkey = htobe32(sge.lkey);
		rvh->va = htobe64(sge.addr);
		rvh->len = htobe32(sge.length);

		ravh = (ibv_tmh_ravh *)(rvh + 1);
		ravh->sl_dct = htobe32(this->dlocal->dct->dct_num);
		ravh->dc_access_key = htobe64(DC_KEY);

		this->post_send(hdr.sge(), IBV_WR_SEND);
	}
};

struct tag_matching_dc : public tag_matching_base {
	struct ibvt_ctx ctx;
	struct ibvt_pd pd;
	struct ibvt_cq_tm<ibvt_cq> srq_cq;
	struct ibvt_srq_tm srq;
	struct ibvt_cq send_cq;
	struct ibvt_qp_tm_dc send_qp;
	struct ibvt_cq recv_cq;
	struct ibvt_dct dct;
	struct ibvt_cq rndv_cq;
	struct ibvt_srq rndv_srq;
	struct ibvt_dct rndv_dct;
	struct ibvt_mr src_mr;
	struct ibvt_mr dst_mr;
	struct ibvt_mr fin;

	tag_matching_dc(int sz) :
		ctx(*this, NULL),
		pd(*this, ctx),
		srq_cq(*this, ctx),
		srq(*this, pd, srq_cq),
		send_cq(*this, ctx),
		send_qp(*this, ctx, pd, send_cq),
		recv_cq(*this, ctx),
		dct(*this, pd, recv_cq, srq),
		rndv_cq(*this, ctx),
		rndv_srq(*this, pd, rndv_cq),
		rndv_dct(*this, pd, rndv_cq, rndv_srq),
		src_mr(*this, pd, sz),
		dst_mr(*this, pd, sz),
		fin(*this, pd, 0x30)
	{ }

	virtual void init() {
		INIT(ctx.init());
		INIT(srq.init());
		INIT(send_qp.init());
		INIT(dct.init());
		INIT(rndv_dct.init());
		INIT(send_qp.connect(&dct, &rndv_dct));
		INIT(src_mr.fill());
		INIT(dst_mr.init());
		INIT(fin.init());
	}

	void rndv(int start, int length, uint64_t tag) {
		EXEC(rndv_srq.recv(fin.sge()));
		EXEC(send_qp.rndv(src_mr.sge(start, length), tag));
		EXEC(send_cq.poll());
		EXEC(rndv_cq.poll());
		EXEC(srq_cq.poll(1));
	}
};

#endif

template <typename T1, int val>
struct types {
	typedef T1 Base;
	static const int size = val;
};

typedef testing::Types<
#if HAVE_STRUCT_IBV_EXP_TMH_RAVH
	types<tag_matching_dc, 0x40>,
	types<tag_matching_dc, 0x2000>,
	types<tag_matching_dc, 0x40000>,
#endif
	types<tag_matching_rc, 0x40>,
	types<tag_matching_rc, 0x2000>,
	types<tag_matching_rc, 0x40000>
> tm_cq_list;

template <typename T>
struct tag_matching : public testing::Test, public T::Base {
	tag_matching() : T::Base(SZ()) {}

	int SZ() { return T::size; }

	void eager(int start, int length, uint64_t tag) {
		EXEC(send_qp.send(this->src_mr.sge(start, length), tag, IBV_TMH_EAGER));
		EXEC(send_cq.poll());
		EXEC(srq_cq.poll(1));
	}

	void append(int start, int length, uint64_t tag, int *idx = NULL) {
		EXEC(srq.append(this->dst_mr, start, length, tag, idx));
		EXEC(srq_cq.poll(0));
	}

	void remove(int idx) {
		EXEC(srq.remove(idx));
		EXEC(srq_cq.poll(0));
	}

	void recv(int start, int length) {
		EXEC(srq.unexp(this->dst_mr, start, length));
	}

	virtual void SetUp() {
		T::Base::init();
	}

	virtual void fix_uwq() {
		for(int i = 0; i<33; i++)
			EXEC(recv(0, SZ()));
		T::Base::phase_cnt = 0;
	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());
	}
};

TYPED_TEST_CASE(tag_matching, tm_cq_list);

TYPED_TEST(tag_matching, e0_unexp) {
	CHK_SUT(tag-matching);
	EXEC(srq.arm());
	EXEC(recv(0, this->SZ()));
	EXEC(fix_uwq());
	EXEC(eager(0, this->SZ(), 0x12345));
	EXEC(dst_mr.check(0x10));
}

TYPED_TEST(tag_matching, e1_match) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, this->SZ() - 0x10, 1));
	EXEC(eager(0, this->SZ(), 1));
	EXEC(dst_mr.check(0, 0x10));
}

#ifdef MIKHAIL
TYPED_TEST(tag_matching, u0_short) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, this->SZ(), 1));
	EXEC(send_qp.send(this->src_mr.sge(0, 0x40), 1,
			  IBV_SEND_INLINE));

	EXEC(send_cq.poll());
	EXEC(srq_cq.poll(1));
	//EXEC(dst_mr.dump());
}

TYPED_TEST(tag_matching, u2_rndv) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, this->SZ(), 1));
	EXEC(send_qp.rndv(this->src_mr.sge(0, this->SZ()), 1));
	EXEC(send_cq.poll());
	EXEC(srq_cq.poll(1));
	EXEC(dst_mr.check());
}

TYPED_TEST(tag_matching, u3_rndv_unexp) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(send_qp.rndv(this->src_mr.sge(0, this->SZ()), 1));
	EXEC(send_cq.poll());
	EXEC(srq_cq.poll(1));
	//EXEC(dst_mr.dump());
}

TYPED_TEST(tag_matching, u4_rndv_sw) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, this->SZ()/2, 1));
	EXEC(send_qp.rndv(this->src_mr.sge(0, this->SZ()), 1));
	EXEC(send_cq.poll());
	EXEC(srq_cq.poll(1));
	//EXEC(dst_mr.dump());
}
#endif

TYPED_TEST(tag_matching, e2_matchN) {
	int i, N = std::min(this->SZ()/0x20, 8);
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	for (i=0; i<N; i++)
		EXEC(append(this->SZ()/N*i, this->SZ()/N, i));
	EXEC(append(0, this->SZ(), 0x333));
	for (i=0; i<N; i++)
		EXEC(eager(this->SZ()/N*i, this->SZ()/N, i));
	EXEC(eager(0, this->SZ(), 0x333));
	EXEC(dst_mr.check(0, 0x10, N));
}

TYPED_TEST(tag_matching, e3_remove) {
	int i3, i4;
	CHK_SUT(tag-matching);

	EXEC(fix_uwq());
	EXEC(append(0, this->SZ()/2, 3, &i3));
	EXEC(append(0, this->SZ()/2, 1));
	EXEC(remove(i3));
	EXEC(append(this->SZ()/2, this->SZ()/2, 4, &i4));
	EXEC(remove(i4));
	EXEC(append(this->SZ()/2, this->SZ()/2, 2));
	EXEC(eager(this->SZ()/2, this->SZ()/2, 2));
	EXEC(eager(0, this->SZ()/2, 1));
	EXEC(dst_mr.check(0, 0x10, 2));
}

TYPED_TEST(tag_matching, e4_repeat) {
	CHK_SUT(tag-matching);

	EXEC(fix_uwq());

	EXEC(recv(0, this->SZ()));
	EXEC(eager(0, this->SZ(), 5));
	EXEC(dst_mr.check(0x10, 0));

	EXEC(append(0, this->SZ(), 5));
	EXEC(eager(0, this->SZ(), 5));
	EXEC(dst_mr.check(0, 0x10));

	EXEC(recv(0, this->SZ()));
	EXEC(eager(0, this->SZ(), 5));
	EXEC(dst_mr.check(0x10, 0));

	EXEC(append(0, this->SZ(), 5));
	EXEC(eager(0, this->SZ(), 5));
	EXEC(dst_mr.check(0, 0x10));

	EXEC(recv(0, this->SZ()));
	EXEC(eager(0, this->SZ(), 5));
	EXEC(dst_mr.check(0x10, 0));
}

#undef N
#define N 17
#define R 15

TYPED_TEST(tag_matching, e5_mix) {
	int i, idx[N], j, s;
	CHK_SUT(tag-matching);

	EXEC(fix_uwq());
	for (j=0; j<R; j++) {
		s = j&1;
		VERBS_INFO("round %d\n", j);
		for (i=s; i<N; i++)
			EXEC(append(0, this->SZ(), i, idx + i));
		for (i=s; i<N; i++)
			if (i&1) {
				EXEC(remove(idx[i]));
				EXEC(recv(0, this->SZ()));
			}
		for (i=s; i<N; i++) {
			EXEC(eager(0, this->SZ(), i));
			if (i&1)
				EXEC(dst_mr.check(0x10, 0));
			else
				EXEC(dst_mr.check(0, 0x10));
		}
	}
}

TYPED_TEST(tag_matching, e6_unexp_inline) {
	ibvt_mr src(*this, this->pd, 0x20),
		dst(*this, this->pd, 0x20);
	CHK_SUT(tag-matching);

	src.fill();
	dst.init();
	EXEC(srq.unexp(dst, 0, 0x20));
	EXEC(fix_uwq());
	EXEC(send_qp.send(src.sge(0x10, 0x10), 0x12345, IBV_TMH_EAGER));
	EXEC(send_cq.poll());
	EXEC(srq_cq.poll(1));
}

TYPED_TEST(tag_matching, e7_no_tag) {
	CHK_SUT(tag-matching);
	EXEC(recv(0, this->SZ()));
	EXEC(fix_uwq());
	EXEC(send_qp.send(this->src_mr.sge(0, this->SZ()), 0, IBV_TMH_NO_TAG));
	EXEC(send_cq.poll());
	EXEC(srq_cq.poll(1));
	EXEC(dst_mr.check(0x10));
}

TYPED_TEST(tag_matching, e8_unexp_long) {
	int i;
	int n = this->SZ() / 0x40;
	if (n > 128)
		SKIP(0);

	CHK_SUT(tag-matching);
	for(i = 0; i < n; i++)
		EXEC(recv(this->SZ() / n * i, this->SZ() / n));
	EXEC(fix_uwq());
	for(i = 0; i < n; i++)
		EXEC(eager(this->SZ() / n * i,
			   this->SZ() / n,
			   1ULL << i));
	EXEC(dst_mr.check(0x10, 0, n));
}

TYPED_TEST(tag_matching, e9_imm) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, this->SZ(), 1));
	EXEC(send_qp.send(this->src_mr.sge(), 1,
			  IBV_TMH_EAGER,
			  IBV_WR_SEND_WITH_IMM));

	EXEC(send_cq.poll());
	EXEC(srq_cq.poll(1));
}

TYPED_TEST(tag_matching, e10_nosge) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());

	EXEC(srq.append(NULL, 1));
	EXEC(srq_cq.poll(0));
	EXEC(send_qp.send(this->src_mr.sge(0, sizeof(struct ibv_tmh)), 1, IBV_TMH_EAGER));
	EXEC(send_cq.poll());
	EXEC(srq_cq.poll(0));
}

TYPED_TEST(tag_matching, r0_unexp) {
	ibvt_mr dst(*this, this->pd, 0x80);

	CHK_SUT(tag-matching);

	dst.init();
	EXEC(srq.unexp(dst, 0, 0x80));
	EXEC(fix_uwq());
	EXEC(send_qp.rndv(this->src_mr.sge(0, this->SZ()), 1));
	EXEC(send_cq.poll());
	EXEC(srq_cq.poll(1));
}

TYPED_TEST(tag_matching, r1_match) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, this->SZ(), 0x1234567890));
	EXEC(rndv(0, this->SZ(), 0x1234567890));
	EXEC(dst_mr.check());

}

#ifdef YONATAN
TYPED_TEST(tag_matching, rX_match) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	for (int i = 0 ; i < 10000 ; i++) {
		EXEC(append(0, this->SZ(), 0x1234567890));
		EXEC(rndv(0, this->SZ(), 0x1234567890));
		EXEC(dst_mr.check());
	}

}
#endif

TYPED_TEST(tag_matching, r2_match2) {
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, this->SZ()/2, 1));
	EXEC(append(this->SZ()/2, this->SZ()/2, 2));
	EXEC(rndv(this->SZ()/2, this->SZ()/2, 2));
	EXEC(rndv(0, this->SZ()/2, 1));
	EXEC(dst_mr.check());
}

TYPED_TEST(tag_matching, r3_remove) {
	int i3, i4;
	CHK_SUT(tag-matching);
	EXEC(fix_uwq());
	EXEC(append(0, this->SZ()/2, 3, &i3));
	EXEC(append(0, this->SZ()/2, 1));
	EXEC(remove(i3));
	EXEC(append(this->SZ()/2, this->SZ()/2, 4, &i4));
	EXEC(remove(i4));
	EXEC(append(this->SZ()/2, this->SZ()/2, 2));
	EXEC(rndv(this->SZ()/2, this->SZ()/2, 2));
	EXEC(rndv(0, this->SZ()/2, 1));
	EXEC(dst_mr.check());
}

TYPED_TEST(tag_matching, s0_append_remove) {
	int i[10];
	CHK_SUT(tag-matching);
	EXEC(srq.append(this->dst_mr, 0, this->SZ(), 0x111, &i[0]));
	EXEC(srq_cq.poll(0));
	EXEC(srq.append(this->dst_mr, 0, this->SZ(), 0x222, &i[1]));
	EXEC(srq_cq.poll(0));
	EXEC(srq.append(this->dst_mr, 0, this->SZ(), 0x333, &i[2]));
	EXEC(srq_cq.poll(0));

	EXEC(srq.remove(i[2]));
	EXEC(srq_cq.poll(0));
	EXEC(srq.remove(i[1]));
	EXEC(srq_cq.poll(0));
	EXEC(srq.remove(i[0]));
	EXEC(srq_cq.poll(0));

	EXEC(recv(0, this->SZ()));
	EXEC(fix_uwq());
	EXEC(eager(0, this->SZ(), 0x12345));
	EXEC(dst_mr.check(0x10));
}

