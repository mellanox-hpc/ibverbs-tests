/**
 * Copyright (C) 2017      Mellanox Technologies Ltd. All rights reserved.
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

#include <infiniband/verbs_exp.h>

#include "env.h"

#if 1
#define BBB 16
#else
#define BBB 1
#endif

#define SZ (512*BBB)
#define SZD(pi_size) ((512+pi_size)*BBB)

#define SZ_p(n) (512*((n)%BBB+1))
#define SZ_pp(n,from,spare) (from+512*((n)%(BBB-spare-from/512)+1))

struct ibvt_qp_sig : public ibvt_qp_rc {
	ibvt_qp_sig(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) :
		ibvt_qp_rc(e, p, c) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr) {
		ibvt_qp_rc::init_attr(attr);
		attr.comp_mask |= IBV_EXP_QP_INIT_ATTR_CREATE_FLAGS;
		attr.exp_create_flags |= IBV_EXP_QP_CREATE_SIGNATURE_EN;
		attr.cap.max_send_wr = 0x355;
		attr.exp_create_flags |= IBV_EXP_QP_CREATE_UMR;
		attr.comp_mask |= IBV_EXP_QP_INIT_ATTR_MAX_INL_KLMS;
		attr.max_inl_send_klms = 3;
	}

	virtual void send_2wr(ibv_sge sge, ibv_sge sge2) {
		struct ibv_send_wr wr = {};
		struct ibv_send_wr wr2 = {};
		struct ibv_send_wr *bad_wr = NULL;

		wr.next = &wr2;
		wr.sg_list = &sge;
		wr.num_sge = 1;
		wr._wr_opcode = IBV_WR_SEND;

		wr2.sg_list = &sge2;
		wr2.num_sge = 1;
		wr2._wr_opcode = IBV_WR_SEND;
		wr2._wr_send_flags = IBV_EXP_SEND_SIGNALED |
				     IBV_EXP_SEND_SIG_PIPELINED;

		DO(ibv_post_send(qp, &wr, &bad_wr));
	}

	virtual void send_2wr_m(ibv_sge sge[], int sge_n, ibv_sge sge2) {
		struct ibv_send_wr wr = {};
		struct ibv_send_wr wr2 = {};
		struct ibv_send_wr *bad_wr = NULL;

		wr.next = &wr2;
		wr.sg_list = sge;
		wr.num_sge = sge_n;
		wr._wr_opcode = IBV_WR_SEND;

		wr2.sg_list = &sge2;
		wr2.num_sge = 1;
		wr2._wr_opcode = IBV_WR_SEND;
		wr2._wr_send_flags = IBV_EXP_SEND_SIGNALED |
				     IBV_EXP_SEND_SIG_PIPELINED;

		DO(ibv_post_send(qp, &wr, &bad_wr));
	}
};

struct ibvt_qp_sig_pipeline : public ibvt_qp_sig {
	ibvt_qp_sig_pipeline(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) :
		ibvt_qp_sig(e, p, c) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr) {
		ibvt_qp_sig::init_attr(attr);
		attr.exp_create_flags |= IBV_EXP_QP_CREATE_SIGNATURE_PIPELINE;
	}

};

struct ibvt_mr_sig : public ibvt_mr {
	ibvt_mr_sig(ibvt_env &e, ibvt_pd &p) :
		ibvt_mr(e, p, 0) {}

	virtual void init() {
		struct ibv_exp_create_mr_in in = {};
		if (mr)
			return;

		in.pd = pd.pd;
		in.attr.max_klm_list_size = 1;
		in.attr.create_flags = IBV_EXP_MR_SIGNATURE_EN;
		in.attr.exp_access_flags = IBV_ACCESS_LOCAL_WRITE |
					   IBV_ACCESS_REMOTE_READ |
					   IBV_ACCESS_REMOTE_WRITE;
		SET(mr, ibv_exp_create_mr(&in));
	}
};

struct sig_domain {
	virtual struct ibv_exp_sig_domain nosig() {
		struct ibv_exp_sig_domain sd = {};

		sd.sig_type = IBV_EXP_SIG_TYPE_NONE;

		return sd;
	}

	virtual struct ibv_exp_sig_domain sig() = 0;
	virtual uint16_t check_mask() = 0;
};

struct sig_t10dif : public sig_domain {
	static unsigned pi_size() {
		return 8;
	}

	enum {
		CHECK_REF_TAG	= 0x0f,
		CHECK_APP_TAG	= 0x30,
		CHECK_GUARD	= 0xc0,
	};

	virtual uint16_t check_mask() {
		return CHECK_APP_TAG | CHECK_GUARD;
	}

	virtual struct ibv_exp_sig_domain sig() {
		struct ibv_exp_sig_domain sd = {};

		sd.sig_type = IBV_EXP_SIG_TYPE_T10_DIF;
		sd.sig.dif.bg_type = IBV_EXP_T10DIF_CRC;
		sd.sig.dif.pi_interval = 512;
		sd.sig.dif.bg = 0x1234;
		sd.sig.dif.app_tag = 0x5678;
		sd.sig.dif.ref_tag = 0xabcdef90;
		sd.sig.dif.ref_remap = 1;
		sd.sig.dif.app_escape = 1;
		sd.sig.dif.ref_escape = 1;
		sd.sig.dif.apptag_check_mask = 0xffff;

		return sd;
	}
};

struct sig_crc32 : public sig_domain {
	static unsigned pi_size() {
		return 4;
	}

	enum {
		CHECK_GUARD = 0xff,
	};

	virtual uint16_t check_mask() {
		return CHECK_GUARD;
	}

	virtual struct ibv_exp_sig_domain sig() {
		struct ibv_exp_sig_domain sd = {};

		sd.sig_type = IBV_EXP_SIG_TYPE_CRC32;
		sd.sig.crc.pi_interval = 512;
		sd.sig.crc.bg = 0xffffffff;

		return sd;
	}
};

template <typename QP, typename SD>
struct sig_test_base : public testing::Test, public SD, public ibvt_env {
	ibvt_ctx ctx;
	ibvt_pd pd;
	ibvt_cq cq;
	QP send_qp;
	QP recv_qp;
	ibvt_mr src_mr;
	ibvt_mr src2_mr;
	ibvt_mr mid_mr;
	ibvt_mr mid2_mr;
	ibvt_mr mid_mr_x2;
	ibvt_mr dst_mr;
	ibvt_mr dst_mr_x2;
	ibvt_mr_sig insert_mr;
	ibvt_mr_sig insert2_mr;
	ibvt_mr_sig check_mr;
	ibvt_mr_sig strip_mr;
	ibvt_mr_sig strip_mr_x2;
	ibvt_mw mw;

	sig_test_base() :
		ctx(*this, NULL),
		pd(*this, ctx),
		cq(*this, ctx),
		send_qp(*this, pd, cq),
		recv_qp(*this, pd, cq),
		src_mr(*this, pd, SZ),
		src2_mr(*this, pd, SZ),
		mid_mr(*this, pd, SZD(this->pi_size())),
		mid2_mr(*this, pd, SZD(this->pi_size())),
		mid_mr_x2(*this, pd, SZD(this->pi_size()) * 2),
		dst_mr(*this, pd, SZ),
		dst_mr_x2(*this, pd, SZ * 2),
		insert_mr(*this, pd),
		insert2_mr(*this, pd),
		check_mr(*this, pd),
		strip_mr(*this, pd),
		strip_mr_x2(*this, pd),
		mw(mid_mr, 0, SZD(this->pi_size()), send_qp)
	{ }

	virtual void config(ibvt_mr &sig_mr, struct ibv_sge data,
			    struct ibv_exp_sig_domain mem,
			    struct ibv_exp_sig_domain wire) {
		struct ibv_send_wr wr = {};
		struct ibv_send_wr *bad_wr;
		struct ibv_exp_sig_attrs sig = {};

		sig.check_mask = this->check_mask();
		sig.mem = mem;
		sig.wire = wire;

		wr.exp_opcode = IBV_EXP_WR_REG_SIG_MR;
		wr.exp_send_flags = IBV_EXP_SEND_SIGNALED;
		wr.ext_op.sig_handover.sig_attrs = &sig;
		wr.ext_op.sig_handover.sig_mr = sig_mr.mr;
		wr.ext_op.sig_handover.access_flags = IBV_ACCESS_LOCAL_WRITE |
						      IBV_ACCESS_REMOTE_READ |
						      IBV_ACCESS_REMOTE_WRITE;
		wr.ext_op.sig_handover.prot = NULL;

		wr.num_sge = 1;
		wr.sg_list = &data;

		DO(ibv_post_send(send_qp.qp, &wr, &bad_wr));
		EXEC(cq.poll());
	}

	virtual void linv(ibvt_mr &sig_mr) {
		struct ibv_send_wr wr = {};
		struct ibv_send_wr *bad_wr;

		wr.exp_opcode = IBV_EXP_WR_LOCAL_INV;
		wr.exp_send_flags = IBV_EXP_SEND_SIGNALED;
		wr.ex.invalidate_rkey = sig_mr.mr->rkey;
		DO(ibv_post_send(send_qp.qp, &wr, &bad_wr));
		EXEC(cq.poll());
	}

	void mr_status(ibvt_mr &mr, int expected) {
		struct ibv_exp_mr_status status;

		DO(ibv_exp_check_mr_status(mr.mr, IBV_EXP_MR_CHECK_SIG_STATUS,
					   &status));
		VERBS_INFO("SEGERR %d %x %x %lx\n",
			   status.sig_err.err_type,
			   status.sig_err.expected,
			   status.sig_err.actual,
			   status.sig_err.sig_err_offset);
		ASSERT_EQ(expected, status.fail_status);
	}

	void ae() {
		struct ibv_async_event event;

		DO(ibv_get_async_event(this->ctx.ctx, &event));
		ibv_ack_async_event(&event);
	}

	virtual void SetUp() {
		INIT(ctx.init());
		if (skip)
			return;
		INIT(send_qp.init());
		INIT(recv_qp.init());
		INIT(send_qp.connect(&recv_qp));
		INIT(recv_qp.connect(&send_qp));
		INIT(src_mr.fill());
		INIT(src2_mr.fill());
		INIT(mid_mr.init());
		INIT(mid2_mr.init());
		INIT(mid_mr_x2.init());
		INIT(dst_mr.init());
		INIT(dst_mr_x2.init());
		INIT(insert_mr.init());
		INIT(insert2_mr.init());
		INIT(check_mr.init());
		INIT(strip_mr.init());
		INIT(strip_mr_x2.init());
		INIT(cq.arm());
		INIT(mw.init());
	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());
	}
};

template <typename SD>
struct sig_test : public sig_test_base<ibvt_qp_sig, SD> {};
typedef testing::Types<sig_t10dif, sig_crc32> sig_domain_types;
TYPED_TEST_CASE(sig_test, sig_domain_types);

typedef sig_test<sig_t10dif> sig_test_t10diff;
typedef sig_test_base<ibvt_qp_sig_pipeline, sig_t10dif> sig_test_pipeline;

TYPED_TEST(sig_test, c0) {
	CHK_SUT(sig_handover);
	EXEC(config(this->insert_mr, this->src_mr.sge(), this->nosig(), this->sig()));
	EXEC(config(this->strip_mr, this->mid_mr.sge(), this->sig(), this->nosig()));
	EXEC(send_qp.rdma(this->mid_mr.sge(), this->insert_mr.sge(0,SZD(this->pi_size())), IBV_WR_RDMA_READ));
	EXEC(cq.poll());
	EXEC(send_qp.rdma(this->dst_mr.sge(), this->strip_mr.sge(0,SZ), IBV_WR_RDMA_READ));
	EXEC(cq.poll());
	EXEC(mr_status(this->strip_mr, 0));
	EXEC(dst_mr.check());
}

TYPED_TEST(sig_test, c1) {
	CHK_SUT(sig_handover);
	EXEC(config(this->insert_mr, this->src_mr.sge(), this->nosig(), this->sig()));
	EXEC(config(this->strip_mr, this->mid_mr.sge(), this->sig(), this->nosig()));
	EXEC(send_qp.rdma(this->insert_mr.sge(0,SZD(this->pi_size())), this->mid_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(send_qp.rdma(this->strip_mr.sge(0,SZ), this->dst_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(mr_status(this->strip_mr, 0));
	EXEC(dst_mr.check());
}

TYPED_TEST(sig_test, c2) {
	CHK_SUT(sig_handover);
	EXEC(config(this->insert_mr, this->mid_mr.sge(), this->sig(), this->nosig()));
	EXEC(config(this->strip_mr, this->dst_mr.sge(), this->nosig(), this->sig()));
	EXEC(send_qp.rdma(this->insert_mr.sge(0,SZ), this->src_mr.sge(), IBV_WR_RDMA_READ));
	EXEC(cq.poll());
	EXEC(send_qp.rdma(this->strip_mr.sge(0,SZD(this->pi_size())), this->mid_mr.sge(), IBV_WR_RDMA_READ));
	EXEC(cq.poll());
	EXEC(mr_status(this->strip_mr, 0));
	EXEC(dst_mr.check());
}

TYPED_TEST(sig_test, c3) {
	CHK_SUT(sig_handover);
	EXEC(config(this->insert_mr, this->mid_mr.sge(), this->sig(), this->nosig()));
	EXEC(config(this->strip_mr, this->dst_mr.sge(), this->nosig(), this->sig()));
	EXEC(send_qp.rdma(this->src_mr.sge(), this->insert_mr.sge(0,SZ), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(send_qp.rdma(this->mid_mr.sge(), this->strip_mr.sge(0,SZD(this->pi_size())), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(mr_status(this->strip_mr, 0));
	EXEC(dst_mr.check());
}

TYPED_TEST(sig_test, c4) {
	CHK_SUT(sig_handover);
	EXEC(config(this->insert_mr, this->src_mr.sge(), this->nosig(), this->sig()));
	EXEC(config(this->check_mr, this->mid_mr.sge(), this->sig(), this->sig()));
	EXEC(config(this->strip_mr, this->mid2_mr.sge(), this->sig(), this->nosig()));
	EXEC(send_qp.rdma(this->insert_mr.sge(0,SZD(this->pi_size())), this->mid_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(send_qp.rdma(this->check_mr.sge(0,SZD(this->pi_size())), this->mid2_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(send_qp.rdma(this->strip_mr.sge(0,SZ), this->dst_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(mr_status(this->strip_mr, 0));
	EXEC(dst_mr.check());
}

TYPED_TEST(sig_test, r0) {
	CHK_SUT(sig_handover);
	EXEC(config(this->insert_mr, this->src_mr.sge(), this->nosig(), this->sig()));
	EXEC(config(this->check_mr, this->mid_mr.sge(), this->sig(), this->sig()));
	struct ibv_exp_sig_domain sd = this->sig();
	sd.sig.dif.ref_remap = 0;
	EXEC(config(this->strip_mr, this->mid2_mr.sge(), sd, this->nosig()));

	EXEC(send_qp.rdma(this->insert_mr.sge(0,SZD(this->pi_size())), this->mid_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(send_qp.rdma(this->check_mr.sge(0,SZD(this->pi_size())), this->mid2_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());

	for (long i = 0; i < 10; i++) {
		EXEC(send_qp.rdma(this->strip_mr.sge(0,SZ), this->dst_mr.sge(), IBV_WR_RDMA_WRITE));
		EXEC(cq.poll());
		EXEC(mr_status(this->strip_mr, 0));
		EXEC(dst_mr.check());
	}
}

TYPED_TEST(sig_test, r1) {
	CHK_SUT(sig_handover);
	EXEC(config(this->insert_mr, this->src_mr.sge(), this->nosig(), this->sig()));
	EXEC(config(this->check_mr, this->mid_mr.sge(), this->sig(), this->sig()));

	EXEC(send_qp.rdma(this->insert_mr.sge(0,SZD(this->pi_size())), this->mid_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(send_qp.rdma(this->check_mr.sge(0,SZD(this->pi_size())), this->mid2_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());

	for (long i = 0; i < 100; i++) {
		EXEC(config(this->strip_mr, this->mid2_mr.sge(), this->sig(), this->nosig()));
		EXEC(send_qp.rdma(this->strip_mr.sge(0,SZ), this->dst_mr.sge(), IBV_WR_RDMA_WRITE));
		EXEC(cq.poll());
		EXEC(mr_status(this->strip_mr, 0));
		EXEC(dst_mr.check());
		EXEC(linv(this->strip_mr));
	}
	getchar();
}

TYPED_TEST(sig_test, r2) {
	CHK_SUT(sig_handover);
	EXEC(config(this->insert_mr, this->src_mr.sge(), this->nosig(), this->sig()));
	EXEC(config(this->check_mr, this->mid_mr.sge(), this->sig(), this->sig()));

	EXEC(send_qp.rdma(this->insert_mr.sge(0,SZD(this->pi_size())), this->mid_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(send_qp.rdma(this->check_mr.sge(0,SZD(this->pi_size())), this->mid2_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());

	for (long i = 0; i < 100; i++) {
		EXEC(config(this->strip_mr, this->mid2_mr.sge(), this->sig(), this->nosig()));
		EXEC(send_qp.rdma(this->strip_mr.sge(0,SZ_p(i)), this->dst_mr.sge(), IBV_WR_RDMA_WRITE));
		EXEC(cq.poll());
		EXEC(mr_status(this->strip_mr, 0));
		EXEC(dst_mr.check(0,0,1,SZ_p(i)));
		EXEC(linv(this->strip_mr));
	}
	getchar();
}

TYPED_TEST(sig_test, r3) {
	CHK_SUT(sig_handover);
	EXEC(config(this->insert_mr, this->src_mr.sge(), this->nosig(), this->sig()));
	EXEC(config(this->check_mr, this->mid_mr.sge(), this->sig(), this->sig()));

	EXEC(send_qp.rdma(this->insert_mr.sge(0,SZD(this->pi_size())), this->mid_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(send_qp.rdma(this->check_mr.sge(0,SZD(this->pi_size())), this->mid2_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());

	for (long i = 0; i < 100; i++) {
		int j = SZ_pp(i,0,3);
		int k = SZ_pp(i,j,2);
		int l = SZ_pp(i,k,1);
		EXEC(config(this->strip_mr, this->mid2_mr.sge(), this->sig(), this->nosig()));
		EXEC(config(this->strip_mr_x2, this->mid2_mr.sge(), this->sig(), this->nosig()));
		EXEC(send_qp.rdma(this->strip_mr.sge(0,j), this->dst_mr.sge(), IBV_WR_RDMA_WRITE, (enum ibv_send_flags)0));
		EXEC(send_qp.rdma(this->strip_mr_x2.sge(j,k-j), this->dst_mr.sge(j,0), IBV_WR_RDMA_WRITE, (enum ibv_send_flags)0));
		EXEC(send_qp.rdma(this->strip_mr.sge(k,l-k), this->dst_mr.sge(k,0), IBV_WR_RDMA_WRITE, (enum ibv_send_flags)0));
		EXEC(send_qp.rdma(this->strip_mr_x2.sge(l,SZ-l), this->dst_mr.sge(l,0), IBV_WR_RDMA_WRITE));
		EXEC(cq.poll());
		EXEC(mr_status(this->strip_mr, 0));
		EXEC(mr_status(this->strip_mr_x2, 0));
		EXEC(dst_mr.check(0,0,1,SZ));
		EXEC(linv(this->strip_mr));
		EXEC(linv(this->strip_mr_x2));
	}
	getchar();
}

TYPED_TEST(sig_test, r4) {
	CHK_SUT(sig_handover);
	struct ibv_exp_sig_domain sd = this->sig();
	enum ibv_send_flags nof = (enum ibv_send_flags)0;

	for (long i = 0; i < 100; i++) {
		int j = SZ_pp(i,0,3);
		int k = SZ_pp(i,j,2);
		int l = SZ_pp(i,k,1);
		sd.sig.dif.ref_tag = 0x28f5b5d * i;

		EXEC(config(this->insert_mr, this->src_mr.sge(), this->nosig(), sd));
		EXEC(send_qp.rdma(this->insert_mr.sge(0,SZD(this->pi_size())), this->mid_mr.sge(), IBV_WR_RDMA_WRITE));
		EXEC(cq.poll());

		EXEC(config(this->strip_mr, this->mw.sge(), sd, this->nosig()));
		EXEC(config(this->strip_mr_x2, this->mw.sge(), sd, this->nosig()));
		EXEC(send_qp.rdma(this->strip_mr.sge(0,j), this->dst_mr.sge(), IBV_WR_RDMA_WRITE, nof));
		EXEC(send_qp.rdma(this->strip_mr_x2.sge(j,k-j), this->dst_mr.sge(j,0), IBV_WR_RDMA_WRITE, nof));
		EXEC(send_qp.rdma(this->strip_mr.sge(k,l-k), this->dst_mr.sge(k,0), IBV_WR_RDMA_WRITE, nof));
		EXEC(send_qp.rdma(this->strip_mr_x2.sge(l,SZ-l), this->dst_mr.sge(l,0), IBV_WR_RDMA_WRITE));
		EXEC(cq.poll());
		EXEC(mr_status(this->insert_mr, 0));
		EXEC(mr_status(this->strip_mr, 0));
		EXEC(mr_status(this->strip_mr_x2, 0));
		EXEC(dst_mr.check(0,0,1,SZ));

		EXEC(linv(this->insert_mr));
		EXEC(linv(this->strip_mr));
		EXEC(linv(this->strip_mr_x2));
	}
	getchar();
}

TYPED_TEST(sig_test, c6) {
	CHK_SUT(sig_handover);
	struct ibv_exp_sig_domain sd = this->sig();
	sd.sig.dif.ref_remap = 0;
	EXEC(config(this->insert_mr, this->src_mr.sge(), this->nosig(), this->sig()));
	EXEC(config(this->insert2_mr, this->src2_mr.sge(), this->nosig(), this->sig()));
	EXEC(config(this->strip_mr_x2, this->mid_mr_x2.sge(), sd, this->nosig()));
	EXEC(send_qp.rdma2(this->insert_mr.sge(0,SZD(this->pi_size())),
			   this->insert2_mr.sge(0,SZD(this->pi_size())),
			   this->mid_mr_x2.sge(),
			   IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(send_qp.rdma(this->strip_mr_x2.sge(0,SZ * 2),
			  this->dst_mr_x2.sge(),
			  IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(mr_status(this->strip_mr_x2, 0));
	EXEC(dst_mr_x2.check());
}

TYPED_TEST(sig_test, e0) {
	CHK_SUT(sig_handover);
	EXEC(config(this->strip_mr, this->mid_mr.sge(), this->sig(), this->nosig()));
	EXEC(send_qp.rdma(this->dst_mr.sge(), this->strip_mr.sge(0,SZ), IBV_WR_RDMA_READ));
	EXEC(cq.poll());
	EXEC(mr_status(this->strip_mr, 1));
}

TYPED_TEST(sig_test, e1) {
	CHK_SUT(sig_handover);
	EXEC(config(this->strip_mr, this->mid_mr.sge(), this->sig(), this->nosig()));
	EXEC(send_qp.rdma(this->strip_mr.sge(0,SZ), this->dst_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(mr_status(this->strip_mr, 1));
}

TYPED_TEST(sig_test, e2) {
	CHK_SUT(sig_handover);
	EXEC(config(this->strip_mr, this->dst_mr.sge(), this->nosig(), this->sig()));
	EXEC(send_qp.rdma(this->strip_mr.sge(0,SZD(this->pi_size())), this->mid_mr.sge(), IBV_WR_RDMA_READ));
	EXEC(cq.poll());
	EXEC(mr_status(this->strip_mr, 1));
}

TYPED_TEST(sig_test, e3) {
	CHK_SUT(sig_handover);
	EXEC(config(this->strip_mr, this->dst_mr.sge(), this->nosig(), this->sig()));
	EXEC(send_qp.rdma(this->mid_mr.sge(), this->strip_mr.sge(0,SZD(this->pi_size())), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(mr_status(this->strip_mr, 1));
}

TYPED_TEST(sig_test, e4) {
	CHK_SUT(sig_handover);
	EXEC(config(this->check_mr, this->mid_mr.sge(), this->sig(), this->sig()));
	EXEC(send_qp.rdma(this->check_mr.sge(0,SZD(this->pi_size())), this->mid2_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(mr_status(this->check_mr, 1));
}

TEST_F(sig_test_pipeline, p0) {
	CHK_SUT(sig_handover);
	EXEC(config(this->strip_mr, this->mid_mr.sge(), this->sig(), this->nosig()));
	EXEC(recv_qp.recv(this->dst_mr.sge()));
	EXEC(recv_qp.recv(this->dst_mr.sge()));
	EXEC(send_qp.send_2wr(this->strip_mr.sge(0,SZ), this->src_mr.sge(0,SZ)));
	cq.poll();
	EXEC(ae());
	EXEC(cq.poll());
	EXEC(mr_status(this->strip_mr, 1));
}

TEST_F(sig_test_pipeline, p1) {
	CHK_SUT(sig_handover);
	EXEC(config(this->strip_mr, this->mid_mr.sge(), this->sig(), this->nosig()));
	EXEC(recv_qp.recv(this->dst_mr.sge()));
	EXEC(recv_qp.recv(this->dst_mr.sge()));
	EXEC(send_qp.send_2wr(this->strip_mr.sge(0,SZ), this->src_mr.sge()));
	cq.poll();
	EXEC(mr_status(this->strip_mr, 1));
}

TEST_F(sig_test_pipeline, p2) {
	CHK_SUT(sig_handover);
	EXEC(config(this->strip_mr, this->mid_mr.sge(), this->sig(), this->nosig()));
	EXEC(recv_qp.recv(this->dst_mr.sge()));
	EXEC(recv_qp.recv(this->dst_mr.sge()));
	struct ibv_sge sge[4];
	for (int i=0; i<4; i++)
		sge[i] = this->strip_mr.sge(0 + i * SZ / 4, SZ/4);
	EXEC(send_qp.send_2wr_m(sge, 4, this->src_mr.sge(0,SZ)));
	cq.poll();
	EXEC(ae());
	EXEC(cq.poll());
	EXEC(mr_status(this->strip_mr, 1));
}


TEST_F(sig_test_pipeline, p3) {
	CHK_SUT(sig_handover);
	for (long i = 0; i < 100; i++) {
		EXEC(config(this->strip_mr, this->mid_mr.sge(), this->sig(), this->nosig()));
		EXEC(recv_qp.recv(this->dst_mr.sge()));
		EXEC(recv_qp.recv(this->dst_mr.sge()));
		struct ibv_sge sge[4];
		for (int i=0; i<4; i++)
			sge[i] = this->strip_mr.sge(0 + i * SZ / 4, SZ/4);
		EXEC(send_qp.send_2wr_m(sge, 4, this->src_mr.sge(0,SZ)));
		cq.poll();
		EXEC(ae());
		EXEC(cq.poll());
		EXEC(mr_status(this->strip_mr, 1));
		EXEC(linv(this->strip_mr));
	}
}

TEST_F(sig_test_pipeline, p4) {
	CHK_SUT(sig_handover);
	EXEC(config(this->strip_mr, this->mid_mr.sge(), this->sig(), this->nosig()));
	for (long i = 0; i < 100; i++) {
		EXEC(recv_qp.recv(this->dst_mr.sge()));
		EXEC(recv_qp.recv(this->dst_mr.sge()));
		EXEC(send_qp.send_2wr(this->strip_mr.sge(0,SZ), this->src_mr.sge()));
	}
	getchar();
}

#include"tests/sig-handover/pillar.cc"
