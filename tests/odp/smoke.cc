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

struct odp_base : public testing::Test, public ibvt_env {
	ibvt_ctx ctx;
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

template<typename QP>
struct odp_qp : public odp_trans {
	ibvt_ctx &ctx;
	ibvt_pd &pd;
	odp_side_qp<QP> src;
	odp_side_qp<QP> dst;

	odp_qp(odp_base &e) :
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

typedef odp_qp<ibvt_qp_rc> odp_rc;

#ifdef HAVE_INFINIBAND_VERBS_EXP_H
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

	odp_dc(odp_base &e) :
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
		FAIL();
	}
	virtual void poll_src() { src.cq.poll(); }
	virtual void poll_dst() { dst.cq.poll(); }

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

#ifdef HAVE_PREFETCH
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

#ifdef HAVE_INFINIBAND_VERBS_EXP_H
struct ibvt_qp_rc_umr : public ibvt_qp_rc {
	ibvt_qp_rc_umr(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) :
		ibvt_qp_rc(e, p, c) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr) {
		ibvt_qp_rc::init_attr(attr);
		attr.comp_mask |= IBV_EXP_QP_INIT_ATTR_CREATE_FLAGS;
		attr.comp_mask |= IBV_EXP_QP_INIT_ATTR_MAX_INL_KLMS;
		attr.exp_create_flags |= IBV_EXP_QP_CREATE_UMR;
		attr.max_inl_send_klms = 3;
	}
};

typedef odp_qp<ibvt_qp_rc_umr> odp_rc_umr;

struct ibvt_mw : public ibvt_mr {
	ibvt_mr &master;
	ibvt_qp &qp;

	ibvt_mw(ibvt_mr &i, intptr_t a, size_t size, ibvt_qp &q) :
		ibvt_mr(i.env, i.pd, size, a), master(i), qp(q) {}

	virtual void init() {
		if (mr)
			return;
		EXEC(init_mmap());

		struct ibv_exp_create_mr_in mr_in = {};
		mr_in.pd = pd.pd;
		mr_in.attr.create_flags = IBV_EXP_MR_INDIRECT_KLMS;
		mr_in.attr.max_klm_list_size = 4;
		mr_in.attr.exp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
		mr_in.comp_mask = 0;
		SET(mr, ibv_exp_create_mr(&mr_in));

		struct ibv_exp_mem_region mem_reg = {};
		mem_reg.base_addr = (intptr_t)buff;
		mem_reg.length = size;
		mem_reg.mr = master.mr;

		struct ibv_exp_send_wr wr = {}, *bad_wr = NULL;
		wr.exp_opcode = IBV_EXP_WR_UMR_FILL;
		wr.ext_op.umr.umr_type = IBV_EXP_UMR_MR_LIST;
		wr.ext_op.umr.exp_access = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
		wr.ext_op.umr.modified_mr = mr;
		wr.ext_op.umr.num_mrs = 1;
		wr.ext_op.umr.mem_list.mem_reg_list = &mem_reg;
		wr.ext_op.umr.base_addr = (intptr_t)buff;
		wr.exp_send_flags = IBV_EXP_SEND_INLINE;

		DO(ibv_exp_post_send(qp.qp, &wr, &bad_wr));
	}
};

struct odp_implicit_mw : public odp_implicit {
	odp_implicit_mw(odp_side &s, odp_side &d) :
		odp_implicit(s, d) {};

	virtual void reg(unsigned long src_addr, unsigned long dst_addr, size_t len) {
		SET(psrc, new ibvt_mw(simr, src_addr, len, ssrc.get_qp()));
		SET(pdst, new ibvt_mw(dimr, dst_addr, len, sdst.get_qp()));
	}
};
#endif

#define ODP_CHK_SUT(len) \
	this->check_ram("MemAvailable:", len * 3); \
	CHK_SUT(odp);

struct odp_send : public odp_base {
	odp_send():
		odp_base(0, IBV_ACCESS_LOCAL_WRITE) {}

	virtual void test(unsigned long src, unsigned long dst, size_t len, int count = 1) {
		EXEC(mem().reg(src, dst, len));
		EXEC(mem().src().fill());
		EXEC(mem().dst().init());

		EXEC(mem().check_stats_before());
		for (int i = 0; i < count; i++) {
			EXEC(trans().recv(mem().dst().sge(len/count*i, len/count)));
			EXEC(trans().send(mem().src().sge(len/count*i, len/count)));
			EXEC(trans().poll_src());
			EXEC(trans().poll_dst());
		}
		EXEC(mem().check_stats_after(len));
		EXEC(mem().dst().check());
		EXEC(mem().unreg());
	}
};

struct odp_rdma_read : public odp_base {
	odp_rdma_read():
		odp_base(IBV_ACCESS_REMOTE_READ, IBV_ACCESS_LOCAL_WRITE) {}

	virtual void test(unsigned long src, unsigned long dst, size_t len, int count = 1) {
		EXEC(mem().reg(src, dst, len));
		EXEC(mem().dst().init());
		EXEC(mem().src().fill());

		EXEC(mem().check_stats_before());
		for (int i = 0; i < count; i++) {
			EXEC(trans().rdma_dst(mem().dst().sge(len/count*i, len/count),
					 mem().src().sge(len/count*i, len/count),
					 IBV_WR_RDMA_READ));
			EXEC(trans().poll_dst());
		}
		EXEC(mem().check_stats_after(len));
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

		EXEC(mem().check_stats_before());
		for (int i = 0; i < count; i++) {
			EXEC(trans().rdma_src(mem().src().sge(len/count*i, len/count),
						mem().dst().sge(len/count*i, len/count),
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
	types<odp_explicit, odp_rc, odp_send>,
	types<odp_explicit, odp_rc, odp_rdma_read>,
	types<odp_explicit, odp_rc, odp_rdma_write>,
#ifdef HAVE_PREFETCH
	types<odp_prefetch, odp_rc, odp_send>,
	types<odp_prefetch, odp_rc, odp_rdma_read>,
	types<odp_prefetch, odp_rc, odp_rdma_write>,
#endif
	types<odp_implicit, odp_rc, odp_send>,
	types<odp_implicit, odp_rc, odp_rdma_read>,
	types<odp_implicit, odp_rc, odp_rdma_write>,
#if HAVE_DECL_IBV_ACCESS_HUGETLB
	types<odp_hugetlb, odp_rc, odp_send>,
	types<odp_hugetlb, odp_rc, odp_rdma_read>,
	types<odp_hugetlb, odp_rc, odp_rdma_write>,
#endif
#ifdef HAVE_INFINIBAND_VERBS_EXP_H
	types<odp_explicit, odp_dc, odp_rdma_write>,
	types<odp_implicit, odp_dc, odp_rdma_write>,
	types<odp_off, odp_dc, odp_send>,
	types<odp_off, odp_dc, odp_rdma_write>,

	types<odp_implicit_mw, odp_rc_umr, odp_send>,
	types<odp_implicit_mw, odp_rc_umr, odp_rdma_read>,
	types<odp_implicit_mw, odp_rc_umr, odp_rdma_write>,
#endif
	types<odp_off, odp_rc, odp_send>,
	types<odp_off, odp_rc, odp_rdma_read>,
	types<odp_off, odp_rc, odp_rdma_write>
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
	types<odp_explicit, odp_rc, odp_send>,
	types<odp_implicit, odp_rc, odp_send>,
#if HAVE_DECL_IBV_ACCESS_HUGETLB
	types<odp_hugetlb, odp_rc, odp_send>,
#endif
	types<odp_off, odp_rc, odp_send>
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

