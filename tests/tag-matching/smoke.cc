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

#define SZ 1024
//#define SZ 128

struct ibvt_srq : public ibvt_obj {
	struct ibv_srq *srq;

	ibvt_pd &pd;
	ibvt_cq &cq;

	ibvt_srq(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) :
		 ibvt_obj(e), pd(p), cq(c) {}

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
		attr.tm_list_size = 16;
		attr.attr.max_wr  = 16;
		attr.attr.max_sge = 1;

		SET(srq, ibv_create_srq_ex(pd.ctx.ctx, &attr));
	}

	virtual void recv(ibvt_mr &mr, int start, int length) {
		struct ibv_recv_wr wr;
		struct ibv_sge sge = mr.sge(start, length);
		struct ibv_recv_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0;
		wr.sg_list = &sge;
		wr.num_sge = 1;
		DO(ibv_post_srq_recv(srq, &wr, &bad_wr));
	}
};

struct ibvt_qp_tm : public ibvt_qp_rc {
	ibvt_qp_tm(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) :
		    ibvt_qp_rc(e, p, c) {}

	virtual void send(ibvt_mr &mr, int start, int length,
			  uint64_t tag, enum ibv_wr_opcode op)
	{
		struct ibv_send_wr wr;
		struct ibv_sge sge = mr.sge(start, length);
		struct ibv_send_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.sg_list = &sge;
		wr.num_sge = 1;
		wr.send_flags = IBV_SEND_SIGNALED;
		wr.opcode = op;

		wr.tm_match.match.bits_64  = tag;

		DO(ibv_post_send(qp, &wr, &bad_wr));
	}
};

struct ibvt_qp_srq : public ibvt_qp_tm {
	ibvt_srq &srq;

	ibvt_qp_srq(ibvt_env &e, ibvt_pd &p, ibvt_cq &c, ibvt_srq &s) :
		    ibvt_qp_tm(e, p, c), srq(s) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr)
	{
		ibvt_qp_rc::init_attr(attr);
		attr.srq = srq.srq;
	}

	virtual void append(ibvt_mr &mr, int start, int length,
			    uint64_t tag, int *idx)
	{
		struct ibv_send_wr wr;
		struct ibv_sge sge = mr.sge(start, length);
		struct ibv_send_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.sg_list = &sge;
		wr.num_sge = 1;
		wr.send_flags = IBV_SEND_SIGNALED;
		wr.opcode = IBV_WR_TM_APPEND;

		wr.tm_tag.match.bits_64 = tag;
		wr.tm_tag.mask.bits_64	= 0xffffffffffffffff;

		DO(ibv_post_send(qp, &wr, &bad_wr));

		if (idx)
			*idx = wr.tm_tag.tag_idx;
	}

	virtual void remove(int idx)
	{
		struct ibv_send_wr wr;
		struct ibv_send_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.send_flags = IBV_SEND_SIGNALED;
		wr.opcode = IBV_WR_TM_REMOVE;
		wr.tm_tag.tag_idx = idx;

		DO(ibv_post_send(qp, &wr, &bad_wr));
	}
};

struct tag_matching : public testing::Test, public ibvt_env {
	struct ibvt_ctx ctx;
	struct ibvt_pd pd;
	struct ibvt_cq srq_cq;
	struct ibvt_srq srq;
	struct ibvt_cq ctrl_cq;
	struct ibvt_qp_srq ctrl_qp;
	struct ibvt_cq send_cq;
	struct ibvt_qp_tm send_qp;
	struct ibvt_cq recv_cq;
	struct ibvt_qp_srq recv_qp;
	struct ibvt_mr_hdr src_mr;
	struct ibvt_mr_hdr dst_mr;

	tag_matching() :
		ctx(*this, NULL),
		pd(*this, ctx),
		srq_cq(*this, ctx),
		srq(*this, pd, srq_cq),
		ctrl_cq(*this, ctx),
		ctrl_qp(*this, pd, ctrl_cq, srq),
		send_cq(*this, ctx),
		send_qp(*this, pd, send_cq),
		recv_cq(*this, ctx),
		recv_qp(*this, pd, recv_cq, srq),
		src_mr(*this, pd, SZ, 0x10),
		dst_mr(*this, pd, SZ, 0x10)
	{ }

	void eager(int start, int length, uint64_t tag) {
		EXEC(send_qp.send(src_mr, start, length, tag,
				  IBV_WR_TAG_SEND_EAGER));

		EXEC(send_cq.poll(1));
		EXEC(srq_cq.poll(2));
	}

	void rndv(int start, int length, uint64_t tag) {
		ibvt_mr fin(*this, pd, 0x20);
		fin.init();
		EXEC(send_qp.recv(fin, 0, 0x20));
		EXEC(send_qp.send(src_mr, start, length, tag,
				  IBV_WR_TAG_SEND_RNDV));
		EXEC(srq_cq.poll(2));
		EXEC(send_cq.poll(2));
		//fin.dump();
	}

	void append(int start, int length, uint64_t tag, int *idx = NULL) {
		EXEC(ctrl_qp.append(dst_mr, start, length, tag, idx));
		EXEC(srq_cq.poll(1));
	}

	void remove(int idx) {
		EXEC(ctrl_qp.remove(idx));
		EXEC(srq_cq.poll(1));
	}

	void recv(int start, int length) {
		EXEC(srq.recv(dst_mr, start, length));
	}

	virtual void SetUp() {
		ASSERT_FALSE(fatality);
		EXEC(srq.init());
		EXEC(ctrl_qp.init());
		EXEC(ctrl_qp.connect(&ctrl_qp));
		EXEC(send_qp.init());
		EXEC(recv_qp.init());
		EXEC(send_qp.connect(&recv_qp));
		EXEC(recv_qp.connect(&send_qp));
		EXEC(src_mr.fill());
		EXEC(dst_mr.init());
	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());
	}
};

TEST_F(tag_matching, e0_unexp) {
	CHK_NODE;
	EXEC(recv(0, SZ));
	EXEC(eager(0x10, SZ-0x10, 0x12345));
	EXEC(dst_mr.check());
}

TEST_F(tag_matching, e1_match) {
	CHK_NODE;
	EXEC(append(0, SZ, 1));
	EXEC(eager(0, SZ, 1));
	EXEC(dst_mr.check());
}

#undef N
#define N 8

TEST_F(tag_matching, e2_matchN) {
	int i;
	CHK_NODE;
	for (i=0; i<N; i++)
		EXEC(append(SZ/N*i, SZ/N, i));
	for (i=0; i<N; i++)
		EXEC(eager(SZ/N*i, SZ/N, i));
	EXEC(dst_mr.check());
}

TEST_F(tag_matching, e3_remove) {
	int i3, i4;
	CHK_NODE;
	EXEC(append(0, SZ/2, 3, &i3));
	EXEC(append(0, SZ/2, 1));
	EXEC(remove(i3));
	EXEC(append(SZ/2, SZ/2, 4, &i4));
	EXEC(remove(i4));
	EXEC(append(SZ/2, SZ/2, 2));
	EXEC(eager(SZ/2, SZ/2, 2));
	EXEC(eager(0, SZ/2, 1));
	EXEC(dst_mr.check());
}

TEST_F(tag_matching, e4_repeat) {
	CHK_NODE;

	EXEC(recv(0, SZ));
	EXEC(eager(0x10, SZ-0x10, 5));
	EXEC(dst_mr.check());

	EXEC(append(0, SZ, 5));
	EXEC(eager(0, SZ, 5));
	EXEC(dst_mr.check());

	EXEC(recv(0, SZ));
	EXEC(eager(0x10, SZ-0x10, 5));
	EXEC(dst_mr.check());

	EXEC(append(0, SZ, 5));
	EXEC(eager(0, SZ, 5));
	EXEC(dst_mr.check());

	EXEC(recv(0, SZ));
	EXEC(eager(0x10, SZ-0x10, 5));
	EXEC(dst_mr.check());
}

#undef N
#define N 2
#define R 1

TEST_F(tag_matching, e5_mix) {
	int i, idx[N], j, s;
	CHK_NODE;

	for (j=0; j<R; j++) {
		s = j&1;
		VERBS_INFO("round %d\n", j);
		for (i=s; i<N; i++)
			EXEC(append(0, SZ, i, idx + i));
		for (i=s; i<N; i++)
			if (i&1) {
				EXEC(remove(idx[i]));
				EXEC(recv(0, SZ));
			}
		for (i=s; i<N; i++) {
			if (i&1)
				EXEC(eager(0x10, SZ-0x10, i));
			else
				EXEC(eager(0, SZ, i));
			EXEC(dst_mr.check());
		}
	}
}

TEST_F(tag_matching, e6_unexp_inline) {
	ibvt_mr src(*this, this->pd, 0x20),
		dst(*this, this->pd, 0x20);
	CHK_NODE;

	src.fill();
	dst.init();
	EXEC(srq.recv(dst, 0, 0x20));
	EXEC(send_qp.send(src, 0x10, 0x10, 0x12345,
			  IBV_WR_TAG_SEND_EAGER));
	EXEC(send_cq.poll(1));
	EXEC(srq_cq.poll(1));
	dst.dump();
}

TEST_F(tag_matching, e7_no_tag) {
	CHK_NODE;
	EXEC(recv(0, SZ));
	EXEC(send_qp.send(src_mr, 0x10, SZ-0x10, 0,
			  IBV_WR_TAG_SEND_NO_TAG));
	EXEC(send_cq.poll(1));
	EXEC(srq_cq.poll(1));
	EXEC(dst_mr.check());
}

#undef N
#define N (SZ / 0x40)

TEST_F(tag_matching, e8_unexp_long) {
	int i;
	CHK_NODE;
	for(i = 0; i < N; i++) {
		EXEC(recv(SZ / N * i, SZ / N));
		EXEC(eager(SZ / N * i + 0x10,
			   SZ / N - 0x10,
			   1ULL << i));
	}
	//dst_mr.dump();
	EXEC(dst_mr.check());
}

TEST_F(tag_matching, r0_unexp) {
	ibvt_mr src(*this, this->pd, 0x80),
		dst(*this, this->pd, 0x80);

	CHK_NODE;

	src.fill();
	dst.init();
	EXEC(srq.recv(dst, 0, 0x20));
	EXEC(send_qp.send(src, 0x10, 0x10, 1,
			  IBV_WR_TAG_SEND_RNDV));
	EXEC(send_cq.poll(1));
	EXEC(srq_cq.poll(1));
	dst.dump();
}

TEST_F(tag_matching, r1_match) {
	CHK_NODE;
	EXEC(append(0, SZ, 1));
	EXEC(rndv(0, SZ, 1));
	EXEC(dst_mr.check());

}

TEST_F(tag_matching, r2_match2) {
	CHK_NODE;
	EXEC(append(0, SZ/2, 1));
	EXEC(append(SZ/2, SZ/2, 2));
	EXEC(rndv(SZ/2, SZ/2, 2));
	EXEC(rndv(0, SZ/2, 1));
	EXEC(dst_mr.check());
}

TEST_F(tag_matching, r3_remove) {
	int i3, i4;
	CHK_NODE;
	EXEC(append(0, SZ/2, 3, &i3));
	EXEC(append(0, SZ/2, 1));
	EXEC(remove(i3));
	EXEC(append(SZ/2, SZ/2, 4, &i4));
	EXEC(remove(i4));
	EXEC(append(SZ/2, SZ/2, 2));
	EXEC(rndv(SZ/2, SZ/2, 2));
	EXEC(rndv(0, SZ/2, 1));
	EXEC(dst_mr.check());
}

