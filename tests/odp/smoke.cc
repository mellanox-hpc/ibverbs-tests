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

struct ibvt_mr_implicit : public ibvt_mr {
	int access_flags;

	ibvt_mr_implicit(ibvt_env &e, ibvt_pd &p, int a) :
		ibvt_mr(e, p, 0), access_flags(a) {}

	virtual void init() {
		EXEC(pd.init());
		SET(mr, ibv_reg_mr(pd.pd, 0, UINT64_MAX, IBV_ACCESS_ON_DEMAND | access_flags));
		if (mr)
			VERBS_TRACE("\t\t\t\t\tibv_reg_mr(pd, 0, 0, %x) = %x\n", access_flags, mr->lkey);
	}
};

struct ibvt_sub_mr : public ibvt_mr {
	ibvt_mr_implicit &master;
	intptr_t addr;

	ibvt_sub_mr(ibvt_mr_implicit &i, intptr_t a, size_t size) :
		ibvt_mr(i.env, i.pd, size), master(i), addr(a) {}

	virtual void init() {
		mr = master.mr;
		buff = (char*)mmap((void*)addr, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON|MAP_FIXED, -1, 0);
	}

	virtual ~ibvt_sub_mr() {
		munmap(buff, size);
		mr = NULL;
	}
};

struct side : public ibvt_obj {
	ibvt_ctx &ctx;
	ibvt_pd pd;
	ibvt_cq cq;
	ibvt_qp_rc qp;
	ibvt_mr_implicit imr;

	side(ibvt_env &e, ibvt_ctx &c, int a) :
		ibvt_obj(e),
		ctx(c),
		pd(e, ctx),
		cq(e, ctx),
		qp(e, pd, cq),
		imr(e, pd, a) {}

	virtual void init() {
		EXEC(qp.init());
		EXEC(imr.init());
	}
};

struct odp_base : public testing::Test, public ibvt_env {
	ibvt_ctx ctx;
	side src;
	side dst;

	odp_base(int src_access_flags, int dst_access_flags) :
		ctx(*this, NULL),
		src(*this, ctx, src_access_flags),
		dst(*this, ctx, dst_access_flags) {}

	void check_stats(int mrs, int pages) {
		EXEC(ctx.check_debugfs("odp_stats/num_odp_mrs", mrs));
		EXEC(ctx.check_debugfs("odp_stats/num_odp_mr_pages", pages));
	}

	virtual void test(unsigned long src, unsigned long dst, size_t len) = 0;

	virtual void SetUp() {
		INIT(ctx.init());
		INIT(ctx.init_debugfs());
		INIT(check_stats(0, 0));
		INIT(src.init());
		INIT(dst.init());
		INIT(src.qp.connect(&dst.qp));
		INIT(dst.qp.connect(&src.qp));
	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());
	}
};

struct odp_send : public odp_base {
	odp_send():
		odp_base(0, IBV_ACCESS_LOCAL_WRITE) {}

	virtual void test(unsigned long src, unsigned long dst, size_t len) {
		ibvt_sub_mr src_mr(this->src.imr, src, len);
		ibvt_sub_mr dst_mr(this->dst.imr, dst, len);

		EXECL(src_mr.fill());
		EXECL(dst_mr.init());

		EXEC(check_stats(2, 0));
		EXEC(dst.qp.recv(dst_mr, 0, len));
		EXEC(src.qp.send(src_mr, 0, len));
		EXEC(src.cq.poll(1));
		EXEC(dst.cq.poll(1));
		EXEC(check_stats(2, len / 0x1000 * 2));

		EXECL(dst_mr.check());
	}
};

struct odp_rdma_read : public odp_base {
	odp_rdma_read():
		odp_base(IBV_ACCESS_REMOTE_READ, IBV_ACCESS_LOCAL_WRITE) {}

	virtual void test(unsigned long src, unsigned long dst, size_t len) {
		ibvt_sub_mr src_mr(this->src.imr, src, len);
		ibvt_sub_mr dst_mr(this->dst.imr, dst, len);
		EXECL(src_mr.fill());
		EXECL(dst_mr.init());

		EXEC(check_stats(2, 0));
		EXEC(dst.qp.rdma(dst_mr,
				 src_mr,
				 IBV_WR_RDMA_READ));
		EXEC(dst.cq.poll(1));
		EXEC(check_stats(2, len / 0x1000 * 2));
		EXECL(dst_mr.check());
	}
};

struct odp_rdma_write : public odp_base {
	odp_rdma_write():
		odp_base(0, IBV_ACCESS_LOCAL_WRITE|IBV_ACCESS_REMOTE_WRITE) {}

	virtual void test(unsigned long src, unsigned long dst, size_t len) {
		ibvt_sub_mr src_mr(this->src.imr, src, len);
		ibvt_sub_mr dst_mr(this->dst.imr, dst, len);
		EXECL(src_mr.fill());
		EXECL(dst_mr.init());

		EXEC(check_stats(2, 0));
		EXEC(src.qp.rdma(src_mr,
				 dst_mr,
				 IBV_WR_RDMA_WRITE));
		EXEC(src.cq.poll(1));
		EXEC(check_stats(2, len / 0x1000 * 2));
		EXECL(dst_mr.check());
	}
};


template <typename T>
struct odp : public T { };

typedef testing::Types<
	struct odp_send,
	struct odp_rdma_read,
	struct odp_rdma_write
> odp_env_list;

TYPED_TEST_CASE(odp, odp_env_list);

TYPED_TEST(odp, t0) {
	CHK_SUT(odp);
	EXEC(test((1ULL<<(32+1))-0x1000,
		  (1ULL<<(32+2))-0x1000,
		  0x2000));
}

TYPED_TEST(odp, t1) {
	CHK_SUT(odp);
	EXEC(test((1ULL<<47) - 0x10000 * 1,
		  (1ULL<<47) - 0x10000 * 2,
		  0x2000));
}

TYPED_TEST(odp, t2) {
	CHK_SUT(odp);
	unsigned long p = 0x2000000000-0x1000;
	for (int i = 0; i < 20; i++) {
		EXEC(test(p, p+0x1000, 0x1000));
		p += 0x2000;
	}
}

