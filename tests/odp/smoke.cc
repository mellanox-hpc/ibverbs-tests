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
#define HAVE_PREFETCH 1
#define IBV_EXP_PREFETCH_WRITE_ACCESS 0
#elif HAVE_DECL_IBV_EXP_PREFETCH_MR
#define HAVE_PREFETCH 1
#define ibv_prefetch_attr ibv_exp_prefetch_attr
#define ibv_prefetch_mr ibv_exp_prefetch_mr
#else
#define HAVE_PREFETCH 0
#endif

struct ibvt_mr_implicit : public ibvt_mr {
	ibvt_mr_implicit(ibvt_env &e, ibvt_pd &p, long a) :
		ibvt_mr(e, p, 0, 0, a) {}

	virtual void init() {
		DO(!(pd.ctx.dev_attr.odp_caps.general_odp_caps & IBV_ODP_SUPPORT_IMPLICIT));
		EXEC(pd.init());
		SET(mr, ibv_reg_mr(pd.pd, 0, UINT64_MAX, IBV_ACCESS_ON_DEMAND | access_flags));
		if (mr)
			VERBS_TRACE("\t\t\t\t\tibv_reg_mr(pd, 0, 0, %lx) = %x\n", access_flags, mr->lkey);
	}
};

#if HAVE_PREFETCH
struct ibvt_mr_pf : public ibvt_mr {
	ibvt_mr_pf(ibvt_env &e, ibvt_pd &p, size_t s, intptr_t a, long af) :
		ibvt_mr(e, p, s, a, af) {}

	virtual void init() {
		EXEC(ibvt_mr::init());

		if (env.skip)
			return;
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
	ibvt_mr &master;

	ibvt_sub_mr(ibvt_mr &i, intptr_t a, size_t size) :
		ibvt_mr(i.env, i.pd, size, a), master(i) {}

	virtual void init() {
		EXEC(init_mmap());
		mr = master.mr;
	}

	virtual ~ibvt_sub_mr() {
		mr = NULL;
	}
};

struct odp_side : public ibvt_obj {
	ibvt_ctx &ctx;
	ibvt_pd &pd;
	int access_flags;

	odp_side(ibvt_env &e, ibvt_ctx &c, ibvt_pd &p, int a) :
		ibvt_obj(e),
		ctx(c),
		pd(p),
		access_flags(a) {}

	virtual ibvt_qp &get_qp() = 0;
};

template<typename QP>
struct odp_side_qp : public odp_side {
	ibvt_cq cq;
	QP qp;

	odp_side_qp(ibvt_env &e, ibvt_ctx &c, ibvt_pd &p, int a) :
		odp_side(e, c, p, a),
		cq(e, ctx),
		qp(e, pd, cq) {}

	virtual void init() {
		EXEC(qp.init());
	}

	virtual ibvt_qp &get_qp() { return qp; }
};

struct odp_mem : public ibvt_obj {
	odp_side &ssrc;
	odp_side &sdst;

	ibvt_abstract_mr *psrc;
	ibvt_abstract_mr *pdst;

	odp_mem(odp_side &s, odp_side &d) : ibvt_obj(s.env), ssrc(s), sdst(d), psrc(NULL), pdst(NULL) {}

	virtual void init() {}
	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) = 0;
	virtual ibvt_abstract_mr &src() { return *psrc; }
	virtual ibvt_abstract_mr &dst() { return *pdst; }
	virtual void unreg() {
		if (psrc)
			delete psrc;
		if (pdst)
			delete pdst;
	}

	int num_page_fault_pages;
	int num_invalidation_pages;

	virtual void check_stats_before() {
		EXEC(ssrc.ctx.check_debugfs("odp_stats/num_odp_mrs", 2));
		EXEC(ssrc.ctx.check_debugfs("odp_stats/num_odp_mr_pages", 0));
		EXEC(ssrc.ctx.read_dev_fs("num_page_fault_pages",
					  &num_page_fault_pages));
		EXEC(ssrc.ctx.read_dev_fs("num_invalidation_pages",
					  &num_invalidation_pages));
	}

	virtual void check_stats_after(size_t len) {
		int pages = (len + PAGE - 1) / PAGE * 2;
		int i;
		EXEC(ssrc.ctx.check_debugfs("odp_stats/num_odp_mrs", 2));
		EXEC(ssrc.ctx.check_dev_fs("num_page_fault_pages",
					   num_page_fault_pages + pages));
		EXEC(ssrc.ctx.read_dev_fs("num_invalidation_pages",
					  &i));
		pages -= i - num_invalidation_pages;
		EXEC(ssrc.ctx.check_debugfs("odp_stats/num_odp_mr_pages", pages));
	}
};

struct odp_trans : public ibvt_obj {
	odp_trans(ibvt_env &e) : ibvt_obj(e) {}

	virtual void send(ibv_sge sge) = 0;
	virtual void recv(ibv_sge sge) = 0;
	virtual void rdma_src(ibv_sge src_sge, ibv_sge dst_sge, enum ibv_wr_opcode opcode) {}
	virtual void rdma_dst(ibv_sge src_sge, ibv_sge dst_sge, enum ibv_wr_opcode opcode) {}
	virtual void poll_src() = 0;
	virtual void poll_dst() = 0;
};

template<typename Ctx>
struct odp_base : public testing::Test, public ibvt_env {
	Ctx ctx;
	ibvt_pd pd;
	int src_access_flags;
	int dst_access_flags;

	odp_base(int s, int d) :
		ctx(*this, NULL),
		pd(*this, ctx),
		src_access_flags(s),
		dst_access_flags(d) {}

	virtual odp_mem &mem() = 0;
	virtual odp_trans &trans() = 0;
	virtual void test(unsigned long src, unsigned long dst, size_t len, int count = 1) = 0;
	virtual void test_page(unsigned long addr) {
		EXEC(test(addr, addr + PAGE, PAGE));
	}

	virtual void init() {
		INIT(ctx.init());
		DO(!(ctx.dev_attr.odp_caps.general_odp_caps & IBV_ODP_SUPPORT));
		INIT(ctx.init_sysfs());
		EXEC(ctx.check_debugfs("odp_stats/num_odp_mrs", 0));
	}

	virtual void SetUp() {
		INIT(init());
	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());
	}
};

template<typename QP, typename Ctx>
struct odp_qp : public odp_trans {
	ibvt_ctx &ctx;
	ibvt_pd &pd;
	odp_side_qp<QP> src;
	odp_side_qp<QP> dst;

	odp_qp(odp_base<Ctx> &e) :
		odp_trans(e),
		ctx(e.ctx),
		pd(e.pd),
		src(e, e.ctx, e.pd, e.src_access_flags),
		dst(e, e.ctx, e.pd, e.dst_access_flags) {}

	virtual void init() {
		INIT(src.init());
		INIT(dst.init());
		INIT(src.qp.connect(&dst.qp));
		INIT(dst.qp.connect(&src.qp));
	}

	virtual void send(ibv_sge sge) { src.qp.send(sge); }
	virtual void recv(ibv_sge sge) { dst.qp.recv(sge); }
	virtual void rdma_src(ibv_sge src_sge, ibv_sge dst_sge,
			      enum ibv_wr_opcode opcode) {
		src.qp.rdma(src_sge, dst_sge, opcode);
	}
	virtual void rdma_dst(ibv_sge src_sge, ibv_sge dst_sge,
			      enum ibv_wr_opcode opcode) {
		dst.qp.rdma(src_sge, dst_sge, opcode);
	}
	virtual void poll_src() { src.cq.poll(); }
	virtual void poll_dst() { dst.cq.poll(); }
};

typedef odp_qp<ibvt_qp_rc, ibvt_ctx> odp_rc;
//typedef odp_qp<ibvt_qp_rc, ibvt_ctx_devx> odp_rc_devx;

#if HAVE_DC
struct odp_side_dci : public odp_side {
	ibvt_cq cq;
	ibvt_qp_dc qp;

	odp_side_dci(ibvt_env &e, ibvt_ctx &c, ibvt_pd &p, int a) :
		odp_side(e, c, p, a),
		cq(e, ctx),
		qp(e, pd, cq) {}

	virtual void init() {
		EXEC(qp.init());
	}

	virtual ibvt_qp &get_qp() { return qp; }
};

struct odp_side_dct : public odp_side {
	ibvt_cq cq;
	ibvt_srq srq;
	ibvt_dct dct;
	ibvt_qp_rc _qp;

	odp_side_dct(ibvt_env &e, ibvt_ctx &c, ibvt_pd &p, int a) :
		odp_side(e, c, p, a),
		cq(e, ctx),
		srq(e, pd, cq),
		dct(e, pd, cq, srq),
		_qp(e, p, cq) {}

	virtual void init() {
		EXEC(dct.init());
	}

	virtual ibvt_qp &get_qp() { return _qp; }
};

struct odp_dc : public odp_trans {
	ibvt_ctx &ctx;
	ibvt_pd &pd;
	odp_side_dci src;
	odp_side_dct dst;

	odp_dc(odp_base<ibvt_ctx> &e) :
		odp_trans(e),
		ctx(e.ctx),
		pd(e.pd),
		src(e, e.ctx, e.pd, e.src_access_flags),
		dst(e, e.ctx, e.pd, e.dst_access_flags) {}

	virtual void init() {
		INIT(src.init());
		INIT(dst.init());
		INIT(src.qp.connect(&dst.dct));
	}

	virtual void send(ibv_sge sge) { src.qp.send(sge); }
	virtual void recv(ibv_sge sge) { dst.srq.recv(sge); }
	virtual void rdma_src(ibv_sge src_sge, ibv_sge dst_sge,
			      enum ibv_wr_opcode opcode) {
		src.qp.rdma(src_sge, dst_sge, opcode);
	}
	virtual void rdma_dst(ibv_sge src_sge, ibv_sge dst_sge,
			      enum ibv_wr_opcode opcode) {
		src.qp.rdma(src_sge, dst_sge, opcode);
	}
	virtual void poll_src() { src.cq.poll(); }
	virtual void poll_dst() { dst.cq.poll(); }

};

struct odp_dc2 : public odp_dc {
	odp_dc2(odp_base<ibvt_ctx> &e) :
		odp_dc(e) {}

	virtual void poll_dst() { src.cq.poll(); }
};
#endif

struct odp_off : public odp_mem {
	odp_off(odp_side &s, odp_side &d) : odp_mem(s, d) {}

	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) {
		SET(psrc, new ibvt_mr(ssrc.env, ssrc.pd, len, src_addr, ssrc.access_flags));
		SET(pdst, new ibvt_mr(sdst.env, sdst.pd, len, dst_addr, sdst.access_flags));
	}

	virtual void check_stats_before() { }
	virtual void check_stats_after(size_t len) { }
};

struct odp_explicit : public odp_mem {
	odp_explicit(odp_side &s, odp_side &d) : odp_mem(s, d) {}

	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) {
		SET(psrc, new ibvt_mr(ssrc.env, ssrc.pd, len, src_addr, ssrc.access_flags | IBV_ACCESS_ON_DEMAND));
		SET(pdst, new ibvt_mr(sdst.env, sdst.pd, len, dst_addr, sdst.access_flags | IBV_ACCESS_ON_DEMAND));
	}
};

#if HAVE_PREFETCH
struct odp_prefetch : public odp_mem {
	odp_prefetch(odp_side &s, odp_side &d) : odp_mem(s, d) {}

	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) {
		SET(psrc, new ibvt_mr_pf(ssrc.env, ssrc.pd, len, src_addr, ssrc.access_flags | IBV_ACCESS_ON_DEMAND));
		SET(pdst, new ibvt_mr_pf(sdst.env, sdst.pd, len, dst_addr, sdst.access_flags | IBV_ACCESS_ON_DEMAND));
	}

	virtual void check_stats_before() { }
	virtual void check_stats_after(size_t len) { }
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

struct ibvt_qp_rc_umr : public ibvt_qp_rc {
	ibvt_qp_rc_umr(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) :
		ibvt_qp_rc(e, p, c) {}

#if HAVE_INFINIBAND_VERBS_EXP_H
	virtual void init_attr(struct ibv_qp_init_attr_ex &attr) {
		ibvt_qp_rc::init_attr(attr);
		attr.comp_mask |= IBV_EXP_QP_INIT_ATTR_CREATE_FLAGS;
		attr.comp_mask |= IBV_EXP_QP_INIT_ATTR_MAX_INL_KLMS;
		attr.exp_create_flags |= IBV_EXP_QP_CREATE_UMR;
		attr.max_inl_send_klms = 3;
	}
#endif
};

typedef odp_qp<ibvt_qp_rc_umr, ibvt_ctx> odp_rc_umr;

struct odp_implicit_mw : public odp_implicit {
	odp_implicit_mw(odp_side &s, odp_side &d) :
		odp_implicit(s, d) {};

	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) {
		SET(psrc, new ibvt_mw(simr, src_addr, len, ssrc.get_qp()));
		SET(pdst, new ibvt_mw(dimr, dst_addr, len, sdst.get_qp()));
	}
};

#define ODP_CHK_SUT(len) \
	CHK_SUT(odp); \
	this->check_ram("MemFree:", len * 3); \
	this->mem().reg(0,0,len); \
	this->mem().src().init(); \
	this->mem().dst().init(); \
	this->mem().unreg();


template<typename Ctx>
struct odp_send : public odp_base<Ctx> {
	odp_send():
		odp_base<Ctx>(0, IBV_ACCESS_LOCAL_WRITE) {}

	virtual void test(unsigned long src, unsigned long dst, size_t len, int count = 1) {
		EXEC(mem().reg(src, dst, len));
		EXEC(mem().src().fill());
		EXEC(mem().dst().init());

		EXEC(mem().check_stats_before());
		for (int i = 0; i < count; i++) {
			EXEC(trans().recv(this->mem().dst().sge(len/count*i, len/count)));
			EXEC(trans().send(this->mem().src().sge(len/count*i, len/count)));
			EXEC(trans().poll_src());
			EXEC(trans().poll_dst());
		}
		EXEC(mem().check_stats_after(len));
		EXEC(mem().dst().check());
		EXEC(mem().unreg());
	}
};

template<typename Ctx>
struct odp_rdma_read : public odp_base<Ctx> {
	odp_rdma_read():
		odp_base<Ctx>(IBV_ACCESS_REMOTE_READ, IBV_ACCESS_LOCAL_WRITE) {}

	virtual void test(unsigned long src, unsigned long dst, size_t len, int count = 1) {
		EXEC(mem().reg(src, dst, len));
		EXEC(mem().dst().init());
		EXEC(mem().src().fill());

		EXEC(mem().check_stats_before());
		for (int i = 0; i < count; i++) {
			EXEC(trans().rdma_dst(this->mem().dst().sge(len/count*i, len/count),
					      this->mem().src().sge(len/count*i, len/count),
					      IBV_WR_RDMA_READ));
			EXEC(trans().poll_dst());
		}
		EXEC(mem().check_stats_after(len));
		EXEC(mem().dst().check());
		EXEC(mem().unreg());
	}
};

template<typename Ctx>
struct odp_rdma_write : public odp_base<Ctx> {
	odp_rdma_write():
		odp_base<Ctx>(0, IBV_ACCESS_LOCAL_WRITE|IBV_ACCESS_REMOTE_WRITE) {}

	virtual void test(unsigned long src, unsigned long dst, size_t len, int count = 1) {
		EXEC(mem().reg(src, dst, len));
		EXEC(mem().src().fill());
		EXEC(mem().dst().init());

		EXEC(mem().check_stats_before());
		for (int i = 0; i < count; i++) {
			EXEC(trans().rdma_src(this->mem().src().sge(len/count*i, len/count),
					      this->mem().dst().sge(len/count*i, len/count),
						IBV_WR_RDMA_WRITE));
			EXEC(trans().poll_src());
		}
		EXEC(mem().check_stats_after(len));
		EXEC(mem().dst().check());
		EXEC(mem().unreg());
	}
};

template <typename T1, typename T2, typename T3>
struct types {
	typedef T1 MEM;
	typedef T2 TRANS;
	typedef T3 OP;
};

template <typename T>
struct odp : public T::OP {
	typename T::TRANS _trans;
	typename T::MEM _mem;
	odp():
		T::OP(),
		_trans(*this),
		_mem(_trans.src, _trans.dst) {}
	odp_mem &mem() { return _mem; }
	odp_trans &trans() { return _trans; }
	virtual void init() {
		INIT(T::OP::init());
		INIT(_trans.init());
		INIT(_mem.init());
	}
};

typedef testing::Types<
	types<odp_explicit, odp_rc, odp_send<ibvt_ctx> >,
	types<odp_explicit, odp_rc, odp_rdma_read<ibvt_ctx> >,
	types<odp_explicit, odp_rc, odp_rdma_write<ibvt_ctx> >,
#if HAVE_PREFETCH
	types<odp_prefetch, odp_rc, odp_send<ibvt_ctx> >,
	types<odp_prefetch, odp_rc, odp_rdma_read<ibvt_ctx> >,
	types<odp_prefetch, odp_rc, odp_rdma_write<ibvt_ctx> >,
#endif
	types<odp_implicit, odp_rc, odp_send<ibvt_ctx> >,
	types<odp_implicit, odp_rc, odp_rdma_read<ibvt_ctx> >,
	types<odp_implicit, odp_rc, odp_rdma_write<ibvt_ctx> >,
#if HAVE_DECL_IBV_ACCESS_HUGETLB
	types<odp_hugetlb, odp_rc, odp_send<ibvt_ctx> >,
	types<odp_hugetlb, odp_rc, odp_rdma_read<ibvt_ctx> >,
	types<odp_hugetlb, odp_rc, odp_rdma_write<ibvt_ctx> >,
#endif

#if HAVE_DC
	types<odp_explicit, odp_dc, odp_rdma_write<ibvt_ctx> >,
	types<odp_implicit, odp_dc, odp_rdma_write<ibvt_ctx> >,
	types<odp_off, odp_dc, odp_send<ibvt_ctx> >,
	types<odp_off, odp_dc, odp_rdma_write<ibvt_ctx> >,
	types<odp_off, odp_dc2, odp_rdma_read<ibvt_ctx> >,
#endif

	types<odp_implicit_mw, odp_rc_umr, odp_send<ibvt_ctx> >,
	types<odp_implicit_mw, odp_rc_umr, odp_rdma_read<ibvt_ctx> >,
	types<odp_implicit_mw, odp_rc_umr, odp_rdma_write<ibvt_ctx> >,

	types<odp_off, odp_rc, odp_send<ibvt_ctx> >,
	types<odp_off, odp_rc, odp_rdma_read<ibvt_ctx> >,
	types<odp_off, odp_rc, odp_rdma_write<ibvt_ctx> >
> odp_env_list;

TYPED_TEST_CASE(odp, odp_env_list);

TYPED_TEST(odp, t0_crossbound) {
	ODP_CHK_SUT(PAGE);
	EXEC(test((1ULL<<(32+1))-PAGE,
		  (1ULL<<(32+2))-PAGE,
		  PAGE * 2));
}

TYPED_TEST(odp, t1_upper) {
	ODP_CHK_SUT(PAGE);
	EXEC(test(UP - 0x10000 * 1,
		  UP - 0x10000 * 2,
		  0x2000));
}

TYPED_TEST(odp, t2_sequence) {
	ODP_CHK_SUT(PAGE);
	unsigned long p = 0x2000000000;
	for (int i = 0; i < 20; i++) {
		EXEC(test(p, p+PAGE, PAGE));
		p += PAGE * 2;
	}
}

TYPED_TEST(odp, t3_1b) {
	ODP_CHK_SUT(PAGE);
	unsigned long p = 0x2000000000;
	EXEC(test(p, p+PAGE, PAGE, PAGE/0x10));
}

TYPED_TEST(odp, t4_6M) {
	ODP_CHK_SUT(0x600000);
	EXEC(test(0,0,0x600000,0x10));
}

template <typename T>
struct odp_long : public odp<T> { odp_long(): odp<T>() {} };

typedef testing::Types<
	types<odp_explicit, odp_rc, odp_send<ibvt_ctx> >,
	types<odp_implicit, odp_rc, odp_send<ibvt_ctx> >,
#if HAVE_DECL_IBV_ACCESS_HUGETLB
	types<odp_hugetlb, odp_rc, odp_send<ibvt_ctx> >,
#endif
	types<odp_off, odp_rc, odp_send<ibvt_ctx> >
> odp_env_list_long;

TYPED_TEST_CASE(odp_long, odp_env_list_long);

TYPED_TEST(odp_long, t5_3G) {
	ODP_CHK_SUT(0xe0000000);
	EXEC(test(0, 0,
		  0xe0000000,
		  0x10));
}

TYPED_TEST(odp_long, t6_16Gplus) {
	ODP_CHK_SUT(0x400000100);
	EXEC(test(0, 0,
		  0x400000100,
		  0x100));
}

#if HAVE_INFINIBAND_VERBS_EXP_H
struct odp_implicit_mw_1imr : public odp_mem {
	ibvt_mr_implicit imr;

	odp_implicit_mw_1imr(odp_side &s, odp_side &d) : odp_mem(s, d),
		imr(s.env, s.pd, s.access_flags | d.access_flags) {}

	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) {
		SET(psrc, new ibvt_mw(imr, src_addr, len, ssrc.get_qp()));
		SET(pdst, new ibvt_mw(imr, dst_addr, len, sdst.get_qp()));
	}

	virtual void init() {
		imr.init();
	}
};

struct odp_persist : public odp_mem {
	ibvt_mr *smr;
	ibvt_mr *dmr;

	odp_persist(odp_side &s, odp_side &d) : odp_mem(s, d), smr(NULL), dmr(NULL) {}

	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) {
		if (!smr) {
			SET(smr, new ibvt_mr(ssrc.env, ssrc.pd, len, src_addr, ssrc.access_flags | IBV_ACCESS_ON_DEMAND));
			SET(dmr, new ibvt_mr(sdst.env, sdst.pd, len, dst_addr, sdst.access_flags | IBV_ACCESS_ON_DEMAND));
			smr->init();
			dmr->init();
		}
		SET(psrc, new ibvt_sub_mr(*smr, src_addr, len));
		SET(pdst, new ibvt_sub_mr(*dmr, dst_addr, len));
	}

	~odp_persist() {
		if (smr)
			delete smr;
		if (dmr)
			delete dmr;
	}
};

template <typename T>
struct odp_spec : public odp<T> { odp_spec(): odp<T>() {} };

typedef testing::Types<
	//types<odp_implicit_mw_1imr, odp_rc_umr, odp_send<ibvt_ctx> >
	//types<odp_persist, odp_rc, odp_send<ibvt_ctx> >,
	//types<odp_persist, odp_rc, odp_rdma_read<ibvt_ctx> >,
	//types<odp_persist, odp_rc, odp_rdma_write<ibvt_ctx> >
	types<odp_implicit, odp_rc, odp_send<ibvt_ctx> >,
	types<odp_implicit, odp_rc, odp_rdma_read<ibvt_ctx> >,
	types<odp_implicit, odp_rc, odp_rdma_write<ibvt_ctx> >
> odp_env_list_spec;

TYPED_TEST_CASE(odp_spec, odp_env_list_spec);

TYPED_TEST(odp_spec, s0) {
	ODP_CHK_SUT(PAGE);
	unsigned long p = 0x2000000000;
	for (int i = 0; i < 20000; i++)
		EXEC(test(p, p+0x40000, 0x40000));
}
#endif

#if HAVE_DECL_MLX5DV_CONTEXT_FLAGS_DEVX
#include "../devx/devx_prm.h"

#if 0
enum {
	MLX5_WQE_UMR_CTRL_FLAG_INLINE =			1 << 7,
	MLX5_WQE_UMR_CTRL_FLAG_CHECK_FREE =		1 << 5,
	MLX5_WQE_UMR_CTRL_FLAG_TRNSLATION_OFFSET =	1 << 4,
	MLX5_WQE_UMR_CTRL_FLAG_CHECK_QPN =		1 << 3,
};

enum {
	MLX5_WQE_UMR_CTRL_MKEY_MASK_LEN			= 1 << 0,
	MLX5_WQE_UMR_CTRL_MKEY_MASK_START_ADDR		= 1 << 6,
	MLX5_WQE_UMR_CTRL_MKEY_MASK_MKEY		= 1 << 13,
	MLX5_WQE_UMR_CTRL_MKEY_MASK_QPN			= 1 << 14,
	MLX5_WQE_UMR_CTRL_MKEY_MASK_ACCESS_LOCAL_WRITE	= 1 << 18,
	MLX5_WQE_UMR_CTRL_MKEY_MASK_ACCESS_REMOTE_READ	= 1 << 19,
	MLX5_WQE_UMR_CTRL_MKEY_MASK_ACCESS_REMOTE_WRITE	= 1 << 20,
	MLX5_WQE_UMR_CTRL_MKEY_MASK_ACCESS_ATOMIC	= 1 << 21,
	MLX5_WQE_UMR_CTRL_MKEY_MASK_FREE		= 1 << 29,
};

enum {
	MLX5_UMR_CTRL_INLINE	= 1 << 7,
};

struct mlx5_wqe_umr_ctrl_seg {
	uint8_t		flags;
	uint8_t		rsvd0[3];
	__be16		klm_octowords;
	__be16		translation_offset;
	__be64		mkey_mask;
	uint8_t		rsvd1[32];
};

struct mlx5_wqe_umr_klm_seg {
	/* up to 2GB */
	__be32		byte_count;
	__be32		mkey;
	__be64		address;
};

union mlx5_wqe_umr_inline_seg {
	struct mlx5_wqe_umr_klm_seg	klm;
};

enum {
	MLX5_WQE_MKEY_CONTEXT_FREE = 1 << 6
};

enum {
	MLX5_WQE_MKEY_CONTEXT_ACCESS_FLAGS_ATOMIC = 1 << 6,
	MLX5_WQE_MKEY_CONTEXT_ACCESS_FLAGS_REMOTE_WRITE = 1 << 5,
	MLX5_WQE_MKEY_CONTEXT_ACCESS_FLAGS_REMOTE_READ = 1 << 4,
	MLX5_WQE_MKEY_CONTEXT_ACCESS_FLAGS_LOCAL_WRITE = 1 << 3,
	MLX5_WQE_MKEY_CONTEXT_ACCESS_FLAGS_LOCAL_READ = 1 << 2
};

struct mlx5_wqe_mkey_context_seg {
	uint8_t		free;
	uint8_t		reserved1;
	uint8_t		access_flags;
	uint8_t		sf;
	__be32		qpn_mkey;
	__be32		reserved2;
	__be32		flags_pd;
	__be64		start_addr;
	__be64		len;
	__be32		bsf_octword_size;
	__be32		reserved3[4];
	__be32		translations_octword_size;
	uint8_t		reserved4[3];
	uint8_t		log_page_size;
	__be32		reserved;
	union mlx5_wqe_umr_inline_seg inseg[0];
};
#endif

static inline int ilog2(int n)
{
	int t;

	if (n <= 0)
		return -1;

	t = 0;
	while ((1 << t) < n)
		++t;

	return t;
}

struct devx_indirect_mr : public ibvt_abstract_mr {
	struct mlx5dv_devx_obj *dvmr;
	ibvt_mr &sub_mr;
	ibvt_qp &umr_qp;
	ibvt_cq &umr_cq;
	uint32_t mkey;

	devx_indirect_mr(ibvt_mr &m, ibvt_qp &q, ibvt_cq &c) :
		ibvt_abstract_mr(m.env, m.size, m.addr),
		dvmr(NULL),
		sub_mr(m),
		umr_qp(q),
		umr_cq(c) {}

	virtual void init() {
		uint32_t in[DEVX_ST_SZ_DW(create_mkey_in) + 0x20] = {0};
		uint32_t out[DEVX_ST_SZ_DW(create_mkey_out)] = {0};
		sub_mr.init();

		buff = (char *)sub_mr.buff;

		DEVX_SET(create_mkey_in, in, opcode, MLX5_CMD_OP_CREATE_MKEY);
		set_mkc(DEVX_ADDR_OF(create_mkey_in, in, memory_key_mkey_entry));

		set_klm(in);

		SET(dvmr, mlx5dv_devx_obj_create(sub_mr.pd.ctx.ctx, in, sizeof(in), out, sizeof(out)));
		mkey = DEVX_GET(create_mkey_out, out, mkey_index) << 8 | 0x42;

		INIT(umr());
	}

	virtual void set_mkc(char *mkc) {
		struct mlx5dv_obj dv = {};
		struct mlx5dv_pd dvpd = {};

		dv.pd.in = sub_mr.pd.pd;
		dv.pd.out = &dvpd;
		mlx5dv_init_obj(&dv, MLX5DV_OBJ_PD);

		DEVX_SET(mkc, mkc, a, 1);
		DEVX_SET(mkc, mkc, rw, 1);
		DEVX_SET(mkc, mkc, rr, 1);
		DEVX_SET(mkc, mkc, lw, 1);
		DEVX_SET(mkc, mkc, lr, 1);
		DEVX_SET(mkc, mkc, pd, dvpd.pdn);
		DEVX_SET(mkc, mkc, translations_octword_size, 2);
		DEVX_SET(mkc, mkc, qpn, 0xffffff);
		DEVX_SET(mkc, mkc, mkey_7_0, 0x42);
	}

	virtual void set_klm(uint32_t *in) {
		struct mlx5_wqe_data_seg *dseg = (struct mlx5_wqe_data_seg *)DEVX_ADDR_OF(create_mkey_in, in, klm_pas_mtt);
		mlx5dv_set_data_seg(dseg, size / 2, sub_mr.mr->lkey, (intptr_t)buff);
		mlx5dv_set_data_seg(dseg + 1, size / 2, sub_mr.mr->lkey, (intptr_t)buff + size / 2);

		DEVX_SET(create_mkey_in, in, translations_octword_actual_size, 2);
	}

	virtual void umr() {
		struct mlx5_wqe_ctrl_seg *ctrl = (struct mlx5_wqe_ctrl_seg *)umr_qp.get_wqe(0);
		mlx5dv_set_ctrl_seg(ctrl, umr_qp.sqi, MLX5_OPCODE_UMR, 0,
				    umr_qp.qp->qp_num, MLX5_WQE_CTRL_CQ_UPDATE, 
				    12, 0, htobe32(mkey));

		struct mlx5_wqe_umr_ctrl_seg *umr = (struct mlx5_wqe_umr_ctrl_seg *)(ctrl + 1);
		umr->flags = MLX5_WQE_UMR_CTRL_FLAG_INLINE;
		umr->mkey_mask = htobe64(MLX5_WQE_UMR_CTRL_MKEY_MASK_LEN |
					 MLX5_WQE_UMR_CTRL_MKEY_MASK_START_ADDR |
					 MLX5_WQE_UMR_CTRL_MKEY_MASK_MKEY |
					 MLX5_WQE_UMR_CTRL_MKEY_MASK_QPN |
					 MLX5_WQE_UMR_CTRL_MKEY_MASK_ACCESS_LOCAL_WRITE |
					 MLX5_WQE_UMR_CTRL_MKEY_MASK_ACCESS_REMOTE_READ |
					 MLX5_WQE_UMR_CTRL_MKEY_MASK_ACCESS_REMOTE_WRITE |
					 MLX5_WQE_UMR_CTRL_MKEY_MASK_ACCESS_ATOMIC |
					 MLX5_WQE_UMR_CTRL_MKEY_MASK_FREE);
		umr->klm_octowords = htobe16(4);

		struct mlx5_wqe_mkey_context_seg *mk = (struct mlx5_wqe_mkey_context_seg *)umr_qp.get_wqe(1);
		mk->access_flags =
			MLX5_WQE_MKEY_CONTEXT_ACCESS_FLAGS_ATOMIC       |
			MLX5_WQE_MKEY_CONTEXT_ACCESS_FLAGS_REMOTE_WRITE |
			MLX5_WQE_MKEY_CONTEXT_ACCESS_FLAGS_REMOTE_READ  |
			MLX5_WQE_MKEY_CONTEXT_ACCESS_FLAGS_LOCAL_WRITE  |
			MLX5_WQE_MKEY_CONTEXT_ACCESS_FLAGS_LOCAL_READ;
		mk->qpn_mkey = htobe32(0xffffff42);
		mk->start_addr = htobe64((intptr_t)buff);
		mk->len = htobe64(size);

		struct mlx5_wqe_data_seg *dseg = (struct mlx5_wqe_data_seg *)umr_qp.get_wqe(2);
		mlx5dv_set_data_seg(dseg, size / 2, sub_mr.mr->lkey, (intptr_t)buff);
		mlx5dv_set_data_seg(dseg + 1, size / 2, sub_mr.mr->lkey, (intptr_t)buff + size / 2);

		umr_qp.ring_db(3);

		umr_cq.poll();
	}

	~devx_indirect_mr() {
		if (dvmr)
			mlx5dv_devx_obj_destroy(dvmr);
	}

	virtual uint32_t lkey() {
		return mkey;
	}
};

struct devx_klm_umr : public devx_indirect_mr {
	devx_klm_umr(ibvt_mr &m, ibvt_qp &q, ibvt_cq &c) : devx_indirect_mr(m, q, c) {}

	virtual void set_mkc(char *mkc) {
		devx_indirect_mr::set_mkc(mkc);

		DEVX_SET(mkc, mkc, access_mode_1_0, MLX5_MKC_ACCESS_MODE_KLMS);
		DEVX_SET(mkc, mkc, free, 1);
		DEVX_SET(mkc, mkc, umr_en, 1);
	}

	virtual void set_klm(uint32_t *in) {}
};

struct devx_klm : public devx_indirect_mr {
	devx_klm(ibvt_mr &m, ibvt_qp &q, ibvt_cq &c) : devx_indirect_mr(m, q, c) {}

	virtual void set_mkc(char *mkc) {
		devx_indirect_mr::set_mkc(mkc);

		DEVX_SET(mkc, mkc, access_mode_1_0, MLX5_MKC_ACCESS_MODE_KLMS);
		DEVX_SET64(mkc, mkc, start_addr, (intptr_t)buff);
		DEVX_SET64(mkc, mkc, len, size);
	}

	virtual void umr() {}
};

struct devx_ksm_umr : public devx_indirect_mr {
	devx_ksm_umr(ibvt_mr &m, ibvt_qp &q, ibvt_cq &c) : devx_indirect_mr(m, q, c) {}

	virtual void set_mkc(char *mkc) {
		devx_indirect_mr::set_mkc(mkc);

		DEVX_SET(mkc, mkc, access_mode_1_0, MLX5_MKC_ACCESS_MODE_KSM);
		DEVX_SET(mkc, mkc, log_entity_size, ilog2(size));
		DEVX_SET(mkc, mkc, free, 1);
		DEVX_SET(mkc, mkc, umr_en, 1);
	}

	virtual void set_klm(uint32_t *in) {}
};

struct devx_ksm : public devx_indirect_mr {
	devx_ksm(ibvt_mr &m, ibvt_qp &q, ibvt_cq &c) : devx_indirect_mr(m, q, c) {}

	virtual void set_mkc(char *mkc) {
		devx_indirect_mr::set_mkc(mkc);

		DEVX_SET(mkc, mkc, access_mode_1_0, MLX5_MKC_ACCESS_MODE_KSM);
		DEVX_SET(mkc, mkc, log_entity_size, ilog2(size));
		DEVX_SET64(mkc, mkc, start_addr, (intptr_t)buff);
		DEVX_SET64(mkc, mkc, len, size);
	}

	virtual void umr() {}
};

template <typename Mem, typename Mr>
struct odp_mem_devx : public Mem {
	ibvt_mr *fsrc;
	ibvt_mr *fdst;
	ibvt_cq umr_cq;
	ibvt_qp_rc umr_qp;

	odp_mem_devx(odp_side &s, odp_side &d) : Mem(s, d),
		fsrc(NULL), fdst(NULL),
		umr_cq(s.env, s.ctx),
		umr_qp(s.env, s.pd, umr_cq) {}

	virtual void init() {
		INIT(Mem::init());
		if (this->env.skip)
			return;
		INIT(umr_cq.init());
		INIT(umr_qp.init());
		INIT(umr_qp.connect(&umr_qp));
	}

	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) {
		EXEC(Mem::reg(src_addr, dst_addr, len));

		fsrc = (ibvt_mr *)this->psrc;
		fdst = (ibvt_mr *)this->pdst;

		SET(this->psrc, new Mr(*fsrc, umr_qp, umr_cq));
		SET(this->pdst, new Mr(*fdst, umr_qp, umr_cq));
	}

	virtual void unreg() {
		EXEC(Mem::unreg());
		if (fsrc)
			delete fsrc;
		if (fdst)
			delete fdst;
	}

	virtual void check_stats_before() { }
	virtual void check_stats_after(size_t len) { }
};

typedef odp_qp<ibvt_qp_rc, ibvt_ctx_devx> odp_rc_devx;

template <typename T>
struct odp_devx : public odp<T> { odp_devx(): odp<T>() {} };

typedef testing::Types<
	types<odp_mem_devx<odp_off, devx_klm>, odp_rc_devx, odp_send<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_off, devx_klm>, odp_rc_devx, odp_rdma_read<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_off, devx_klm>, odp_rc_devx, odp_rdma_write<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_explicit, devx_klm>, odp_rc_devx, odp_send<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_explicit, devx_klm>, odp_rc_devx, odp_rdma_read<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_explicit, devx_klm>, odp_rc_devx, odp_rdma_write<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_implicit, devx_klm>, odp_rc_devx, odp_send<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_implicit, devx_klm>, odp_rc_devx, odp_rdma_read<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_implicit, devx_klm>, odp_rc_devx, odp_rdma_write<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_off, devx_ksm>, odp_rc_devx, odp_send<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_off, devx_ksm>, odp_rc_devx, odp_rdma_read<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_off, devx_ksm>, odp_rc_devx, odp_rdma_write<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_explicit, devx_ksm>, odp_rc_devx, odp_send<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_explicit, devx_ksm>, odp_rc_devx, odp_rdma_read<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_explicit, devx_ksm>, odp_rc_devx, odp_rdma_write<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_implicit, devx_ksm>, odp_rc_devx, odp_send<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_implicit, devx_ksm>, odp_rc_devx, odp_rdma_read<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_implicit, devx_ksm>, odp_rc_devx, odp_rdma_write<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_off, devx_klm_umr>, odp_rc_devx, odp_send<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_off, devx_klm_umr>, odp_rc_devx, odp_rdma_read<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_off, devx_klm_umr>, odp_rc_devx, odp_rdma_write<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_explicit, devx_klm_umr>, odp_rc_devx, odp_send<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_explicit, devx_klm_umr>, odp_rc_devx, odp_rdma_read<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_explicit, devx_klm_umr>, odp_rc_devx, odp_rdma_write<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_implicit, devx_klm_umr>, odp_rc_devx, odp_send<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_implicit, devx_klm_umr>, odp_rc_devx, odp_rdma_read<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_implicit, devx_klm_umr>, odp_rc_devx, odp_rdma_write<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_off, devx_ksm_umr>, odp_rc_devx, odp_send<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_off, devx_ksm_umr>, odp_rc_devx, odp_rdma_read<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_off, devx_ksm_umr>, odp_rc_devx, odp_rdma_write<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_explicit, devx_ksm_umr>, odp_rc_devx, odp_send<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_explicit, devx_ksm_umr>, odp_rc_devx, odp_rdma_read<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_explicit, devx_ksm_umr>, odp_rc_devx, odp_rdma_write<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_implicit, devx_ksm_umr>, odp_rc_devx, odp_send<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_implicit, devx_ksm_umr>, odp_rc_devx, odp_rdma_read<ibvt_ctx_devx> >,
	types<odp_mem_devx<odp_implicit, devx_ksm_umr>, odp_rc_devx, odp_rdma_write<ibvt_ctx_devx> >
> odp_devx_list_spec;

TYPED_TEST_CASE(odp_devx, odp_devx_list_spec);

TYPED_TEST(odp_devx, t0_crossbound) {
	ODP_CHK_SUT(PAGE);
	EXEC(test((1ULL<<(32+1))-PAGE,
		  (1ULL<<(32+2))-PAGE,
		  PAGE * 2));
}

TYPED_TEST(odp_devx, t1_upper) {
	ODP_CHK_SUT(PAGE);
	EXEC(test(UP - 0x10000 * 1,
		  UP - 0x10000 * 2,
		  0x2000));
}

TYPED_TEST(odp_devx, t2_sequence) {
	ODP_CHK_SUT(PAGE);
	unsigned long p = 0x2000000000;
	for (int i = 0; i < 20; i++) {
		EXEC(test(p, p+PAGE, PAGE));
		p += PAGE * 2;
	}
}

TYPED_TEST(odp_devx, t3_1b) {
	ODP_CHK_SUT(PAGE);
	unsigned long p = 0x2000000000;
	EXEC(test(p, p+PAGE, PAGE, PAGE/0x10));
}

TYPED_TEST(odp_devx, t4_6M) {
	ODP_CHK_SUT(0x600000);
	EXEC(test(0,0,0x600000,0x10));
}

#endif

