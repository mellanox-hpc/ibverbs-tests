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
#ifndef __IBVT_ENV_H_
#define __IBVT_ENV_H_

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
#include <linux/limits.h>

#include <infiniband/verbs.h>

#ifdef HAVE_INFINIBAND_VERBS_EXP_H
#include <infiniband/verbs_exp.h>

#define ibv_flow_attr	  		ibv_exp_flow_attr
#define ibv_flow_spec_eth		ibv_exp_flow_spec_eth
#define ibv_flow			ibv_exp_flow
#define ibv_flow_attr			ibv_exp_flow_attr
#define ibv_create_flow			ibv_exp_create_flow
#define ibv_destroy_flow		ibv_exp_destroy_flow
#define ibv_flow_attr		ibv_exp_flow_attr
#define ibv_flow_spec	ibv_exp_flow_spec
#define ibv_flow_attr	ibv_exp_flow_attr
#define ibv_flow_spec_tunnel 	ibv_exp_flow_spec_tunnel
#define ibv_flow_spec_eth	ibv_exp_flow_spec_eth
#define ibv_flow_spec_ipv4	ibv_exp_flow_spec_ipv4
#define ibv_flow_spec_tcp_udp 	ibv_exp_flow_spec_tcp_udp
#define ibv_flow_attr	ibv_exp_flow_attr
#define IBV_FLOW_ATTR_NORMAL	IBV_EXP_FLOW_ATTR_NORMAL
#define ibv_flow_attr	ibv_exp_flow_attr
#define ibv_flow_spec	ibv_exp_flow_spec
#define IBV_FLOW_SPEC_ETH	IBV_EXP_FLOW_SPEC_ETH
#define ibv_flow_spec_eth	ibv_exp_flow_spec_eth
#define IBV_FLOW_SPEC_IPV4	IBV_EXP_FLOW_SPEC_IPV4
#define IBV_FLOW_SPEC_UDP	IBV_EXP_FLOW_SPEC_UDP
#define IBV_FLOW_SPEC_VXLAN_TUNNEL IBV_EXP_FLOW_SPEC_VXLAN_TUNNEL
#define ibv_flow_spec_tunnel	ibv_exp_flow_spec_tunnel
#define IBV_FLOW_SPEC_INNER	IBV_EXP_FLOW_SPEC_INNER
#define ibv_flow		ibv_exp_flow

#define IBV_FLOW_SPEC_ACTION_TAG	IBV_EXP_FLOW_SPEC_ACTION_TAG
#define ibv_flow_spec_action_tag	ibv_exp_flow_spec_action_tag

#define ibv_peer_commit			 ibv_exp_peer_commit
#define ibv_peer_commit_qp		 ibv_exp_peer_commit_qp

#define ibv_create_qp_ex		 ibv_exp_create_qp
#define ibv_qp_init_attr_ex		 ibv_exp_qp_init_attr
#define ibv_create_cq_attr_ex		 ibv_exp_cq_init_attr

#define IBV_QP_INIT_ATTR_PD		 IBV_EXP_QP_INIT_ATTR_PD
#define IBV_QP_INIT_ATTR_PEER_DIRECT	 IBV_EXP_QP_INIT_ATTR_PEER_DIRECT
#define IBV_CREATE_CQ_ATTR_PEER_DIRECT	 IBV_EXP_CQ_INIT_ATTR_PEER_DIRECT

#define IBV_PEER_OP_FENCE		 IBV_EXP_PEER_OP_FENCE
#define IBV_PEER_OP_STORE_DWORD		 IBV_EXP_PEER_OP_STORE_DWORD
#define IBV_PEER_OP_STORE_QWORD		 IBV_EXP_PEER_OP_STORE_QWORD
#define IBV_PEER_OP_POLL_AND_DWORD	 IBV_EXP_PEER_OP_POLL_AND_DWORD
#define IBV_PEER_OP_POLL_NOR_DWORD	 IBV_EXP_PEER_OP_POLL_NOR_DWORD
#define IBV_PEER_OP_POLL_GEQ_DWORD	 IBV_EXP_PEER_OP_POLL_NOR_DWORD

#define IBV_PEER_OP_FENCE_CAP		 IBV_EXP_PEER_OP_FENCE_CAP
#define IBV_PEER_OP_STORE_DWORD_CAP	 IBV_EXP_PEER_OP_STORE_DWORD_CAP
#define IBV_PEER_OP_STORE_QWORD_CAP	 IBV_EXP_PEER_OP_STORE_QWORD_CAP
#define IBV_PEER_OP_POLL_AND_DWORD_CAP	 IBV_EXP_PEER_OP_POLL_AND_DWORD_CAP
#define IBV_PEER_OP_POLL_NOR_DWORD_CAP	 IBV_EXP_PEER_OP_POLL_NOR_DWORD_CAP

#define IBV_PEER_FENCE_CPU_TO_HCA	 IBV_EXP_PEER_FENCE_CPU_TO_HCA
#define IBV_PEER_FENCE_PEER_TO_HCA	 IBV_EXP_PEER_FENCE_PEER_TO_HCA
#define IBV_PEER_FENCE_PEER_TO_CPU	 IBV_EXP_PEER_FENCE_PEER_TO_CPU
#define IBV_PEER_FENCE_HCA_TO_PEER	 IBV_EXP_PEER_FENCE_HCA_TO_PEER

#define ibv_peer_direct_attr		 ibv_exp_peer_direct_attr
#define ibv_peer_direction		 ibv_exp_peer_direction
#define ibv_peer_op			 ibv_exp_peer_op

#define IBV_ROLLBACK_ABORT_UNCOMMITED    IBV_EXP_ROLLBACK_ABORT_UNCOMMITED
#define IBV_ROLLBACK_ABORT_LATE		 IBV_EXP_ROLLBACK_ABORT_LATE

#define ibv_rollback_ctx		 ibv_exp_rollback_ctx
#define ibv_rollback_qp			 ibv_exp_rollback_qp
#define ibv_peer_peek			 ibv_exp_peer_peek
#define ibv_peer_peek_cq		 ibv_exp_peer_peek_cq
#define ibv_peer_abort_peek		 ibv_exp_peer_abort_peek
#define ibv_peer_abort_peek_cq		 ibv_exp_peer_abort_peek_cq

#define IBV_PEER_DIRECTION_FROM_CPU	 IBV_EXP_PEER_DIRECTION_FROM_CPU
#define IBV_PEER_DIRECTION_FROM_HCA	 IBV_EXP_PEER_DIRECTION_FROM_HCA

#define ibv_peer_buf			 ibv_exp_peer_buf
#define ibv_peer_buf_alloc_attr		 ibv_exp_peer_buf_alloc_attr

#define ibv_create_cq_ex_(ctx, attr, n, ch) \
		ibv_exp_create_cq(ctx, n, NULL, ch, 0, attr)

#define	ibv_device_attr_ex ibv_exp_device_attr
#define ibv_query_device_(ctx, attr, attr2) ({ \
		int ret = ibv_exp_query_device(ctx, attr); \
		attr2 = (typeof attr2)(attr); ret ; })

#define IBV_ACCESS_ON_DEMAND		IBV_EXP_ACCESS_ON_DEMAND

#define ibv_reg_mr(p, a, len, access) ({ \
		struct ibv_exp_reg_mr_in in; \
		in.pd = p; \
		in.addr = a; \
		in.length = len; \
		in.exp_access = access; \
		in.comp_mask = 0; \
		ibv_exp_reg_mr(&in); })

#else

#define ibv_create_cq_attr_ex		 ibv_cq_init_attr_ex

#define ibv_create_cq_ex_(ctx, attr, n, ch) ({ \
		(attr)->cqe = n; \
		(attr)->channel = ch; \
		ibv_cq_ex_to_cq(ibv_create_cq_ex(ctx, attr)); })

#define ibv_query_device_(ctx, attr, attr2) ({ \
		int ret = ibv_query_device_ex(ctx, NULL, attr); \
		attr2 = &(attr)->orig_attr; ret ; })

#endif

#include "common.h"

#define EXEC(x) do { \
		VERBS_TRACE("%3d.%p: execute\t%s" #x "\n", __LINE__, this, this->env.lvl_str); \
		this->env.lvl_str[this->env.lvl++] = ' '; \
		ASSERT_NO_FATAL_FAILURE(this->x); \
		this->env.lvl_str[--this->env.lvl] = 0; \
	} while(0)

#define INIT(x) do { \
		if (!this->env.skip) { \
			VERBS_TRACE("%3d.%p: initialize\t%s" #x "\n", __LINE__, this, this->env.lvl_str); \
			this->env.lvl_str[this->env.lvl++] = ' '; \
			EXPECT_NO_FATAL_FAILURE(this->x); \
			this->env.lvl_str[--this->env.lvl] = 0; \
		} \
	} while(0)

#define EXECL(x) do { \
		VERBS_TRACE("%3d.%p: execute\t%s" #x "\n", __LINE__, this, this->env.lvl_str); \
		this->env.lvl_str[this->env.lvl++] = ' '; \
		ASSERT_NO_FATAL_FAILURE(x); \
		this->env.lvl_str[--this->env.lvl] = 0; \
	} while(0)

#define DO(x) do { \
		VERBS_TRACE("%3d.%p: doing\t%s" #x "\n", __LINE__, this, env.lvl_str); \
		ASSERT_EQ(0, x) << "errno: " << errno; \
	} while(0)

#define SET(x,y) do { \
		VERBS_TRACE("%3d.%p: doing\t%s" #y "\n", __LINE__, this, env.lvl_str); \
		x=y; \
		if (this->env.run) \
			ASSERT_TRUE(x) << #y << " errno: " << errno; \
		else if (!x) { \
			VERBS_TRACE("%3d.%p: failed\t%s" #y " - skipping test\n", __LINE__, this, env.lvl_str); \
			this->env.skip = 1; \
		} \
	} while(0)

#define SKIP() \
	do { \
		::testing::UnitTest::GetInstance()->runtime_skip(); \
	} while(0)

#define CHK_SUT(FEATURE_NAME) \
	do { \
		if (this->skip) { \
			std::cout << "[  SKIPPED ] Feature " << #FEATURE_NAME << " lacks resources to be tested" << std::endl; \
			SKIP(); \
			return; \
		} \
		this->run = 1; \
	} while(0);


#define FREE(x,y) do { \
		if (y) { \
			VERBS_TRACE("%3d.%p: freeing\t%s" #x "\n", __LINE__, this, env.lvl_str); \
			if (x(y)) { \
				ADD_FAILURE() << "errno: " << errno; \
				env.fatality = 1; \
			} \
			y = NULL; \
		} \
	} while(0)

#define POLL_RETRIES 80000000

#define ACTIVE (1 << 0)

#define Q_KEY 0x11111111

static void hexdump(const char *pfx, void *buff, size_t len)  __attribute__ ((unused));
static void hexdump(const char *pfx, void *buff, size_t len) {
	unsigned char *p = (unsigned char*)buff, *end = (unsigned char*)buff + len, c;

	while(p < end) {
		printf("%4s %p: ", pfx, p /*- (unsigned char *)buff*/);
		for (c=0; c<16; c++)
			printf( p+c < end ? "%02x " : "   ", p[c]);
		for (c=0; c<16; c++)
			printf( p+c < end ? "%c" : " ", p[c] >= 32 && p[c] < 128 ? p[c] : '.');
		p += 16;
		printf("\n");
	}
}

struct ibvt_env {
	ibvt_env &env;
	char lvl_str[256];
	int lvl;
	int skip;
	int fatality;
	int flags;
	int run;

	ibvt_env() :
		env(*this),
		lvl(0),
		skip(0),
		fatality(0),
		flags(ACTIVE),
		run(0)
	{
		memset(lvl_str, 0, sizeof(lvl_str));
	}
};

struct ibvt_obj {
	ibvt_env &env;

	ibvt_obj(ibvt_env &e) : env(e) { }

	virtual void init() = 0;
};

struct ibvt_ctx : public ibvt_obj {
	struct ibv_context *ctx;
	ibvt_ctx *other;
	struct ibv_device *dev;
	struct ibv_device_attr *dev_attr_orig;
	struct ibv_device_attr_ex dev_attr;
	uint8_t port_num;
	uint16_t lid;
	char *pdev_name;

	ibvt_ctx(ibvt_env &e, ibvt_ctx *o = NULL) :
		ibvt_obj(e),
		ctx(NULL),
		other(o),
		port_num(0),
		pdev_name(NULL) {}

	void init_debugfs() {
		char path[PATH_MAX];
		char pdev[PATH_MAX], *p = pdev, *tok;
		struct stat st;

		sprintf(path, "/sys/class/infiniband/%s", ibv_get_device_name(dev));
		readlink(path, pdev, PATH_MAX);
		while ((tok = strsep(&p, "/"))) {
			if (tok[0] == '.')
				continue;
			sprintf(path, "/sys/kernel/debug/mlx5/%s", tok);
			if (stat(path, &st) == 0) {
				pdev_name = strdup(tok);
				return;
			}
		}
	}

	void check_debugfs(const char* var, int val) {
		char path[PATH_MAX];
		char buff[PATH_MAX];
		int fd;

		if (!pdev_name)
			return;

		sprintf(path, "/sys/kernel/debug/mlx5/%s/%s", pdev_name, var);
		fd = open(path, O_RDONLY);

		if (fd < 0)
			return;
		read(fd, buff, sizeof(buff));
		close(fd);

		VERBS_TRACE("%3d.%p: debugfs\t%s %s = %s\n", __LINE__,
			    this, env.lvl_str, var, buff);
		ASSERT_EQ(val, atoi(buff)) << var;
	}

	virtual bool check_port(struct ibv_device *dev, struct ibv_port_attr &port_attr ) {
		if (getenv("IBV_DEV") && strcmp(ibv_get_device_name(dev), getenv("IBV_DEV")))
			return false;
		if (port_attr.state == IBV_PORT_ACTIVE)
			return true;
		return false;
	}

	virtual void init() {
		struct ibv_port_attr port_attr;
		struct ibv_device **dev_list = NULL;
		int num_devices;
		if (ctx)
			return;

		dev_list = ibv_get_device_list(&num_devices);
		for (int dev = 0; dev < num_devices; dev++) {
			if (other && other->dev == dev_list[dev])
				continue;
			SET(ctx, ibv_open_device(dev_list[dev]));
			memset(&dev_attr, 0, sizeof(dev_attr));
			DO(ibv_query_device_(ctx, &dev_attr, dev_attr_orig));
			for (int port = 1; port <= dev_attr_orig->phys_port_cnt; port++) {
				DO(ibv_query_port(ctx, port, &port_attr));
				if (!check_port(dev_list[dev], port_attr))
					continue;

				port_num = port;
				lid = port_attr.lid;
				break;
			}
			if (port_num) {
				this->dev = dev_list[dev];
				break;
			} else {
				DO(ibv_close_device(ctx));
				ctx = NULL;
			}
		}
		ibv_free_device_list(dev_list);
		if (!port_num)
			env.skip = 1;
	}

	virtual ~ibvt_ctx() {
		FREE(ibv_close_device, ctx);
	}
};

struct ibvt_pd : public ibvt_obj {
	struct ibv_pd *pd;
	ibvt_ctx &ctx;

	ibvt_pd(ibvt_env &e, ibvt_ctx &c) : ibvt_obj(e), pd(NULL), ctx(c) {}

	virtual void init() {
		if (pd)
			return;
		EXEC(ctx.init());
		SET(pd, ibv_alloc_pd(ctx.ctx));
	}

	virtual ~ibvt_pd() {
		FREE(ibv_dealloc_pd, pd);
	}
};

struct ibvt_cq : public ibvt_obj {
	struct ibv_cq *cq;
	ibvt_ctx &ctx;

	ibvt_cq(ibvt_env &e, ibvt_ctx &c) : ibvt_obj(e), cq(NULL), ctx(c) {}

	virtual void init_attr(struct ibv_create_cq_attr_ex &attr, int &cqe) {
		memset(&attr, 0, sizeof(attr));
		cqe = 0x1000;
	}

	virtual void init() {
		struct ibv_create_cq_attr_ex attr;
		int cqe;
		if (cq)
			return;
		EXEC(ctx.init());
		init_attr(attr, cqe);
		SET(cq, ibv_create_cq_ex_(ctx.ctx, &attr, cqe, NULL));
	}

	virtual ~ibvt_cq() {
		FREE(ibv_destroy_cq, cq);
	}

	virtual void arm() {}

	virtual void poll(int n) {
		struct ibv_wc wc[n];
		int result = 0, retries = POLL_RETRIES;

		VERBS_TRACE("%d.%p polling...\n", __LINE__, this);

		while (!result && --retries) {
			result = ibv_poll_cq(cq, n, wc);
			ASSERT_GE(result,0);
		}
		ASSERT_GT(retries,0) << "errno: " << errno;

		for (int i=0; i<result; i++) {
			VERBS_TRACE("poll status %s(%d) opcode %d len %d qp %x lid %x\n",
					ibv_wc_status_str(wc[i].status),
					wc[i].status, wc[i].opcode, wc[i].byte_len, wc[i].qp_num, wc[i].slid);
			ASSERT_FALSE(wc[i].status) << ibv_wc_status_str(wc[i].status);
		}
	}
	virtual void poll_arrive(int n) {
		struct ibv_wc wc[n];
		int result = 0, retries = POLL_RETRIES;

		VERBS_TRACE("%d.%p polling...\n", __LINE__, this);

		while (!result && --retries) {
			result = ibv_poll_cq(cq, n, wc);
			ASSERT_GE(result,0);
		}
		ASSERT_EQ(result,0) << "errno: " << errno;

	}
};

struct ibvt_cq_event : public ibvt_cq {
	struct ibv_comp_channel *channel;
	int num_cq_events;

	ibvt_cq_event(ibvt_env &e, ibvt_ctx &c) :
		ibvt_cq(e, c), channel(NULL), num_cq_events(0) {}

	virtual void init() {
		struct ibv_create_cq_attr_ex attr;
		int cqe;
		if (cq)
			return;
		EXEC(ctx.init());
		SET(channel, ibv_create_comp_channel(ctx.ctx));
		init_attr(attr, cqe);
		SET(cq, ibv_create_cq_ex_(ctx.ctx, &attr, cqe, channel));
	}

	virtual ~ibvt_cq_event() {
		if (cq)
			ibv_ack_cq_events(cq, num_cq_events);
		FREE(ibv_destroy_cq, cq);
		FREE(ibv_destroy_comp_channel, channel);
	}

	virtual void arm() {
		num_cq_events = 0;
		DO(ibv_req_notify_cq(cq, 0));
	}

	virtual void poll(int n) {
		struct ibv_cq *ev_cq;
		void *ev_ctx;

		DO(ibv_get_cq_event(channel, &ev_cq, &ev_ctx));
		ASSERT_EQ(ev_cq, cq);
		num_cq_events++;
		DO(ibv_req_notify_cq(cq, 0));
		EXEC(ibvt_cq::poll(n));
	}
};

struct ibvt_mr : public ibvt_obj {
	struct ibv_mr *mr;
	ibvt_pd &pd;
	size_t size;
	intptr_t addr;
	long access_flags;
	char *buff;

	ibvt_mr(ibvt_env &e, ibvt_pd &p, size_t s, intptr_t a = 0, long af = IBV_ACCESS_LOCAL_WRITE|IBV_ACCESS_REMOTE_READ|IBV_ACCESS_REMOTE_WRITE) :
		ibvt_obj(e),
		mr(NULL),
		pd(p),
		size(s),
		addr(a),
		access_flags(af),
		buff(NULL) {}

	virtual int mmap_flags() {
		return MAP_PRIVATE|MAP_ANON;
	}

	virtual void init() {
		int flags = mmap_flags();
		if (mr)
			return;
		EXEC(pd.init());
		buff = (char*)mmap((void*)addr, size, PROT_READ|PROT_WRITE, flags, -1, 0);
		ASSERT_NE(buff, MAP_FAILED);
		memset(buff, 0, size);
		SET(mr, ibv_reg_mr(pd.pd, buff, size, access_flags));
		VERBS_TRACE("\t\t\t\tibv_reg_mr(pd, %p, %zx, %lx) = %x\n", buff, size, access_flags, mr->lkey);
	}

	virtual ~ibvt_mr() {
		FREE(ibv_dereg_mr, mr);
		munmap(buff, size);
	}

	virtual void fill() {
		EXEC(init());
		for (size_t i = 0; i < size; i++)
			buff[i] = i & 0xff;
	}

	virtual void check(size_t skip = 0, size_t shift = 0, int repeat = 1) {
		for (int n = 0; n < repeat; n++)
			for (size_t i = skip + n * (size / repeat); i < size / repeat - shift; i++)
				ASSERT_EQ((char)((i + shift) & 0xff), buff[i]) << "i=" << i;
		memset(buff, 0, size);
	}

	virtual void dump(const char *pfx = "") {
		hexdump(pfx, buff, size);
	}

	virtual struct ibv_sge sge(intptr_t start, size_t length) {
		struct ibv_sge ret;

		memset(&ret, 0, sizeof(ret));
		ret.addr = (intptr_t)buff + start;
		ret.length = length;
		ret.lkey = mr->lkey;

		return ret;
	}

	virtual struct ibv_sge sge() { return sge(0, size); }
};

struct ibvt_mr_hdr : public ibvt_mr {
	size_t hdr_size;

	ibvt_mr_hdr(ibvt_env &e, ibvt_pd &p, size_t s, size_t h) : ibvt_mr(e, p, s), hdr_size(h) {}

	virtual void init() {
		EXEC(ibvt_mr::init());
		//memset(buff, 0xff, size);
	}

	virtual void fill() {
		EXEC(ibvt_mr::init());
		for (size_t i = 0; i < size; i++)
			buff[i] = (i & 0xff) | 0x80;
	}

	virtual void check() {
		size_t in_hdr = 0;
		for (size_t i = 0; i < size; i++) {
			if (in_hdr)
				in_hdr--;
			else if (0x80 & buff[i])
				ASSERT_EQ((char)((i & 0xff) | 0x80), buff[i]) << "i=" << i;
			else {
				VERBS_TRACE("ibvt_mr_hdr skipping header at 0x%zx\n", i);
				in_hdr = hdr_size - 1;
			}
		}
		memset(buff, 0, size);
	}
};

struct ibvt_mr_ud : public ibvt_mr_hdr {
	ibvt_mr_ud(ibvt_env &e, ibvt_pd &p, size_t s) :
		ibvt_mr_hdr(e, p, s, 40) {}
};

struct ibvt_qp : public ibvt_obj {
	struct ibv_qp *qp;
	ibvt_pd &pd;
	ibvt_cq &cq;

	ibvt_qp(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) : ibvt_obj(e), qp(NULL), pd(p), cq(c) {}

	virtual ~ibvt_qp() {
		FREE(ibv_destroy_qp, qp);
	}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr) {
		memset(&attr, 0, sizeof(attr));
		attr.cap.max_send_wr = 0x1000;
		attr.cap.max_recv_wr = 0x1000;
		attr.cap.max_send_sge = 1;
		attr.cap.max_recv_sge = 1;
		attr.send_cq = cq.cq;
		attr.recv_cq = cq.cq;
		attr.pd = pd.pd;
		attr.comp_mask = IBV_QP_INIT_ATTR_PD;
	}

	virtual void recv(ibv_sge sge) {
		struct ibv_recv_wr wr;
		struct ibv_recv_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0;
		wr.sg_list = &sge;
		wr.num_sge = 1;
		DO(ibv_post_recv(qp, &wr, &bad_wr));
	}

	virtual void post_send(ibv_sge sge, enum ibv_wr_opcode opcode) {
		struct ibv_send_wr wr;
		struct ibv_send_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0;
		wr.sg_list = &sge;
		wr.num_sge = 1;
		wr.opcode = opcode;
		wr.send_flags = IBV_SEND_SIGNALED;
		DO(ibv_post_send(qp, &wr, &bad_wr));
	}

	virtual void rdma(ibv_sge src_sge, ibv_sge dst_sge, enum ibv_wr_opcode opcode, enum ibv_send_flags flags = IBV_SEND_SIGNALED) {
		struct ibv_send_wr wr;
		struct ibv_send_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0;
		wr.sg_list = &src_sge;
		wr.num_sge = 1;
		wr.opcode = opcode;
		wr.send_flags = flags;

		wr.wr.rdma.remote_addr = dst_sge.addr;
		wr.wr.rdma.rkey = dst_sge.lkey;

		DO(ibv_post_send(qp, &wr, &bad_wr));
	}

	virtual void send(ibv_sge sge) {
		post_send(sge, IBV_WR_SEND);
	}
};

struct ibvt_qp_rc : public ibvt_qp {
	ibvt_qp *remote;

	ibvt_qp_rc(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) : ibvt_qp(e, p, c) {}

	virtual void init() {
		struct ibv_qp_init_attr_ex attr;
		EXEC(pd.init());
		EXEC(cq.init());
		init_attr(attr);
		attr.qp_type = IBV_QPT_RC;
		SET(qp, ibv_create_qp_ex(pd.ctx.ctx, &attr));
	}

	virtual void connect(ibvt_qp *remote) {
		struct ibv_qp_attr attr;
		int flags;

		this->remote = remote;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_INIT;
		attr.port_num = pd.ctx.port_num;
		attr.pkey_index = 0;
		attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
		flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
		DO(ibv_modify_qp(qp, &attr, flags));

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTR;
		attr.path_mtu = IBV_MTU_512;
		attr.dest_qp_num = remote->qp->qp_num;
		attr.rq_psn = 0;
		attr.max_dest_rd_atomic = 1;
		attr.min_rnr_timer = 12;
		attr.ah_attr.is_global = 0;
		attr.ah_attr.dlid = remote->pd.ctx.lid;
		attr.ah_attr.sl = 0;
		attr.ah_attr.src_path_bits = 0;
		attr.ah_attr.port_num = remote->pd.ctx.port_num;
		flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
			IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
		DO(ibv_modify_qp(qp, &attr, flags));

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTS;
		attr.timeout = 14;
		attr.retry_cnt = 7;
		attr.rnr_retry = 7;
		attr.sq_psn = 0;
		attr.max_rd_atomic = 1;
		flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
			IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;

		DO(ibv_modify_qp(qp, &attr, flags));
	}
};

struct ibvt_qp_ud : public ibvt_qp_rc {
	struct ibv_ah *ah;

	ibvt_qp_ud(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) : ibvt_qp_rc(e, p, c) {}

	virtual void init() {
		struct ibv_qp_init_attr_ex attr;
		EXEC(pd.init());
		EXEC(cq.init());
		init_attr(attr);
		attr.qp_type = IBV_QPT_UD;
		SET(qp, ibv_create_qp_ex(pd.ctx.ctx, &attr));
	}

	virtual void post_send(ibv_sge sge, enum ibv_wr_opcode opcode) {
		struct ibv_sge sge_ud = sge;
		struct ibv_send_wr wr;
		struct ibv_send_wr *bad_wr = NULL;

		sge_ud.addr += 40;
		sge_ud.length -= 40;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0;
		wr.sg_list = &sge_ud;
		wr.num_sge = 1;
		wr.opcode = opcode;
		wr.send_flags = IBV_SEND_SIGNALED;

		wr.wr.ud.ah = ah;
		wr.wr.ud.remote_qpn = remote->qp->qp_num;
		wr.wr.ud.remote_qkey = Q_KEY;

		DO(ibv_post_send(qp, &wr, &bad_wr));
	}

	virtual void connect(ibvt_qp *remote) {
		struct ibv_qp_attr attr;
		int flags;

		this->remote = remote;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_INIT;
		attr.port_num = pd.ctx.port_num;
		attr.pkey_index = 0;
		attr.qkey = Q_KEY;
		flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_QKEY;
		DO(ibv_modify_qp(qp, &attr, flags));

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTR;
		flags = IBV_QP_STATE;
		DO(ibv_modify_qp(qp, &attr, flags));

		attr.ah_attr.is_global = 0;
		attr.ah_attr.dlid = remote->pd.ctx.lid;
		attr.ah_attr.sl = 0;
		attr.ah_attr.src_path_bits = 0;
		attr.ah_attr.port_num = remote->pd.ctx.port_num;
		SET(ah, ibv_create_ah(pd.pd, &attr.ah_attr));

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTS;
		attr.sq_psn = 0;
		flags = IBV_QP_STATE | IBV_QP_SQ_PSN;

		DO(ibv_modify_qp(qp, &attr, flags));
	}
};

#endif

