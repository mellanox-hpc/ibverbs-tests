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

#if HAVE_DECL_IBV_PREFETCH_MR
#define HAVE_PREFETCH
#define IBV_EXP_PREFETCH_WRITE_ACCESS 0
#elif HAVE_DECL_IBV_EXP_PREFETCH_MR
#define HAVE_PREFETCH
#define ibv_prefetch_attr ibv_exp_prefetch_attr
#define ibv_prefetch_mr ibv_exp_prefetch_mr
#endif

struct ibvt_mr_implicit : public ibvt_mr {
	ibvt_mr_implicit(ibvt_env &e, ibvt_pd &p, long a) :
		ibvt_mr(e, p, 0, 0, a) {}

	virtual void init() {
		EXEC(pd.init());
		SET(mr, ibv_reg_mr(pd.pd, 0, UINT64_MAX, IBV_ACCESS_ON_DEMAND | access_flags));
		if (mr)
			VERBS_TRACE("\t\t\t\t\tibv_reg_mr(pd, 0, 0, %lx) = %x\n", access_flags, mr->lkey);
	}
};

#ifdef HAVE_PREFETCH
struct ibvt_mr_pf : public ibvt_mr {
	ibvt_mr_pf(ibvt_env &e, ibvt_pd &p, size_t s, intptr_t a, long af) :
		ibvt_mr(e, p, s, a, af) {}

	virtual void init() {
		EXEC(ibvt_mr::init());

		struct ibv_prefetch_attr attr;

		attr.flags = IBV_EXP_PREFETCH_WRITE_ACCESS;
		attr.addr = buff;
		attr.length = size;
		attr.comp_mask = 0;
		DO(ibv_prefetch_mr(mr, &attr));
	}
};
#endif

#if HAVE_DECL_IBV_ACCESS_HUGETLB
struct ibvt_mr_hp : public ibvt_mr {
	ibvt_mr_hp(ibvt_env &e, ibvt_pd &p, size_t s, intptr_t a, long af) :
		ibvt_mr(e, p, s, a, af | IBV_ACCESS_HUGETLB) {}

	virtual int mmap_flags() {
		return MAP_PRIVATE|MAP_ANON|MAP_HUGETLB;
	}
};
#endif

struct ibvt_sub_mr : public ibvt_mr {
	ibvt_mr_implicit &master;

	ibvt_sub_mr(ibvt_mr_implicit &i, intptr_t a, size_t size) :
		ibvt_mr(i.env, i.pd, size, a), master(i) {}

	virtual void init() {
		int flags = MAP_PRIVATE|MAP_ANON;
		if (addr)
			flags |= MAP_FIXED;
		mr = master.mr;
		buff = (char*)mmap((void*)addr, size, PROT_READ|PROT_WRITE, flags, -1, 0);
		ASSERT_NE(buff, MAP_FAILED);
	}

	virtual ~ibvt_sub_mr() {
		mr = NULL;
	}
};

struct odp_side : public ibvt_obj {
	ibvt_ctx &ctx;
	ibvt_pd &pd;
	ibvt_cq cq;
	ibvt_qp_rc qp;
	int access_flags;

	odp_side(ibvt_env &e, ibvt_ctx &c, ibvt_pd &p, int a) :
		ibvt_obj(e),
		ctx(c),
		pd(p),
		cq(e, ctx),
		qp(e, pd, cq),
		access_flags(a) {}

	virtual void init() {
		EXEC(qp.init());
	}
};

struct odp_mem : public ibvt_obj {
	odp_side &ssrc;
	odp_side &sdst;

	ibvt_mr *psrc;
	ibvt_mr *pdst;

	odp_mem(odp_side &s, odp_side &d) : ibvt_obj(s.env), ssrc(s), sdst(d), psrc(NULL), pdst(NULL) {}

	virtual void init() {}
	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) = 0;
	virtual ibvt_mr &src() { return *psrc; }
	virtual ibvt_mr &dst() { return *pdst; }
	virtual void unreg() {
		if (psrc)
			delete psrc;
		if (pdst)
			delete pdst;
	}
};

struct odp_base : public testing::Test, public ibvt_env {
	ibvt_ctx ctx;
	ibvt_pd pd;
	odp_side src;
	odp_side dst;

	odp_base(int src_access_flags, int dst_access_flags) :
		ctx(*this, NULL),
		pd(*this, ctx),
		src(*this, ctx, pd, src_access_flags),
		dst(*this, ctx, pd, dst_access_flags) {}

	void check_stats(int mrs, int pages) {
		EXEC(ctx.check_debugfs("odp_stats/num_odp_mrs", mrs));
		EXEC(ctx.check_debugfs("odp_stats/num_odp_mr_pages", pages));
	}

	virtual odp_mem &mem() = 0;
	virtual void test(unsigned long src, unsigned long dst, size_t len, int count = 1) = 0;

	virtual void init() {
		INIT(ctx.init());
		INIT(ctx.init_debugfs());
		INIT(check_stats(0, 0));
		INIT(src.init());
		INIT(dst.init());
		INIT(src.qp.connect(&dst.qp));
		INIT(dst.qp.connect(&src.qp));
	}

	virtual void SetUp() {
		INIT(init());
	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());
	}
};


struct odp_off : public odp_mem {
	odp_off(odp_side &s, odp_side &d) : odp_mem(s, d) {}

	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) {
		SET(psrc, new ibvt_mr(ssrc.env, ssrc.pd, len, src_addr, ssrc.access_flags));
		SET(pdst, new ibvt_mr(sdst.env, sdst.pd, len, dst_addr, sdst.access_flags));
	}

};

struct odp_explicit : public odp_mem {
	odp_explicit(odp_side &s, odp_side &d) : odp_mem(s, d) {}

	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) {
		SET(psrc, new ibvt_mr(ssrc.env, ssrc.pd, len, src_addr, ssrc.access_flags | IBV_ACCESS_ON_DEMAND));
		SET(pdst, new ibvt_mr(sdst.env, sdst.pd, len, dst_addr, sdst.access_flags | IBV_ACCESS_ON_DEMAND));
	}
};

#ifdef HAVE_PREFETCH
struct odp_prefetch : public odp_mem {
	odp_prefetch(odp_side &s, odp_side &d) : odp_mem(s, d) {}

	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) {
		SET(psrc, new ibvt_mr_pf(ssrc.env, ssrc.pd, len, src_addr, ssrc.access_flags | IBV_ACCESS_ON_DEMAND));
		SET(pdst, new ibvt_mr_pf(sdst.env, sdst.pd, len, dst_addr, sdst.access_flags | IBV_ACCESS_ON_DEMAND));
	}
};
#endif


#if HAVE_DECL_IBV_ACCESS_HUGETLB
struct odp_hugetlb : public odp_mem {
	odp_hugetlb(odp_side &s, odp_side &d) : odp_mem(s, d) {}

	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) {
		SET(psrc, new ibvt_mr_hp(ssrc.env, ssrc.pd, len, src_addr, ssrc.access_flags | IBV_ACCESS_ON_DEMAND));
		SET(pdst, new ibvt_mr_hp(sdst.env, sdst.pd, len, dst_addr, sdst.access_flags | IBV_ACCESS_ON_DEMAND));
	}
};
#endif

struct odp_implicit : public odp_mem {
	ibvt_mr_implicit simr;
	ibvt_mr_implicit dimr;
	odp_implicit(odp_side &s, odp_side &d) : odp_mem(s, d),
		simr(s.env, s.pd, s.access_flags),
		dimr(d.env, d.pd, d.access_flags) {}

	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) {
		SET(psrc, new ibvt_sub_mr(simr, src_addr, len));
		SET(pdst, new ibvt_sub_mr(dimr, dst_addr, len));
	}
	virtual void init() {
		simr.init();
		dimr.init();
	}
};

struct odp_send : public odp_base {
	odp_send():
		odp_base(0, IBV_ACCESS_LOCAL_WRITE) {}

	virtual void test(unsigned long src, unsigned long dst, size_t len, int count = 1) {
		EXEC(mem().reg(src, dst, len));
		EXEC(mem().src().fill());
		EXEC(mem().dst().init());

		EXEC(check_stats(2, 0));
		for (int i = 0; i < count; i++) {
			EXEC(dst.qp.recv(mem().dst().sge(len/count*i, len/count)));
			EXEC(src.qp.send(mem().src().sge(len/count*i, len/count)));
			EXEC(src.cq.poll(1));
			EXEC(dst.cq.poll(1));
		}
		EXEC(check_stats(2, len / 0x1000 * 2));

		EXEC(mem().dst().check());
		EXEC(mem().unreg());
	}


};

struct odp_rdma_read : public odp_base {
	odp_rdma_read():
		odp_base(IBV_ACCESS_REMOTE_READ, IBV_ACCESS_LOCAL_WRITE) {}

	virtual void test(unsigned long src, unsigned long dst, size_t len, int count = 1) {
		EXEC(mem().reg(src, dst, len));
		EXEC(mem().src().fill());
		EXEC(mem().dst().init());

		EXEC(check_stats(2, 0));
		for (int i = 0; i < count; i++) {
			EXEC(dst.qp.rdma(mem().dst().sge(len/count*i, len/count),
					 mem().src().sge(len/count*i, len/count),
					 IBV_WR_RDMA_READ));
			EXEC(dst.cq.poll(1));
		}
		EXEC(check_stats(2, len / 0x1000 * 2));
		EXEC(mem().dst().check());
		EXEC(mem().unreg());
	}
};

struct odp_rdma_write : public odp_base {
	odp_rdma_write():
		odp_base(0, IBV_ACCESS_LOCAL_WRITE|IBV_ACCESS_REMOTE_WRITE) {}

	virtual void test(unsigned long src, unsigned long dst, size_t len, int count = 1) {
		EXEC(mem().reg(src, dst, len));
		EXEC(mem().src().fill());
		EXEC(mem().dst().init());

		EXEC(check_stats(2, 0));
		for (int i = 0; i < count; i++) {
			EXEC(src.qp.rdma(mem().src().sge(len/count*i, len/count),
						mem().dst().sge(len/count*i, len/count),
						IBV_WR_RDMA_WRITE));
			EXEC(src.cq.poll(1));
		}
		EXEC(check_stats(2, len / 0x1000 * 2));
		EXEC(mem().dst().check());
		EXEC(mem().unreg());
	}
};

struct odp_rdma_write_1_signal : public odp_base {
	odp_rdma_write_1_signal():
		odp_base(0, IBV_ACCESS_LOCAL_WRITE|IBV_ACCESS_REMOTE_WRITE) {}

	virtual void test(unsigned long src, unsigned long dst, size_t len, int count = 1) {
		EXEC(mem().reg(src, dst, len));
		EXEC(mem().src().fill());
		EXEC(mem().dst().init());
		enum ibv_send_flags f = IBV_SEND_INLINE;

		EXEC(check_stats(2, 0));
		for (int i = 0; i < count; i++) {
			if (i == count-1)
				f = (enum ibv_send_flags)(IBV_SEND_INLINE | IBV_SEND_SIGNALED);
			EXEC(src.qp.rdma(mem().src().sge(len/count*i, len/count),
						mem().dst().sge(len/count*i, len/count),
						IBV_WR_RDMA_WRITE, f));
		}
		EXEC(src.cq.poll(1));
		EXEC(check_stats(2, len / 0x1000 * 2));
		EXEC(mem().dst().check());
		EXEC(mem().unreg());
	}
};

template <typename T1, typename T2>
struct types {
	typedef T1 MEM;
	typedef T2 OP;
};

template <typename T>
struct odp : public T::OP {
	typename T::MEM _mem;
	odp(): T::OP(), _mem(T::OP::src, T::OP::dst) {}
	odp_mem &mem() { return _mem; }
	virtual void init() {
		INIT(T::OP::init());
		INIT(_mem.init());
	}
};

typedef testing::Types<
	types<odp_explicit, odp_send>,
	types<odp_explicit, odp_rdma_read>,
	types<odp_explicit, odp_rdma_write>,
#ifdef HAVE_PREFETCH
	types<odp_prefetch, odp_send>,
	types<odp_prefetch, odp_rdma_read>,
	types<odp_prefetch, odp_rdma_write>,
#endif
	types<odp_implicit, odp_send>,
	types<odp_implicit, odp_rdma_read>,
	types<odp_implicit, odp_rdma_write>,
	//types<odp_implicit, odp_rdma_write_1_signal>,
#if HAVE_DECL_IBV_ACCESS_HUGETLB
	types<odp_hugetlb, odp_send>,
	types<odp_hugetlb, odp_rdma_read>,
	types<odp_hugetlb, odp_rdma_write>,
#endif
	types<odp_off, odp_send>,
	types<odp_off, odp_rdma_read>,
	types<odp_off, odp_rdma_write>
> odp_env_list;

#ifdef __x86_64__

#define PAGE 0x1000
#define UP (1ULL<<47)

#elif (__ppc64__|__PPC64__)

#define PAGE 0x10000
#define UP (1ULL<<46)

#endif

TYPED_TEST_CASE(odp, odp_env_list);

TYPED_TEST(odp, t0_crossbound) {
	CHK_SUT(odp);
	EXEC(test((1ULL<<(32+1))-PAGE,
		  (1ULL<<(32+2))-PAGE,
		  0x2000));
}

TYPED_TEST(odp, t1_upper) {
	CHK_SUT(odp);
	EXEC(test(UP - 0x10000 * 1,
		  UP - 0x10000 * 2,
		  0x2000));
}

TYPED_TEST(odp, t2_sequence) {
	CHK_SUT(odp);
	unsigned long p = 0x2000000000;
	for (int i = 0; i < 20; i++) {
		EXEC(test(p, p+PAGE, PAGE));
		p += PAGE * 2;
	}
}

TYPED_TEST(odp, t3_1b) {
	CHK_SUT(odp);
	unsigned long p = 0x2000000000;
	EXEC(test(p, p+PAGE, PAGE, PAGE/0x10));
}

TYPED_TEST(odp, t4_6M) {
	CHK_SUT(odp);
	EXEC(test(0,0,0x600000,0x10));
}


TYPED_TEST(odp, t5_3G) {
	CHK_SUT(odp);
	EXEC(test(0, 0,
		  0xe0000000,
		  0x10));
}

#ifdef HUGE_AMOUNT_OF_MEMORY
TYPED_TEST(odp, t4_16Gplus) {
	CHK_SUT(odp);
	EXEC(test(0, 0,
		  0x400000100,
		  0x100));
}
#endif

