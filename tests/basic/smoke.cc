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

template <typename T1, typename T2, typename T3>
struct types {
	typedef T1 QP;
	typedef T2 MR;
	typedef T3 CQ;
};

template <typename T>
struct base_test : public testing::Test, public ibvt_env {
	struct ibvt_ctx ctx;
	struct ibvt_pd pd;
	struct T::CQ cq;
	struct T::QP send_qp;
	struct T::QP recv_qp;
	struct T::MR src_mr;
	struct T::MR dst_mr;

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

	virtual void SetUp() {
		EXEC(ctx.init());
		if (skip)
			return;
		EXEC(send_qp.init());
		EXEC(recv_qp.init());
		EXEC(send_qp.connect(&recv_qp));
		EXEC(recv_qp.connect(&send_qp));
		EXEC(src_mr.fill());
		EXEC(dst_mr.init());
		EXEC(cq.arm());
	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());
	}
};

typedef testing::Types<
	types<ibvt_qp_rc, ibvt_mr, ibvt_cq>,
	types<ibvt_qp_ud, ibvt_mr_ud, ibvt_cq>,
	types<ibvt_qp_rc, ibvt_mr, ibvt_cq_event>,
	types<ibvt_qp_ud, ibvt_mr_ud, ibvt_cq_event>
> base_test_env_list;

TYPED_TEST_CASE(base_test, base_test_env_list);

TYPED_TEST(base_test, t0) {
	CHK_SUT(basic);
	EXEC(recv(0, SZ));
	EXEC(send(0, SZ));
	EXEC(cq.poll(2));
	EXEC(dst_mr.check());
}

TYPED_TEST(base_test, t1) {
	CHK_SUT(basic);
	EXEC(recv(0, SZ/2));
	EXEC(recv(SZ/2, SZ/2));
	EXEC(send(0, SZ/2));
	EXEC(cq.poll(2));
	EXEC(send(SZ/2, SZ/2));
	EXEC(cq.poll(2));
	EXEC(dst_mr.check());
}

template <typename T>
struct rdma_test : public base_test<T> {};

typedef testing::Types<
	types<ibvt_qp_rc, ibvt_mr, ibvt_cq>,
	types<ibvt_qp_rc, ibvt_mr, ibvt_cq_event>
> rdma_test_env_list;

TYPED_TEST_CASE(rdma_test, rdma_test_env_list);

TYPED_TEST(rdma_test, t0) {
	CHK_SUT(basic);
	EXEC(send_qp.rdma(this->src_mr.sge(), this->dst_mr.sge(), IBV_WR_RDMA_WRITE));
	EXEC(cq.poll(1));
	EXEC(dst_mr.check());
}

TYPED_TEST(rdma_test, t1) {
	CHK_SUT(basic);
	EXEC(recv_qp.rdma(this->dst_mr.sge(), this->src_mr.sge(), IBV_WR_RDMA_READ));
	EXEC(cq.poll(1));
	EXEC(dst_mr.check());
}

