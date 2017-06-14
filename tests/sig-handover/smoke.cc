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

#define SZ 1024

struct ibvt_qp_sig : public ibvt_qp_rc {
	ibvt_qp_sig(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) :
		ibvt_qp_rc(e, p, c) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr) {
		ibvt_qp_rc::init_attr(attr);
		attr.comp_mask |= IBV_EXP_QP_INIT_ATTR_CREATE_FLAGS;
		attr.exp_create_flags |= IBV_EXP_QP_CREATE_SIGNATURE_EN;
	}
};

struct ibvt_mr_sig : public ibvt_mr {
	ibvt_mr_sig(ibvt_env &e, ibvt_pd &p, size_t s) :
		ibvt_mr(e, p, s) {}

	virtual void init() {
		struct ibv_exp_create_mr_in in = {};
		if (mr)
			return;

		in.pd = pd.pd;
		in.attr.max_klm_list_size = 1;
		in.attr.create_flags = IBV_EXP_MR_SIGNATURE_EN;
		in.attr.exp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
		SET(mr, ibv_exp_create_mr(&in));
	}
};



struct sig_test : public testing::Test, public ibvt_env {
	struct ibvt_ctx ctx;
	struct ibvt_pd pd;
	struct ibvt_cq cq;
	struct ibvt_qp_sig send_qp;
	struct ibvt_qp_sig recv_qp;
	struct ibvt_mr src_mr;
	struct ibvt_mr mid_mr;
	struct ibvt_mr dst_mr;
	struct ibvt_mr_sig insert_mr;
	struct ibvt_mr_sig strip_mr;

	sig_test() :
		ctx(*this, NULL),
		pd(*this, ctx),
		cq(*this, ctx),
		send_qp(*this, pd, cq),
		recv_qp(*this, pd, cq),
		src_mr(*this, pd, 1024),
		mid_mr(*this, pd, 1040),
		dst_mr(*this, pd, 1024),
		insert_mr(*this, pd, 1040),
		strip_mr(*this, pd, 1040)
	{ }

	#define CHECK_REF_TAG 0x0f
	#define CHECK_APP_TAG 0x30
	#define CHECK_GUARD   0xc0

	virtual void config(ibvt_mr &sig_mr, ibvt_mr &data_mr, int dir) {
		struct ibv_sge sge = data_mr.sge();
		struct ibv_send_wr wr = {};
		struct ibv_send_wr *bad_wr;
		struct ibv_exp_sig_attrs sig = {};

		sig.check_mask |= CHECK_REF_TAG;
		sig.check_mask |= CHECK_APP_TAG;
		sig.check_mask |= CHECK_GUARD;
		if (dir) {
			sig.mem.sig_type = IBV_EXP_SIG_TYPE_T10_DIF;
			sig.mem.sig.dif.bg_type = IBV_EXP_T10DIF_CRC;
			sig.mem.sig.dif.pi_interval = 512;
			sig.mem.sig.dif.bg = 0x1234;
			sig.mem.sig.dif.app_tag = 0x5678;
			sig.mem.sig.dif.ref_tag = 0xabcdef90;
			sig.mem.sig.dif.ref_remap = 1;
			sig.mem.sig.dif.app_escape = 1;
			sig.mem.sig.dif.ref_escape = 1;
			sig.mem.sig.dif.apptag_check_mask = 0xffff;
		} else {
			sig.mem.sig_type = IBV_EXP_SIG_TYPE_NONE;
		}

		if (dir) {
			sig.wire.sig_type = IBV_EXP_SIG_TYPE_NONE;
		} else {
			sig.wire.sig_type = IBV_EXP_SIG_TYPE_T10_DIF;
			sig.wire.sig.dif.bg_type = IBV_EXP_T10DIF_CRC;
			sig.wire.sig.dif.pi_interval = 512;
			sig.wire.sig.dif.bg = 0x1234;
			sig.wire.sig.dif.app_tag = 0x5678;
			sig.wire.sig.dif.ref_tag = 0xabcdef90;
			sig.wire.sig.dif.ref_remap = 1;
			sig.wire.sig.dif.app_escape = 1;
			sig.wire.sig.dif.ref_escape = 1;
			sig.wire.sig.dif.apptag_check_mask = 0xffff;
		}

		wr.exp_opcode = IBV_EXP_WR_REG_SIG_MR;
		wr.exp_send_flags = IBV_EXP_SEND_SIGNALED;
		wr.ext_op.sig_handover.sig_attrs = &sig;
		wr.ext_op.sig_handover.sig_mr = sig_mr.mr;
		wr.ext_op.sig_handover.access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
		wr.ext_op.sig_handover.prot = NULL;

		wr.num_sge = 1;
		wr.sg_list = &sge;

		DO(ibv_post_send(send_qp.qp, &wr, &bad_wr));
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

	virtual void SetUp() {
		INIT(ctx.init());
		if (skip)
			return;
		INIT(send_qp.init());
		INIT(recv_qp.init());
		INIT(send_qp.connect(&recv_qp));
		INIT(recv_qp.connect(&send_qp));
		INIT(insert_mr.init());
		INIT(strip_mr.init());
		INIT(src_mr.fill());
		INIT(mid_mr.init());
		INIT(dst_mr.init());
		INIT(cq.arm());
	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());
	}
};

TEST_F(sig_test, t0) {
	CHK_SUT(sig_handover);
	EXEC(config(this->insert_mr, this->src_mr, 0));
	EXEC(cq.poll(1));
	EXEC(config(this->strip_mr, this->mid_mr, 1));
	EXEC(cq.poll(1));
	EXEC(send_qp.rdma(this->insert_mr.sge(), this->mid_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll(1));
	EXEC(recv_qp.rdma(this->dst_mr.sge(), this->strip_mr.sge(), IBV_WR_RDMA_READ));
	EXEC(cq.poll(1));
	EXEC(mr_status(this->strip_mr, 0));
	EXEC(dst_mr.check());
}

TEST_F(sig_test, t1) {
	CHK_SUT(sig_handover);
	EXEC(config(this->strip_mr, this->mid_mr, 1));
	EXEC(cq.poll(1));
	EXEC(recv_qp.rdma(this->dst_mr.sge(), this->strip_mr.sge(), IBV_WR_RDMA_READ));
	EXEC(cq.poll(1));
	EXEC(mr_status(this->strip_mr, 1));
}


