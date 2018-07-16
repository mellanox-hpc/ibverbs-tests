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
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/limits.h>

#include <infiniband/verbs.h>

#if HAVE_INFINIBAND_VERBS_EXP_H
#include <infiniband/verbs_exp.h>

#define ibv_flow_attr		       ibv_exp_flow_attr
#define ibv_flow_spec_eth	       ibv_exp_flow_spec_eth
#define ibv_flow		       ibv_exp_flow
#define ibv_flow_attr		       ibv_exp_flow_attr
#define ibv_create_flow		       ibv_exp_create_flow
#define ibv_destroy_flow	       ibv_exp_destroy_flow
#define ibv_flow_attr		       ibv_exp_flow_attr
#define ibv_flow_spec		       ibv_exp_flow_spec
#define ibv_flow_attr		       ibv_exp_flow_attr
#define ibv_flow_spec_tunnel	       ibv_exp_flow_spec_tunnel
#define ibv_flow_spec_eth	       ibv_exp_flow_spec_eth
#define ibv_flow_spec_ipv4	       ibv_exp_flow_spec_ipv4
#define ibv_flow_spec_tcp_udp	       ibv_exp_flow_spec_tcp_udp
#define ibv_flow_attr		       ibv_exp_flow_attr
#define IBV_FLOW_ATTR_NORMAL	       IBV_EXP_FLOW_ATTR_NORMAL
#define ibv_flow_attr		       ibv_exp_flow_attr
#define ibv_flow_spec		       ibv_exp_flow_spec
#define IBV_FLOW_SPEC_ETH	       IBV_EXP_FLOW_SPEC_ETH
#define ibv_flow_spec_eth	       ibv_exp_flow_spec_eth
#define IBV_FLOW_SPEC_IPV4	       IBV_EXP_FLOW_SPEC_IPV4
#define IBV_FLOW_SPEC_UDP	       IBV_EXP_FLOW_SPEC_UDP
#define IBV_FLOW_SPEC_VXLAN_TUNNEL     IBV_EXP_FLOW_SPEC_VXLAN_TUNNEL
#define ibv_flow_spec_tunnel	       ibv_exp_flow_spec_tunnel
#define IBV_FLOW_SPEC_INNER	       IBV_EXP_FLOW_SPEC_INNER
#define ibv_flow		       ibv_exp_flow
#define ibv_flow_spec_type	       ibv_exp_flow_spec_type

#define IBV_FLOW_SPEC_ACTION_TAG       IBV_EXP_FLOW_SPEC_ACTION_TAG
#define ibv_flow_spec_action_tag       ibv_exp_flow_spec_action_tag

#define ibv_peer_commit		       ibv_exp_peer_commit
#define ibv_peer_commit_qp	       ibv_exp_peer_commit_qp

#define ibv_create_qp_ex	       ibv_exp_create_qp
#define ibv_qp_init_attr_ex	       ibv_exp_qp_init_attr
#define ibv_create_cq_attr_ex	       ibv_exp_cq_init_attr

#define IBV_QP_INIT_ATTR_PD	       IBV_EXP_QP_INIT_ATTR_PD
#define IBV_QP_INIT_ATTR_PEER_DIRECT   IBV_EXP_QP_INIT_ATTR_PEER_DIRECT
#define IBV_CREATE_CQ_ATTR_PEER_DIRECT IBV_EXP_CQ_INIT_ATTR_PEER_DIRECT

#define IBV_PEER_OP_FENCE	       IBV_EXP_PEER_OP_FENCE
#define IBV_PEER_OP_STORE_DWORD	       IBV_EXP_PEER_OP_STORE_DWORD
#define IBV_PEER_OP_STORE_QWORD	       IBV_EXP_PEER_OP_STORE_QWORD
#define IBV_PEER_OP_POLL_AND_DWORD     IBV_EXP_PEER_OP_POLL_AND_DWORD
#define IBV_PEER_OP_POLL_NOR_DWORD     IBV_EXP_PEER_OP_POLL_NOR_DWORD
#define IBV_PEER_OP_POLL_GEQ_DWORD     IBV_EXP_PEER_OP_POLL_NOR_DWORD

#define IBV_PEER_OP_FENCE_CAP	       IBV_EXP_PEER_OP_FENCE_CAP
#define IBV_PEER_OP_STORE_DWORD_CAP    IBV_EXP_PEER_OP_STORE_DWORD_CAP
#define IBV_PEER_OP_STORE_QWORD_CAP    IBV_EXP_PEER_OP_STORE_QWORD_CAP
#define IBV_PEER_OP_POLL_AND_DWORD_CAP IBV_EXP_PEER_OP_POLL_AND_DWORD_CAP
#define IBV_PEER_OP_POLL_NOR_DWORD_CAP IBV_EXP_PEER_OP_POLL_NOR_DWORD_CAP

#define IBV_PEER_FENCE_CPU_TO_HCA      IBV_EXP_PEER_FENCE_CPU_TO_HCA
#define IBV_PEER_FENCE_PEER_TO_HCA     IBV_EXP_PEER_FENCE_PEER_TO_HCA
#define IBV_PEER_FENCE_PEER_TO_CPU     IBV_EXP_PEER_FENCE_PEER_TO_CPU
#define IBV_PEER_FENCE_HCA_TO_PEER     IBV_EXP_PEER_FENCE_HCA_TO_PEER

#define ibv_peer_direct_attr	       ibv_exp_peer_direct_attr
#define ibv_peer_direction	       ibv_exp_peer_direction
#define ibv_peer_op		       ibv_exp_peer_op

#define IBV_ROLLBACK_ABORT_UNCOMMITED  IBV_EXP_ROLLBACK_ABORT_UNCOMMITED
#define IBV_ROLLBACK_ABORT_LATE	       IBV_EXP_ROLLBACK_ABORT_LATE

#define ibv_rollback_ctx	       ibv_exp_rollback_ctx
#define ibv_rollback_qp		       ibv_exp_rollback_qp
#define ibv_peer_peek		       ibv_exp_peer_peek
#define ibv_peer_peek_cq	       ibv_exp_peer_peek_cq
#define ibv_peer_abort_peek	       ibv_exp_peer_abort_peek
#define ibv_peer_abort_peek_cq	       ibv_exp_peer_abort_peek_cq

#define IBV_PEER_DIRECTION_FROM_CPU    IBV_EXP_PEER_DIRECTION_FROM_CPU
#define IBV_PEER_DIRECTION_FROM_HCA    IBV_EXP_PEER_DIRECTION_FROM_HCA

#define ibv_peer_buf		       ibv_exp_peer_buf
#define ibv_peer_buf_alloc_attr	       ibv_exp_peer_buf_alloc_attr
#define IBV_SEND_SIGNALED	       IBV_EXP_SEND_SIGNALED

#define ibv_create_cq_ex_(ctx, attr, n, ch) \
		ibv_exp_create_cq(ctx, n, NULL, ch, 0, attr)

#define	ibv_device_attr_ex ibv_exp_device_attr
#define ibv_query_device_(ctx, attr, attr2) ({ \
		int ret; \
		(attr)->comp_mask = IBV_EXP_DEVICE_ATTR_RESERVED - 1; \
		ret = ibv_exp_query_device(ctx, attr); \
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


#define ibv_srq_init_attr_ex		ibv_exp_create_srq_attr
#define ibv_create_srq_ex		ibv_exp_create_srq
#define ibv_dct				ibv_exp_dct

#define IBV_WR_SEND			IBV_EXP_WR_SEND
#define IBV_WR_RDMA_READ		IBV_EXP_WR_RDMA_READ
#define IBV_WR_RDMA_WRITE		IBV_EXP_WR_RDMA_WRITE
#define ibv_wr_opcode			ibv_exp_wr_opcode
#define ibv_send_flags			ibv_exp_send_flags
#define _wr_opcode			exp_opcode
#define _wr_send_flags			exp_send_flags
#define ibv_post_send			ibv_exp_post_send
#define ibv_send_wr			ibv_exp_send_wr

#define ibv_wc				ibv_exp_wc
#define ibv_poll_cq(a,b,c)		ibv_exp_poll_cq(a,b,c,sizeof(*(c)))
#define ibv_wc_opcode			ibv_exp_wc_opcode
#define _wc_opcode			exp_opcode
#define wc_flags			exp_wc_flags
#define IBV_ODP_SUPPORT_IMPLICIT	IBV_EXP_ODP_SUPPORT_IMPLICIT
#else

#define general_odp_caps		general_caps
#define ibv_create_cq_attr_ex		ibv_cq_init_attr_ex

#define ibv_create_cq_ex_(ctx, attr, n, ch) ({ \
		(attr)->cqe = n; \
		(attr)->channel = ch; \
		(attr)->wc_flags = IBV_CREATE_CQ_SUP_WC_FLAGS; \
		ibv_cq_ex_to_cq(ibv_create_cq_ex(ctx, attr)); })

#define ibv_query_device_(ctx, attr, attr2) ({ \
		int ret = ibv_query_device_ex(ctx, NULL, attr); \
		attr2 = &(attr)->orig_attr; ret ; })

#define _wr_opcode			opcode
#define _wr_send_flags			send_flags
#define _wc_opcode			opcode

#endif

#ifdef __x86_64__

#define PAGE 0x1000
#define UP (1ULL<<47)

#elif (__ppc64__|__PPC64__)

#define PAGE 0x10000
#define UP (1ULL<<46)

#endif

#define PAGE_MASK (PAGE - 1)
#define PAGE_ALIGN(x) ((x) & ~PAGE_MASK)
#define PAGE_UPALIGN(x) (((x) + PAGE_MASK) & ~PAGE_MASK)


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
			if (this->env.skip) { \
				VERBS_TRACE("%3d.%p: failed\t%s" #x " - skipping test\n", __LINE__, this, this->env.lvl_str); \
				return; \
			} \
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
		if (this->env.run) \
			ASSERT_EQ(0, x) << "errno: " << errno; \
		else if (x) { \
			VERBS_NOTICE("%3d.%p: failed\t%s" #x " - skipping test (errno %d)\n", __LINE__, this, env.lvl_str, errno); \
			this->env.skip = 1; \
			return; \
		} \
	} while(0)

#define SET(x,y) do { \
		VERBS_TRACE("%3d.%p: doing\t%s" #y "\n", __LINE__, this, env.lvl_str); \
		x=y; \
		if (this->env.run) \
			ASSERT_TRUE(x) << #y << " errno: " << errno; \
		else if (!x) { \
			VERBS_NOTICE("%3d.%p: failed\t%s" #y " - skipping test (errno %d)\n", __LINE__, this, env.lvl_str, errno); \
			this->env.skip = 1; \
			return; \
		} \
	} while(0)

#define SKIP(report) do { \
		::testing::UnitTest::GetInstance()->runtime_skip(report); \
		return; \
	} while(0)


#define CHK_SUT(FEATURE_NAME) \
	do { \
		if (this->skip) { \
			std::cout << "[  SKIPPED ] Feature " << #FEATURE_NAME << " lacks resources to be tested" << std::endl; \
			SKIP(1); \
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

#ifdef PALLADIUM
#define POLL_RETRIES 8000000000ULL
#else
#define POLL_RETRIES 80000000ULL
#endif

#define ACTIVE (1 << 0)

#define Q_KEY 0x11111111

#define DC_KEY 1

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

	char meminfo[8192];
	int ram_init;

	void init_ram() {
		int fd = open("/proc/meminfo", O_RDONLY);
		ASSERT_GT(fd, 0);
		ASSERT_GT(read(fd, meminfo, sizeof(meminfo)), 0);
		close(fd);
		ram_init = 1;
	}

	void check_ram(const char* var, long val) {
		if (!ram_init)
			ASSERT_NO_FATAL_FAILURE(init_ram());
		char *hit = strstr(meminfo, var);
		ASSERT_TRUE(hit) << var;
		if ((val >> 10) > atoi(hit + strlen(var))) {
			VERBS_NOTICE("%smeminfo %s is lower than %ld - skipping test\n",
				     lvl_str, var, val >> 10);
			skip = 1;
		}
	}

	ibvt_env() :
		env(*this),
		lvl(0),
		skip(0),
		fatality(0),
		flags(ACTIVE),
		run(0),
		ram_init(0)
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
	char *vdev_name;

#if HAVE_INFINIBAND_VERBS_EXP_H

#define DEV_FS "/sys/class/infiniband_verbs"
#define DEBUGFS "/sys/kernel/debug/mlx5"

	void init_sysfs() {
		char path[PATH_MAX];
		char pdev[PATH_MAX], *p = pdev, *tok;
		struct stat st;

		sprintf(path, "/sys/class/infiniband/%s", ibv_get_device_name(dev));
		if (readlink(path, pdev, PATH_MAX) <= 0)
			return;

		while ((tok = strsep(&p, "/"))) {
			if (tok[0] == '.')
				continue;
			sprintf(path, DEBUGFS "/%s", tok);
			if (stat(path, &st) == 0) {
				pdev_name = strdup(tok);
				break;
			}
		}

		DIR *verbs = opendir(DEV_FS);
		struct dirent *d;
		char buff[PATH_MAX];
		int fd, len;

		while ((d = readdir(verbs))) {
			sprintf(path, DEV_FS "/%s/ibdev",
				d->d_name);
			fd = open(path, O_RDONLY);
			if (fd < 0)
				continue;
			len = read(fd, buff, sizeof(buff));
			close(fd);
			buff[len-1] = 0;
			if (!strcmp(buff, ibv_get_device_name(dev))) {
				vdev_name = strdup(d->d_name);
				break;
			}
		}
		closedir(verbs);
	}

	int read_sysfs(const char* dir, char* dev_name, const char* var) {
		char path[PATH_MAX];
		char buff[PATH_MAX];
		int fd;

		sprintf(path, "%s/%s/%s", dir, dev_name, var);
		fd = open(path, O_RDONLY);

		if (fd < 0)
			return -1;
		if (read(fd, buff, sizeof(buff)) <= 0) {
			close(fd);
			return -1;
		}
		close(fd);
		return atoi(buff);
	}

	void check_debugfs(const char* var, int val) {
		if (!pdev_name)
			return;
		int val_fs = read_sysfs(DEBUGFS, pdev_name, var);
		VERBS_INFO("%3d.%p: debugfs\t%s %s = %d\n", __LINE__,
			    this, env.lvl_str, var, val_fs);
		ASSERT_GE(val_fs, val) << var;
	}

	void read_dev_fs(const char* var, int* val) {
		if (!vdev_name)
			return;
		*val = read_sysfs(DEV_FS, vdev_name, var);
		VERBS_INFO("%3d.%p: dev fs\t%s %s = %d\n", __LINE__,
			    this, env.lvl_str, var, *val);
	}

	void check_dev_fs(const char* var, int val) {
		if (!vdev_name)
			return;
		int val_fs = read_sysfs(DEV_FS, vdev_name, var);
		VERBS_INFO("%3d.%p: dev fs\t%s %s = %d\n", __LINE__,
			    this, env.lvl_str, var, val_fs);
		ASSERT_GE(val_fs, val) << var;
	}
#else
	void init_sysfs() {}
	void check_debugfs(const char* var, int val) {}
	void read_dev_fs(const char* var, int* val) {}
	void check_dev_fs(const char* var, int val) {}
#endif

	ibvt_ctx(ibvt_env &e, ibvt_ctx *o = NULL) :
		ibvt_obj(e),
		ctx(NULL),
		other(o),
		port_num(0),
		pdev_name(NULL),
		vdev_name(NULL)	{}

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
		if (!port_num) {
			VERBS_NOTICE("suitable port not found\n");
			env.skip = 1;
		}
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

struct ibvt_cq;

struct ibvt_wc {
	struct ibv_wc wc;
	ibvt_cq &cq;

	struct ibv_wc operator()() {
		return wc;
	}

	ibvt_wc(ibvt_cq &c) : cq(c) {
		memset(&wc, 0, sizeof(wc));
	}

	virtual ~ibvt_wc() ;
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

	virtual void poll() {
		ibvt_wc wc(*this);

		VERBS_TRACE("%d.%p polling...\n", __LINE__, this);
		EXEC(do_poll(wc));

		VERBS_TRACE("poll status %s(%d) opcode %d len %d qp %x lid %x flags %lx\n",
				ibv_wc_status_str(wc().status),
				wc().status, wc()._wc_opcode,
				wc().byte_len, wc().qp_num,
				wc().slid, (uint64_t)wc().wc_flags);
		//if (wc().status) getchar();
		ASSERT_FALSE(wc().status) << ibv_wc_status_str(wc().status);
	}

#if HAVE_INFINIBAND_VERBS_EXP_H
	virtual void do_poll(struct ibvt_wc &wc) {
		long result = 0, retries = POLL_RETRIES;
		errno = 0;
		while (!result && --retries) {
			result = ibv_poll_cq(cq, 1, &wc.wc);
			ASSERT_GE(result,0);
		}
		ASSERT_GT(retries,0) << "errno: " << errno;
	}
#else
	struct ibv_cq_ex *cq2() {
		return (struct ibv_cq_ex *)this->cq;
	}

	virtual void do_poll(struct ibvt_wc &wc) {
		long result = 0, retries = POLL_RETRIES;
		struct ibv_poll_cq_attr attr = {};

		errno = 0;
		while (--retries) {
			result = ibv_start_poll(cq2(), &attr);
			if (!result)
				break;
			ASSERT_EQ(ENOENT, result);
		}
		ASSERT_GT(retries,0) << "errno: " << errno;

		wc.wc.status = cq2()->status;
		wc.wc.wr_id = cq2()->wr_id;
		wc.wc.opcode = ibv_wc_read_opcode(cq2());
		wc.wc.wc_flags = ibv_wc_read_wc_flags(cq2());
		wc.wc.byte_len = ibv_wc_read_byte_len(cq2());
		wc.wc.slid = ibv_wc_read_slid(cq2());
		wc.wc.qp_num = ibv_wc_read_qp_num(cq2());

	}
#endif
	virtual void poll_arrive(int n) {
		struct ibv_wc wc[n];
		long result = 0, retries = POLL_RETRIES;

		VERBS_TRACE("%d.%p polling...\n", __LINE__, this);

		while (!result && --retries) {
			result = ibv_poll_cq(cq, n, wc);
			ASSERT_GE(result,0);
		}
		ASSERT_EQ(result,0) << "errno: " << errno;

	}
};

inline ibvt_wc::~ibvt_wc() {
#if !HAVE_INFINIBAND_VERBS_EXP_H
	ibv_end_poll(cq.cq2());
#endif
}

struct ibvt_cq_event : public ibvt_cq {
	struct ibv_comp_channel *channel;
	int num_cq_events;
	int solicited_only;

	ibvt_cq_event(ibvt_env &e, ibvt_ctx &c) :
		ibvt_cq(e, c),
		channel(NULL),
		num_cq_events(0),
		solicited_only(0) {}

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
		if (num_cq_events)
			ibv_ack_cq_events(cq, num_cq_events);
		FREE(ibv_destroy_cq, cq);
		FREE(ibv_destroy_comp_channel, channel);
	}

	virtual void arm() {
		num_cq_events = 0;
		DO(ibv_req_notify_cq(cq, solicited_only));
	}

	virtual void do_poll(struct ibvt_wc &wc) {
		struct ibv_cq *ev_cq;
		void *ev_ctx;

		DO(ibv_get_cq_event(channel, &ev_cq, &ev_ctx));
		ASSERT_EQ(ev_cq, cq);
		num_cq_events++;
		DO(ibv_req_notify_cq(cq, solicited_only));
		EXEC(ibvt_cq::do_poll(wc));
	}
};

struct ibvt_mr : public ibvt_obj {
	struct ibv_mr *mr;
	ibvt_pd &pd;
	size_t size;
	intptr_t addr;
	long access_flags;
	char *buff;

	char *mem;
	size_t mem_size;

	ibvt_mr(ibvt_env &e, ibvt_pd &p, size_t s, intptr_t a = 0,
		long af = IBV_ACCESS_LOCAL_WRITE |
			  IBV_ACCESS_REMOTE_READ |
			  IBV_ACCESS_REMOTE_WRITE) :
		ibvt_obj(e),
		mr(NULL),
		pd(p),
		size(s),
		addr(a),
		access_flags(af),
		buff(NULL),
		mem(NULL) {}

	virtual int mmap_flags() {
		return MAP_PRIVATE|MAP_ANON;
	}

	virtual void init_mmap() {
		int flags = mmap_flags();
		if (addr) {
			flags |= MAP_FIXED;
			mem_size = PAGE_UPALIGN(size + (addr & PAGE_MASK));
		} else {
			mem_size = size;
		}

		mem = (char*)mmap((void*)PAGE_ALIGN(addr),
				  mem_size, PROT_READ|PROT_WRITE,
				  flags, -1, 0);
		ASSERT_NE(mem, MAP_FAILED);

		buff = addr ? (char*)addr : mem;
	}

	virtual void init() {
		if (mr)
			return;
		EXEC(pd.init());
		EXEC(init_mmap());
		SET(mr, ibv_reg_mr(pd.pd, buff, size, access_flags));
		VERBS_TRACE("\t\t\t\tibv_reg_mr(pd, %p, %zx, %lx) = %x\n", buff, size, access_flags, mr->lkey);
	}

	virtual ~ibvt_mr() {
		FREE(ibv_dereg_mr, mr);
		if (mem && mem != MAP_FAILED)
			munmap(mem, mem_size);
	}

	virtual void fill() {
		EXEC(init());
		for (size_t i = 0; i < size; i++)
			buff[i] = i & 0xff;
		VERBS_TRACE("\t\t\t\tfill(%p, %zx, %lx) = %x\n", buff, size, access_flags, mr->lkey);
	}

	virtual void check(size_t skip = 0, size_t shift = 0, int repeat = 1, size_t length = 0) {
		if (!length)
			length = size;
		for (int n = 0; n < repeat; n++)
			for (size_t i = skip + n * (length / repeat); i < length / repeat - shift; i++)
				ASSERT_EQ((char)((i + shift) & 0xff), buff[i]) << "i=" << i;
		memset(buff, 0, size);
	}

	virtual void dump(size_t offset = 0,
			  size_t length = 0,
			  const char *pfx = "") {
		hexdump(pfx, buff + offset, length ?: size);
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

struct ibvt_srq : public ibvt_obj {
	struct ibv_srq *srq;

	ibvt_pd &pd;
	ibvt_cq &cq;

	ibvt_srq(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) :
		 ibvt_obj(e), srq(NULL), pd(p), cq(c) {}

	~ibvt_srq() {
		FREE(ibv_destroy_srq, srq);
	}

	virtual void init_attr(struct ibv_srq_init_attr_ex &attr) {
#if HAVE_INFINIBAND_VERBS_EXP_H
		attr.comp_mask =
			IBV_EXP_CREATE_SRQ_CQ;
		attr.srq_type = IBV_EXP_SRQT_BASIC;
		attr.pd = pd.pd;
		attr.cq = cq.cq;
		attr.base.attr.max_wr  = 128;
		attr.base.attr.max_sge = 1;
#else
		attr.comp_mask =
			IBV_SRQ_INIT_ATTR_TYPE |
			IBV_SRQ_INIT_ATTR_PD |
			IBV_SRQ_INIT_ATTR_CQ;
		attr.pd = pd.pd;
		attr.cq = cq.cq;
		attr.attr.max_wr  = 128;
		attr.attr.max_sge = 1;
#endif
	}

	virtual void init() {
		struct ibv_srq_init_attr_ex attr = {};
		if (srq)
			return;

		INIT(pd.init());
		INIT(cq.init());
		init_attr(attr);
		SET(srq, ibv_create_srq_ex(pd.ctx.ctx, &attr));
	}

	virtual void recv(ibv_sge sge) {
		struct ibv_recv_wr wr;
		struct ibv_recv_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0x56789;
		wr.sg_list = &sge;
		wr.num_sge = 1;
		DO(ibv_post_srq_recv(srq, &wr, &bad_wr));
	}
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

	virtual void init() {
		struct ibv_qp_init_attr_ex attr;
		INIT(pd.init());
		INIT(cq.init());
		init_attr(attr);
		SET(qp, ibv_create_qp_ex(pd.ctx.ctx, &attr));
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

	virtual void post_send(ibv_sge sge, enum ibv_wr_opcode opcode,
			       int flags = IBV_SEND_SIGNALED) {
		struct ibv_send_wr wr;
		struct ibv_send_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0;
		wr.sg_list = &sge;
		wr.num_sge = 1;
		wr._wr_opcode = opcode;
		wr._wr_send_flags = flags;
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
		wr._wr_opcode = opcode;
		wr._wr_send_flags = flags;

		wr.wr.rdma.remote_addr = dst_sge.addr;
		wr.wr.rdma.rkey = dst_sge.lkey;

		DO(ibv_post_send(qp, &wr, &bad_wr));
	}

	virtual void rdma2(ibv_sge src_sge1,
			   ibv_sge src_sge2,
			   ibv_sge dst_sge,
			   enum ibv_wr_opcode opcode,
			   enum ibv_send_flags flags = IBV_SEND_SIGNALED) {
		struct ibv_send_wr wr;
		struct ibv_send_wr *bad_wr = NULL;
		struct ibv_sge sg[2];


		sg[0] = src_sge1;
		sg[1] = src_sge2;
		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0;
		wr.sg_list = sg;
		wr.num_sge = 2;
		wr._wr_opcode = opcode;
		wr._wr_send_flags = flags;

		wr.wr.rdma.remote_addr = dst_sge.addr;
		wr.wr.rdma.rkey = dst_sge.lkey;

		DO(ibv_post_send(qp, &wr, &bad_wr));
	}

	virtual void send(ibv_sge sge) {
		post_send(sge, IBV_WR_SEND);
	}

	virtual void connect(ibvt_qp *remote) = 0;

	virtual int hdr_len() { return 0; }
};

struct ibvt_qp_rc : public ibvt_qp {
	ibvt_qp *remote;

	ibvt_qp_rc(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) : ibvt_qp(e, p, c) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr) {
		ibvt_qp::init_attr(attr);
		attr.qp_type = IBV_QPT_RC;
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

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr) {
		ibvt_qp::init_attr(attr);
		attr.qp_type = IBV_QPT_UD;
	}

	virtual void post_send(ibv_sge sge, enum ibv_wr_opcode opcode,
			       int flags = IBV_SEND_SIGNALED) {
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
		wr._wr_opcode = opcode;
		wr._wr_send_flags = flags;

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

	virtual int hdr_len() { return 40; }
};

#if HAVE_INFINIBAND_VERBS_EXP_H
struct ibvt_dct : public ibvt_obj {
	struct ibv_dct *dct;
	ibvt_pd &pd;
	ibvt_cq &cq;
	ibvt_srq &srq;

	ibvt_dct(ibvt_env &e, ibvt_pd &p, ibvt_cq &c, ibvt_srq &s) :
		ibvt_obj(e), dct(NULL), pd(p), cq(c), srq(s) {}

	virtual void init() {
		struct ibv_exp_dct_init_attr attr = {};

		if (dct)
			return;

		INIT(pd.init());
		INIT(cq.init());
		INIT(srq.init());

		attr.pd = pd.pd;
		attr.cq = cq.cq;
		attr.srq = srq.srq;
		attr.dc_key = DC_KEY;
		attr.port = pd.ctx.port_num;
		attr.access_flags = IBV_ACCESS_REMOTE_WRITE;
		attr.min_rnr_timer = 2;
		attr.mtu = IBV_MTU_512;
		attr.hop_limit = 1;
		attr.inline_size = 0;
		SET(dct, ibv_exp_create_dct(pd.ctx.ctx, &attr));
	}

	virtual void connect(ibvt_qp *remote) { }

	virtual ~ibvt_dct() {
		FREE(ibv_exp_destroy_dct, dct);
	}
};

struct ibvt_qp_dc : public ibvt_qp_ud {
	ibvt_dct* dremote;

	ibvt_qp_dc(ibvt_env &e, ibvt_pd &p, ibvt_cq &c) : ibvt_qp_ud(e, p, c) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr) {
		ibvt_qp::init_attr(attr);
		attr.qp_type = IBV_EXP_QPT_DC_INI;
	}

	virtual void connect(ibvt_dct *remote) {
		struct ibv_exp_qp_attr attr;
		long long flags;

		this->dremote = remote;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_INIT;
		attr.port_num = pd.ctx.port_num;
		attr.pkey_index = 0;
		attr.dct_key = DC_KEY;
		flags = IBV_EXP_QP_STATE |
			IBV_EXP_QP_PKEY_INDEX |
			IBV_EXP_QP_PORT |
			IBV_EXP_QP_DC_KEY;

		DO(ibv_exp_modify_qp(qp, &attr, flags));

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTR;
		attr.path_mtu = IBV_MTU_512;
		attr.ah_attr.is_global = 0;
		flags = IBV_EXP_QP_STATE | IBV_EXP_QP_PATH_MTU |
			IBV_EXP_QP_AV;


		attr.ah_attr.dlid = dremote->pd.ctx.lid;
		attr.ah_attr.sl = 0;
		attr.ah_attr.src_path_bits = 0;
		attr.ah_attr.port_num = dremote->pd.ctx.port_num;
		SET(ah, ibv_create_ah(pd.pd, &attr.ah_attr));
		DO(ibv_exp_modify_qp(qp, &attr, flags));

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTS;
		attr.timeout	    = 14;
		attr.retry_cnt	    = 7;
		attr.rnr_retry	    = 7;
		attr.max_rd_atomic  = 1;
		flags = IBV_EXP_QP_STATE | IBV_EXP_QP_TIMEOUT |
			IBV_EXP_QP_RETRY_CNT |
			IBV_EXP_QP_RNR_RETRY |
			IBV_EXP_QP_MAX_QP_RD_ATOMIC;

		DO(ibv_exp_modify_qp(qp, &attr, flags));
	}

	virtual void post_send(ibv_sge sge, enum ibv_wr_opcode opcode,
			       int flags = IBV_SEND_SIGNALED) {
		struct ibv_sge sge_ud = sge;
		struct ibv_exp_send_wr wr = {};
		struct ibv_exp_send_wr *bad_wr = NULL;

		wr.sg_list = &sge_ud;
		wr.num_sge = 1;
		wr.exp_opcode = opcode;
		wr.exp_send_flags = flags;

		wr.dc.ah = ah;
		wr.dc.dct_number = dremote->dct->dct_num;
		wr.dc.dct_access_key = DC_KEY;

		DO(ibv_exp_post_send(qp, &wr, &bad_wr));
	}

	virtual void rdma(ibv_sge src_sge, ibv_sge dst_sge, enum ibv_wr_opcode opcode, enum ibv_send_flags flags = IBV_SEND_SIGNALED) {
		struct ibv_send_wr wr;
		struct ibv_send_wr *bad_wr = NULL;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0;
		wr.sg_list = &src_sge;
		wr.num_sge = 1;
		wr._wr_opcode = opcode;
		wr._wr_send_flags = flags;

		wr.dc.ah = ah;
		wr.dc.dct_number = dremote->dct->dct_num;
		wr.dc.dct_access_key = DC_KEY;

		wr.wr.rdma.remote_addr = dst_sge.addr;
		wr.wr.rdma.rkey = dst_sge.lkey;

		DO(ibv_post_send(qp, &wr, &bad_wr));
	}
};
#endif

template <typename QP>
struct ibvt_qp_srq : public QP {
	ibvt_srq &srq;

	ibvt_qp_srq(ibvt_env &e, ibvt_pd &p, ibvt_cq &c, ibvt_srq &s) :
		    QP(e, p, c), srq(s) {}

	virtual void init_attr(struct ibv_qp_init_attr_ex &attr) {
		QP::init_attr(attr);
		attr.srq = srq.srq;
	}

};

#if HAVE_INFINIBAND_VERBS_EXP_H
struct ibvt_mw : public ibvt_mr {
	ibvt_mr &master;
	ibvt_qp &qp;

	ibvt_mw(ibvt_mr &i, intptr_t a, size_t size, ibvt_qp &q) :
		ibvt_mr(i.env, i.pd, size, a), master(i), qp(q) {}

	virtual void init() {
		intptr_t addr;
		if (mr || env.skip)
			return;
		if (master.buff) {
			addr = (intptr_t)master.buff;
			buff = master.buff;
		} else {
			EXEC(init_mmap());
			addr = (intptr_t)buff;
		}

		struct ibv_exp_create_mr_in mr_in = {};
		mr_in.pd = pd.pd;
		mr_in.attr.create_flags = IBV_EXP_MR_INDIRECT_KLMS;
		mr_in.attr.max_klm_list_size = 4;
		mr_in.attr.exp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
		mr_in.comp_mask = 0;
		SET(mr, ibv_exp_create_mr(&mr_in));

		if (env.skip)
			return;
		struct ibv_exp_mem_region mem_reg = {};
		mem_reg.base_addr = addr;
		mem_reg.length = size;
		mem_reg.mr = master.mr;

		struct ibv_exp_send_wr wr = {}, *bad_wr = NULL;
		wr.exp_opcode = IBV_EXP_WR_UMR_FILL;
		wr.ext_op.umr.umr_type = IBV_EXP_UMR_MR_LIST;
		wr.ext_op.umr.exp_access = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
		wr.ext_op.umr.modified_mr = mr;
		wr.ext_op.umr.num_mrs = 1;
		wr.ext_op.umr.mem_list.mem_reg_list = &mem_reg;
		wr.ext_op.umr.base_addr = addr;
		wr.exp_send_flags = IBV_EXP_SEND_INLINE;

		DO(ibv_exp_post_send(qp.qp, &wr, &bad_wr));
	}
};
#endif

#endif

