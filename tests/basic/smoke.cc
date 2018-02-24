/**
 * Copyright (C) 2016      Mellanox Technologies Ltd. All rights reserved.
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

#include <infiniband/verbs.h>

#include "env.h"

//#define SZ 1024
#define SZ 128

template <typename T1, typename T2>
struct types_2 {
	typedef T1 QP;
	typedef T2 CQ;
};

template <typename T>
struct base_test : public testing::Test, public ibvt_env {
	struct ibvt_ctx ctx;
	struct ibvt_pd pd;
	struct T::CQ cq;
	struct T::QP send_qp;
	struct T::QP recv_qp;
	struct ibvt_mr src_mr;
	struct ibvt_mr dst_mr;

	base_test() :
		ctx(*this, NULL),
		pd(*this, ctx),
		cq(*this, ctx),
		send_qp(*this, pd, cq),
		recv_qp(*this, pd, cq),
		src_mr(*this, pd, SZ),
		dst_mr(*this, pd, SZ)
	{ }

	void send(intptr_t start, size_t length) {
		EXEC(send_qp.send(src_mr.sge(start, length)));
	}

	void recv(intptr_t start, size_t length) {
		EXEC(recv_qp.recv(dst_mr.sge(start, length)));
	}

	void check(int count) {
		EXEC(dst_mr.check(this->recv_qp.hdr_len(), 0, count));
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
		INIT(dst_mr.init());
		INIT(cq.arm());
	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());
	}
};

typedef testing::Types<
	types_2<ibvt_qp_rc, ibvt_cq>,
	types_2<ibvt_qp_ud, ibvt_cq>,
	types_2<ibvt_qp_rc, ibvt_cq_event>,
	types_2<ibvt_qp_ud, ibvt_cq_event>
> base_test_env_list;

TYPED_TEST_CASE(base_test, base_test_env_list);

TYPED_TEST(base_test, t0) {
	CHK_SUT(basic);
	EXEC(recv(0, SZ));
	EXEC(send(0, SZ));
	EXEC(cq.poll());
	EXEC(cq.poll());
	EXEC(check(1));
}

TYPED_TEST(base_test, t1) {
	CHK_SUT(basic);
	EXEC(recv(0, SZ/2));
	EXEC(recv(SZ/2, SZ/2));
	EXEC(send(0, SZ/2));
	EXEC(cq.poll());
	EXEC(cq.poll());
	EXEC(send(SZ/2, SZ/2));
	EXEC(cq.poll());
	EXEC(cq.poll());
	EXEC(check(2));
}

template <typename T>
struct rdma_test : public base_test<T> {};

typedef testing::Types<
	types_2<ibvt_qp_rc, ibvt_cq>,
	types_2<ibvt_qp_rc, ibvt_cq_event>
> rdma_test_env_list;

TYPED_TEST_CASE(rdma_test, rdma_test_env_list);

TYPED_TEST(rdma_test, t0) {
	CHK_SUT(basic);
	EXEC(send_qp.rdma(this->src_mr.sge(), this->dst_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll());
	EXEC(dst_mr.check());
}

TYPED_TEST(rdma_test, t1) {
	CHK_SUT(basic);
	EXEC(recv_qp.rdma(this->dst_mr.sge(), this->src_mr.sge(), IBV_WR_RDMA_READ));
	EXEC(cq.poll());
	EXEC(dst_mr.check());
}

template <typename T1, typename T2, typename T3>
struct types_3 {
	typedef T1 Send;
	typedef T2 Recv;
	typedef T3 CQ;
};

typedef testing::Types<
	types_3<ibvt_qp_rc, ibvt_qp_srq<ibvt_qp_rc>, ibvt_cq>,
	types_3<ibvt_qp_rc, ibvt_qp_srq<ibvt_qp_rc>, ibvt_cq_event>,
#if HAVE_INFINIBAND_VERBS_EXP_H
	types_3<ibvt_qp_dc, ibvt_dct, ibvt_cq>,
	types_3<ibvt_qp_dc, ibvt_dct, ibvt_cq_event>,
#endif
	types_3<ibvt_qp_ud, ibvt_qp_srq<ibvt_qp_ud>, ibvt_cq>,
	types_3<ibvt_qp_ud, ibvt_qp_srq<ibvt_qp_ud>, ibvt_cq_event>
> srq_test_env_list;

template <typename T>
struct srq_test : public testing::Test, public ibvt_env {
	struct ibvt_ctx ctx;
	struct ibvt_pd pd;
	struct T::CQ cq;
	struct ibvt_srq srq;
	struct T::Send send_qp;
	struct T::Recv recv_obj;
	struct ibvt_mr src_mr;
	struct ibvt_mr dst_mr;

	srq_test() :
		ctx(*this, NULL),
		pd(*this, ctx),
		cq(*this, ctx),
		srq(*this, pd, cq),
		send_qp(*this, pd, cq),
		recv_obj(*this, pd, cq, srq),
		src_mr(*this, pd, SZ),
		dst_mr(*this, pd, SZ)
	{ }

	void send(intptr_t start, size_t length) {
		EXEC(send_qp.send(src_mr.sge(start, length)));
	}

	void recv(intptr_t start, size_t length) {
		EXEC(srq.recv(dst_mr.sge(start, length)));
	}

	void check(int count) {
		EXEC(dst_mr.check(this->send_qp.hdr_len(), 0, count));
	}

	virtual void SetUp() {
		INIT(ctx.init());
		if (skip)
			return;
		INIT(srq.init());
		INIT(send_qp.init());
		INIT(recv_obj.init());
		INIT(send_qp.connect(&recv_obj));
		INIT(recv_obj.connect(&send_qp));
		INIT(src_mr.fill());
		INIT(dst_mr.init());
		INIT(cq.arm());
	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());
	}
};

TYPED_TEST_CASE(srq_test, srq_test_env_list);

TYPED_TEST(srq_test, t0) {
	CHK_SUT(basic);
	EXEC(recv(0, SZ));
	EXEC(send(0, SZ));
	EXEC(cq.poll());
	EXEC(cq.poll());
	EXEC(check(1));
}

