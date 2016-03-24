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
#ifndef __PEER_DIRECT_ENV_H_
#define __PEER_DIRECT_ENV_H_

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

#ifdef PEER_DIRECT_EXP
#include <infiniband/verbs_exp.h>

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

#else

#define ibv_create_cq_ex_(ctx, attr, n, ch) ({ \
		(attr)->cqe = n; \
		(attr)->channel = ch; \
		ibv_create_cq_ex(ctx, attr); })

#endif

#include <infiniband/peer_ops.h>
#include <infiniband/arch.h>
#include <list>
#include "common.h"

#define EXEC(x) do { \
		VERBS_TRACE("%3d.%p: execute\t%s" #x "\n", __LINE__, this, this->lvl_str); \
		this->lvl_str[this->lvl++] = ' '; \
		ASSERT_NO_FATAL_FAILURE(x); \
		this->lvl_str[--this->lvl] = 0; \
	} while(0)

#define DO(x) do { \
		VERBS_TRACE("%3d.%p.%d: doing\t%s" #x "\n", __LINE__, this, i, this->lvl_str); \
		ASSERT_EQ(0, x) << "errno: " << errno; \
	} while(0)

#define SET(x,y) do { \
		VERBS_TRACE("%3d.%p.%d: doing\t%s" #y "\n", __LINE__, this, i, this->lvl_str); \
		x=y; \
		ASSERT_TRUE(x) << #y << " errno: " << errno; \
	} while(0)
#define FREE(x,y) do { \
		if (x) { \
			VERBS_TRACE("%d.%p.%d: destroying " #y "(" #x ")\n", __LINE__, this, i); \
			ASSERT_EQ(0, y(x)) << "errno: " << errno; \
		} \
		x = NULL; \
	} while(0)

#define SZ 1024
#define MAX_WR 8

#define SENDER 0
#define RECEIVER 1

#define POLL_RETRIES 2147483

class ibverbs_env {
public:
	enum {
		PAIR = 2,
		GRH_LENGTH = 40,
		Q_KEY = 0x11111111,
		PATTERN = 0x5a,
	};

	struct ibv_port_attr port_attr[PAIR];
	struct ibv_context *context[PAIR];
	struct ibv_pd *domain[PAIR];
	struct ibv_cq *queue[PAIR];
	struct ibv_qp *queue_pair[PAIR];
	struct ibv_mr *region[PAIR];
	struct ibv_ah *address[PAIR];
	struct ibv_comp_channel *channel[PAIR];
	char buff[PAIR][SZ];
	char lvl_str[256];
	int lvl;

	ibverbs_env() {
		memset(lvl_str, 0, sizeof(lvl_str));
		lvl = 0;
	}

	virtual void poll(int i, int n) {
		struct ibv_wc wc[n];
		int result = 0, retries = POLL_RETRIES;

		VERBS_TRACE("%d.%p.%d polling...\n", __LINE__, this, i);

		while (!result && --retries) {
			result = ibv_poll_cq(queue[i], n, wc);
			ASSERT_GE(result,0);
		}
		ASSERT_GT(retries,0);

		for (int i=0; i<result; i++) {
			VERBS_TRACE("poll status %s(%d) opcode %d len %d qp %x lid %x\n",
					ibv_wc_status_str(wc[i].status),
					wc[i].status, wc[i].opcode, wc[i].byte_len, wc[i].qp_num, wc[i].slid);
			ASSERT_EQ(0, wc[i].status);
		}
	}

	virtual void xmit(int i, int start, int length) {
		struct ibv_send_wr wr;
		struct ibv_sge sge;
		struct ibv_send_wr *bad_wr = NULL;

		memset(&sge, 0, sizeof(sge));
		sge.addr = (uintptr_t)buff[i]+start;
		sge.length = length;
		sge.lkey = region[i]->lkey;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0;
		wr.sg_list = &sge;
		wr.num_sge = 1;
		wr.opcode = IBV_WR_SEND;
		wr.send_flags = IBV_SEND_SIGNALED;
		DO(ibv_post_send(queue_pair[i], &wr, &bad_wr));
	}

	virtual void recv(int i, int start, int length) {
		struct ibv_recv_wr wr;
		struct ibv_sge sge;
		struct ibv_recv_wr *bad_wr = NULL;

		memset(&sge, 0, sizeof(sge));
		sge.addr = (uintptr_t)buff[i]+start;
		sge.length = length;
		sge.lkey = region[i]->lkey;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0;
		wr.sg_list = &sge;
		wr.num_sge = 1;
		DO(ibv_post_recv(queue_pair[i], &wr, &bad_wr));
	}

	virtual enum ibv_qp_type qp_type() = 0;

	virtual void init_qp(int i) {
		struct ibv_qp_init_attr attr;
		SET(channel[i], ibv_create_comp_channel(context[i]));
		SET(queue[i], ibv_create_cq(context[i], 2, NULL, channel[i], 0)); /* NB: cqe 1 will unveil bariers porblems */
		SET(region[i], ibv_reg_mr(domain[i], buff[i], SZ,
					IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE));

		memset(&attr, 0, sizeof(attr));
		attr.qp_type = qp_type();
		attr.sq_sig_all = 1;
		attr.send_cq = queue[i];
		attr.recv_cq = queue[i];
		attr.cap.max_send_wr = 50;
		attr.cap.max_recv_wr = 50;
		attr.cap.max_send_sge = 1;
		attr.cap.max_recv_sge = 1;
		SET(queue_pair[i], ibv_create_qp(domain[i], &attr));
	}

	virtual void arm_qp(int i) {}

	virtual void to_init(int i) = 0;

	virtual void to_rtr(int i) = 0;

	virtual void to_rts(int i) = 0;

	virtual void init() {
		int num_devices;
		struct ibv_device **dev_list = NULL;
		int i = 0;

		SET(dev_list, ibv_get_device_list(&num_devices));

		if (num_devices == 1) {
			dev_list[1] = dev_list[0];
			num_devices++;
		}

		ASSERT_EQ(PAIR, num_devices);

		for (i=0; i<PAIR; i++) {
			SET(context[i], ibv_open_device(dev_list[i^1]));
			DO(ibv_query_port(context[i], 1, &port_attr[i]));
			SET(domain[i], ibv_alloc_pd(context[i]));
			channel[i] = NULL;
		}

		ibv_free_device_list(dev_list);

		EXEC(init_qp(SENDER));
		EXEC(init_qp(RECEIVER));

		EXEC(arm_qp(SENDER));
		EXEC(arm_qp(RECEIVER));

		EXEC(to_init(SENDER));
		EXEC(to_init(RECEIVER));

		EXEC(to_rtr(SENDER));
		EXEC(to_rtr(RECEIVER));

		EXEC(to_rts(SENDER));
		EXEC(to_rts(RECEIVER));
	}

	virtual void setup_buff(int i, int j) {
		memset(this->buff[i], this->PATTERN, SZ);
		memset(this->buff[j], 0, SZ);
	}

	virtual void check_fin(int i, int n) = 0;

	virtual void fini() {
		int i;
		for (i = 0; i < PAIR; i++) {
			FREE(queue_pair[i], ibv_destroy_qp);
			FREE(region[i], ibv_dereg_mr);
			FREE(queue[i], ibv_destroy_cq);
			FREE(domain[i], ibv_dealloc_pd);
			FREE(channel[i], ibv_destroy_comp_channel);
			FREE(context[i], ibv_close_device);
		}
	}

};

class ibverbs_env_rc : public virtual ibverbs_env {
public:

	virtual enum ibv_qp_type qp_type() { return IBV_QPT_RC; }

	virtual void to_init(int i) {
		struct ibv_qp_attr attr;
		int flags;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_INIT;
		attr.port_num = 1;
		attr.pkey_index = 0;
		attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
		flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
		DO(ibv_modify_qp(queue_pair[i], &attr, flags));
	}

	virtual void to_rtr(int i) {
		struct ibv_qp_attr attr;
		int flags;
		uint32_t remote_qpn = queue_pair[i^1]->qp_num;
		uint16_t dlid = port_attr[i^1].lid;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTR;
		attr.path_mtu = IBV_MTU_512;
		attr.dest_qp_num = remote_qpn;
		attr.rq_psn = 0;
		attr.max_dest_rd_atomic = 1;
		attr.min_rnr_timer = 12;
		attr.ah_attr.is_global = 0;
		attr.ah_attr.dlid = dlid;
		attr.ah_attr.sl = 0;
		attr.ah_attr.src_path_bits = 0;
		attr.ah_attr.port_num = 1;
		flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
			IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
		DO(ibv_modify_qp(queue_pair[i], &attr, flags));
	}

	virtual void to_rts(int i) {
		struct ibv_qp_attr attr;
		int flags;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTS;
		attr.timeout = 14;
		attr.retry_cnt = 7;
		attr.rnr_retry = 7;
		attr.sq_psn = 0;
		attr.max_rd_atomic = 1;
		flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
			IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;

		DO(ibv_modify_qp(queue_pair[i], &attr, flags));
	}

	virtual void check_fin(int i, int n) {
		int k;

		EXEC(poll(i,n));
		for (k = 0; k < SZ; k++)
			ASSERT_EQ(PATTERN, buff[i][k]) << "k=" << k;
	}
};

class ibverbs_env_uc : public virtual ibverbs_env {
public:

	virtual enum ibv_qp_type qp_type() { return IBV_QPT_UC; }

	virtual void to_init(int i) {
		struct ibv_qp_attr attr;
		int flags;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_INIT;
		attr.port_num = 1;
		attr.pkey_index = 0;
		attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
		flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
		DO(ibv_modify_qp(queue_pair[i], &attr, flags));
	}

	virtual void to_rtr(int i) {
		struct ibv_qp_attr attr;
		int flags;
		uint32_t remote_qpn = queue_pair[i^1]->qp_num;
		uint16_t dlid = port_attr[i^1].lid;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTR;
		attr.path_mtu = IBV_MTU_512;
		attr.dest_qp_num = remote_qpn;
		attr.rq_psn = 0;
		attr.ah_attr.is_global = 0;
		attr.ah_attr.dlid = dlid;
		attr.ah_attr.sl = 0;
		attr.ah_attr.src_path_bits = 0;
		attr.ah_attr.port_num = 1;
		flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
			IBV_QP_RQ_PSN;
		DO(ibv_modify_qp(queue_pair[i], &attr, flags));
	}

	virtual void to_rts(int i) {
		struct ibv_qp_attr attr;
		int flags;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTS;
		attr.sq_psn = 0;
		flags = IBV_QP_STATE | IBV_QP_SQ_PSN;

		DO(ibv_modify_qp(queue_pair[i], &attr, flags));
	}

	virtual void check_fin(int i, int n) {
		int k;

		EXEC(poll(i,n));
		for (k = 0; k < SZ; k++)
			ASSERT_EQ(PATTERN, buff[i][k]) << "k=" << k;
	}
};

class ibverbs_env_ud : public virtual ibverbs_env {
public:
	virtual enum ibv_qp_type qp_type() { return IBV_QPT_UD; }

	virtual void xmit(int i, int start, int length) {
		struct ibv_send_wr wr;
		struct ibv_sge sge;
		struct ibv_send_wr *bad_wr = NULL;

		memset(&sge, 0, sizeof(sge));
		sge.addr = (uintptr_t)buff[i]+start;
		sge.length = length - GRH_LENGTH;
		sge.lkey = region[i]->lkey;

		memset(&wr, 0, sizeof(wr));
		wr.next = NULL;
		wr.wr_id = 0;
		wr.sg_list = &sge;
		wr.num_sge = 1;
		wr.opcode = IBV_WR_SEND;
		wr.send_flags = IBV_SEND_SIGNALED;

		wr.wr.ud.ah = address[i];
		wr.wr.ud.remote_qpn = queue_pair[i^1]->qp_num;
		wr.wr.ud.remote_qkey = Q_KEY;

		DO(ibv_post_send(queue_pair[i], &wr, &bad_wr));
	}

	virtual void to_init(int i) {
		struct ibv_qp_attr attr;
		int flags;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_INIT;
		attr.port_num = 1;
		attr.pkey_index = 0;
		attr.qkey = Q_KEY;
		flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_QKEY;
		DO(ibv_modify_qp(queue_pair[i], &attr, flags));
	}

	virtual void to_rtr(int i) {
		struct ibv_qp_attr attr;
		int flags;
		uint16_t dlid = port_attr[i^1].lid;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTR;
		flags = IBV_QP_STATE;
		DO(ibv_modify_qp(queue_pair[i], &attr, flags));

		attr.ah_attr.is_global = 0;
		attr.ah_attr.dlid = dlid;
		attr.ah_attr.sl = 0;
		attr.ah_attr.src_path_bits = 0;
		attr.ah_attr.port_num = 1;
		SET(address[i], ibv_create_ah(domain[i], &attr.ah_attr));
	}

	virtual void to_rts(int i) {
		struct ibv_qp_attr attr;
		int flags;

		memset(&attr, 0, sizeof(attr));
		attr.qp_state = IBV_QPS_RTS;
		attr.sq_psn = 0;
		flags = IBV_QP_STATE | IBV_QP_SQ_PSN;

		DO(ibv_modify_qp(queue_pair[i], &attr, flags));
	}

	virtual void check_fin(int i, int n) {
		int j, k;

		EXEC(poll(i,n));
		for (j = 0; j < n; j++)
			for (k = SZ/n*j+GRH_LENGTH; k < SZ/n*(j+1); k++)
				ASSERT_EQ(PATTERN, buff[i][k])
					<< "j=" << j << " k=" << k;
	}
};

class nothing { };

class ibverbs_event : public virtual ibverbs_env {
public:
	int num_cq_events;

	virtual void poll(int i, int n) {
		if (i == RECEIVER) {
			ibverbs_env::poll(i,n);
			return;
		}
		struct ibv_wc wc[n];
		struct ibv_cq *ev_cq;
		void *ev_ctx;
		int result = 0, retries = POLL_RETRIES;

		DO(ibv_get_cq_event(channel[i], &ev_cq, &ev_ctx));
		ASSERT_EQ(ev_cq, queue[i]);
		num_cq_events++;
		DO(ibv_req_notify_cq(queue[i], 0));
		while (!result && --retries) {
			result = ibv_poll_cq(queue[i], n, wc);
			ASSERT_GE(result,0);
		}
		ASSERT_GT(retries,0);

		for (int i=0; i<result; i++) {
			VERBS_TRACE("poll status %s(%d) opcode %d len %d qp %x lid %x\n",
					ibv_wc_status_str(wc[i].status),
					wc[i].status, wc[i].opcode, wc[i].byte_len, wc[i].qp_num, wc[i].slid);
			ASSERT_FALSE(wc[i].status);
		}
	}

	virtual void arm_qp(int i) {
		num_cq_events = 0;
		if (i == SENDER)
			DO(ibv_req_notify_cq(queue[i], 0));
	}

	virtual void fini() {
		VERBS_TRACE("num_cq_events %d\n", num_cq_events);
		ibv_ack_cq_events(queue[SENDER], num_cq_events);
		EXEC(ibverbs_env::fini());
	}
};

template <typename T1, typename T2>
struct types {
	typedef T1 Base;
	typedef T2 Aux;
};

template <typename T>
class base_test : public testing::Test, public T::Base, public T::Aux {
	virtual void SetUp() {
		EXEC(this->init());
	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());
		EXEC(this->fini());
	}
};

struct xmit_peer_ctx {
	struct ibv_send_wr wr1[MAX_WR], wr2[MAX_WR];
	struct ibv_sge sge1[MAX_WR], sge2[MAX_WR];
	int peek_op_type;
	uint32_t peek_op_data;
	uint64_t peek_id;
	uint64_t rollback_id;
	union {
		struct xmit_buff {
			struct {
				uint32_t db_record;
				uint64_t db_ring;
			} commit;
			struct {
				uint32_t owner;
				uint32_t cons_index;
				uint32_t db_record;
			} peek;
		} * f;
		char * buff;
	} ctrl;
};

template <typename T>
class peerdirect_test : public testing::Test, public T::Base, public T::Aux {
public:
	struct peer_ctx {
		peerdirect_test& t;
		struct ibv_pd *domain;

		peer_ctx(peerdirect_test& _t, struct ibv_pd *d) : t(_t), domain(d) {}
	};

	struct peer_mr {
		peer_ctx &ctx;
		struct ibv_mr *region;
		uintptr_t start;
		uintptr_t end;
		peer_mr(peer_ctx &c, void* base, size_t length) : ctx(c) {
			start = (uintptr_t)base;
			end = start+length-1;
		}
	};

	struct queue_buf {
		struct ibv_peer_buf pb;
		peer_ctx * ctx;
		size_t length;
	};


	std::list<struct peer_mr *> mr_list;
	std::set<struct queue_buf *> wq_set;
	ibverbs_env_rc peer;
	bool fatality;
	char *buff_pos;

	void ops2wr(struct peer_op_wr *op, int entries, void* buff,
			struct ibv_send_wr *wr, struct ibv_sge *sge) {
		char * ptr = (char *)buff;
		int i;

		memset(wr, 0, MAX_WR*sizeof(*wr));
		memset(sge, 0, MAX_WR*sizeof(*sge));
		for(i=0; i < entries; op = op->next, i++) {
			wr[i].opcode = IBV_WR_RDMA_WRITE;
			if (op->type == IBV_PEER_OP_STORE_DWORD ||
					op->type == IBV_PEER_OP_POLL_AND_DWORD ||
					op->type == IBV_PEER_OP_POLL_NOR_DWORD) {
				sge[i].addr = (uintptr_t)ptr;
				wr[i].wr.rdma.remote_addr = (uintptr_t)op->wr.dword_va.target_va;
				sge[i].length = 4;
				if (op->type != IBV_PEER_OP_STORE_DWORD)
					wr[i].opcode = IBV_WR_RDMA_READ;
				else
					memcpy(ptr, &op->wr.dword_va.data, 4);
				ptr += 4;
			} else if (op->type == IBV_PEER_OP_STORE_QWORD) {
				sge[i].addr = (uintptr_t)ptr;
				wr[i].wr.rdma.remote_addr = (uintptr_t)op->wr.qword_va.target_va;
				sge[i].length = 8;
				memcpy(ptr, &op->wr.qword_va.data, 8);
				ptr += 8;
			} else {
				ASSERT_TRUE(0) << "unknown type: " << op->type;
			}

			for (class std::list<peer_mr *>::iterator it=mr_list.begin(); it!=mr_list.end(); it++) {
				uintptr_t start = wr[i].wr.rdma.remote_addr, end = start + sge[i].length - 1;
				if (start >= (*it)->start && end <= (*it)->end) {
					wr[i].wr.rdma.rkey = (*it)->region->rkey;
					break;
				}
			}
			ASSERT_TRUE(wr[i].wr.rdma.rkey);
			sge[i].lkey = peer.region[SENDER]->lkey;
			wr[i].sg_list = &sge[i];
			wr[i].num_sge = 1;
			if (i && wr[i-1].opcode != IBV_WR_RDMA_READ)
				wr[i-1].next = &wr[i];
			wr[i].next = NULL;
		}
	}

	void xmit_peer(int i) {
		xmit_peer_ctx ctx;
		memset(&ctx, 0, sizeof(ctx));
		EXEC(peer_prep(i, &ctx, 1));
		EXEC(peer_exec(i, &ctx));
		EXEC(peer_poll(i, &ctx));
	}

	void xmit_peer_prep(int i, int j, int n, xmit_peer_ctx* ctx) {
		int k;
		memset(ctx, 0, sizeof(*ctx)*n);
		for(k = 0; k < n; k++) {
			EXEC(this->xmit(i, SZ/n*k, SZ/n));
			EXEC(this->peer_prep(i, ctx+k, k+1));
			EXEC(this->recv(j, SZ/n*k, SZ/n));
		}

		EXEC(this->setup_buff(i, j));
	}

	void peer_prep(int i, xmit_peer_ctx* ctx, int offset) {
		int n = MAX_WR;
		struct peer_op_wr op_buff[2][MAX_WR];
		struct ibv_peer_commit commit_ops;
		commit_ops.storage = op_buff[0];
		commit_ops.entries = MAX_WR;
		struct ibv_peer_peek peek_ops;
		peek_ops.storage = op_buff[1];
		peek_ops.entries = MAX_WR;
		peek_ops.cqe_offset_from_head = offset;

		ctx->ctrl.buff = buff_pos;
		buff_pos += sizeof(*(ctx->ctrl.f));

		memset(&op_buff, 0, sizeof(op_buff));
		while(--n) {
			op_buff[0][n-1].next = &op_buff[0][n];
			op_buff[1][n-1].next = &op_buff[1][n];
		}

		DO(ibv_peer_commit_qp(this->queue_pair[i], &commit_ops));
		DO(ibv_peer_peek_cq(this->queue[i], &peek_ops));
		EXEC(ops2wr(commit_ops.storage, commit_ops.entries, &ctx->ctrl.f->commit, ctx->wr1, ctx->sge1));
		EXEC(ops2wr(peek_ops.storage, peek_ops.entries, &ctx->ctrl.f->peek, ctx->wr2, ctx->sge2));
		ctx->rollback_id = commit_ops.rollback_id;
		ctx->peek_op_type = peek_ops.storage->type;
		ctx->peek_op_data = peek_ops.storage->wr.dword_va.data;
		ctx->peek_id = peek_ops.peek_id;
	}

	void peer_exec(int i, xmit_peer_ctx* ctx) {
		struct ibv_send_wr *bad_wr = NULL;
		int retries = POLL_RETRIES;

		ctx->ctrl.f->peek.owner = ~ctx->peek_op_data;

		DO(ibv_post_send(peer.queue_pair[i], ctx->wr1, &bad_wr));
		EXEC(peer.poll(i, 2));

		VERBS_INFO("peer polling... (owner %x)\n", ctx->ctrl.f->peek.owner);
		while(--retries) {
			DO(ibv_post_send(peer.queue_pair[i], ctx->wr2, &bad_wr));
			EXEC(peer.poll(i, 1));
			if (ctx->peek_op_type == IBV_PEER_OP_POLL_AND_DWORD) {
				if (ctx->ctrl.f->peek.owner & ctx->peek_op_data)
					break;
			} else if (ctx->peek_op_type == IBV_PEER_OP_POLL_NOR_DWORD) {
				if (~(ctx->ctrl.f->peek.owner | ctx->peek_op_data))
					break;
			} else {
				ASSERT_TRUE(0) << "unknown type: " << ctx->peek_op_type;
			}
			VERBS_INFO("owner:%x\n", ctx->ctrl.f->peek.owner);
		}
		VERBS_INFO("peer polled %d (owner %x\n", retries, ctx->ctrl.f->peek.owner);
		ASSERT_TRUE(retries);
	}

	void peer_poll(int i, xmit_peer_ctx* ctx) {
		struct ibv_send_wr *bad_wr = NULL;
		DO(ibv_post_send(peer.queue_pair[i], ctx->wr2+1, &bad_wr));
		EXEC(peer.poll(i, 1));
		EXEC(this->poll(i, 1));
	}

	void peer_abort(int i, xmit_peer_ctx* ctx) {
		struct ibv_peer_abort_peek abort_ops;
		abort_ops.peek_id = ctx->peek_id;
		ibv_peer_abort_peek_cq(this->queue[i], &abort_ops);
		EXEC(this->poll(i, 1));
	}

	void peer_fail(int i, int flags, xmit_peer_ctx* ctx) {
		struct ibv_rollback_ctx rb_ctx;
		rb_ctx.rollback_id = ctx->rollback_id;
		rb_ctx.flags = flags;
		DO(ibv_rollback_qp(this->queue_pair[i], &rb_ctx));

		struct ibv_peer_abort_peek abort_ops;
		abort_ops.peek_id = ctx->peek_id;
		ibv_peer_abort_peek_cq(this->queue[i], &abort_ops);
	}

	static uint64_t register_va(void *start, size_t length, uint64_t peer_id) {
		/* TODO mutex */
		peer_ctx *ctx = (peer_ctx *)peer_id;
		peer_mr *reg_h = new peer_mr(*ctx, start, length);

		reg_h->region = ibv_reg_mr(ctx->domain, start, length,
					IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
		VERBS_TRACE("register_va %p [%lx] %p\n", start, length, reg_h->region);
		if (!reg_h->region) {
			EXPECT_TRUE(false) << "ibv_reg_mr on peer memory failed";
			reg_h->ctx.t.fatality = true;
			return 0;
		}

		reg_h->ctx.t.mr_list.push_back(reg_h);

		return (uint64_t)reg_h;
	}

	static int unregister_va(uint64_t registration_id, uint64_t peer_id) {
		peer_mr *reg_h = (peer_mr *)registration_id;
		VERBS_TRACE("unregister_va %p\n", reg_h->region);
		if(ibv_dereg_mr(reg_h->region)) {
			EXPECT_TRUE(false) << "ibv_dereg_mr on peer memory failed";
			reg_h->ctx.t.fatality = true;
		}
		reg_h->ctx.t.mr_list.remove(reg_h);
		delete reg_h;
		return 1;
	}

	static struct ibv_peer_buf * buf_alloc(struct ibv_peer_buf_alloc_attr *attr) {
		peer_ctx *ctx = (peer_ctx *)attr->peer_id;
		struct queue_buf *qb = new queue_buf;
		if (!qb) {
			EXPECT_TRUE(false) << "struct alloc failed";
			ctx->t.fatality = true;
			return NULL;
		}
		qb->pb.addr = mmap(NULL, attr->length, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
		if (qb->pb.addr == MAP_FAILED) {
			EXPECT_TRUE(false) << "mmap failed errno " << errno;
			ctx->t.fatality = true;
			delete qb;
			return NULL;
		}
		qb->pb.length = attr->length;
		qb->pb.comp_mask = 0;
		qb->length = attr->length;
		qb->ctx = ctx;
		VERBS_TRACE("buf_alloc %p[%lx]\n", qb->pb.addr, qb->length);
		ctx->t.wq_set.insert(qb);
		return &qb->pb;
	}

	static int buf_release(struct ibv_peer_buf *pb) {
		struct queue_buf *qb = (struct queue_buf*)pb;
		peer_ctx *ctx = qb->ctx;
		VERBS_TRACE("buf_release %p[%lx]\n", qb->pb.addr, qb->length);
		munmap(qb->pb.addr, qb->length);
		if (ctx->t.wq_set.erase(qb) != 1) {
			EXPECT_TRUE(false) << "unexpected qb";
			ctx->t.fatality = true;
		}
		delete qb;
		return 1;
	}

	void init_qp(int i) {
		struct ibv_qp_init_attr_ex qp_attr;
		struct ibv_create_cq_attr_ex cq_attr;
		struct ibv_peer_direct_attr pd_attr;

		if (i == RECEIVER) {
			EXEC(T::Base::init_qp(i));
			return;
		}

		fatality = false;

		memset(&qp_attr, 0, sizeof(qp_attr));
		memset(&cq_attr, 0, sizeof(cq_attr));
		memset(&pd_attr, 0, sizeof(pd_attr));

		peer_ctx *ctx = new peer_ctx(*this, peer.domain[i^1]);

		pd_attr.peer_id = (uint64_t)ctx;
		pd_attr.register_va = register_va;
		pd_attr.unregister_va = unregister_va;
		pd_attr.buf_alloc = buf_alloc;
		pd_attr.buf_release = buf_release;
		pd_attr.caps = IBV_PEER_OP_STORE_DWORD_CAP|IBV_PEER_OP_STORE_QWORD_CAP
			|IBV_PEER_OP_POLL_AND_DWORD_CAP|IBV_PEER_OP_POLL_NOR_DWORD_CAP;

		qp_attr.comp_mask = IBV_QP_INIT_ATTR_PD
			|IBV_QP_INIT_ATTR_PEER_DIRECT;
		qp_attr.qp_type = this->qp_type();
		qp_attr.sq_sig_all = 1;
		qp_attr.cap.max_send_wr = 10;
		qp_attr.cap.max_recv_wr = 10;
		qp_attr.cap.max_send_sge = 1;
		qp_attr.cap.max_recv_sge = 1;
		qp_attr.pd = this->domain[i];
		qp_attr.peer_direct_attrs = &pd_attr;

		cq_attr.comp_mask = IBV_CREATE_CQ_ATTR_PEER_DIRECT;
		cq_attr.peer_direct_attrs = &pd_attr;

		SET(this->channel[i], ibv_create_comp_channel(this->context[i]));
		SET(this->queue[i], ibv_create_cq_ex_(this->context[i], &cq_attr, 20, this->channel[i]));
		qp_attr.send_cq = qp_attr.recv_cq = this->queue[i];

		SET(this->region[i], ibv_reg_mr(this->domain[i], this->buff[i], SZ,
					IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE));
		SET(this->queue_pair[i], ibv_create_qp_ex(this->context[i], &qp_attr));
		ASSERT_FALSE(this->fatality);
	}

	virtual void SetUp() {
		EXEC(peer.init());
		EXEC(this->init());

		VERBS_INFO("connect lid %x-%x qp %x-%x peer %x-%x\n",
				this->port_attr[SENDER].lid, this->port_attr[RECEIVER].lid,
				this->queue_pair[SENDER]->qp_num, this->queue_pair[RECEIVER]->qp_num,
				peer.queue_pair[SENDER]->qp_num, peer.queue_pair[RECEIVER]->qp_num);
		ASSERT_EQ(3U, wq_set.size());
		ASSERT_EQ(4U, mr_list.size());
		buff_pos = peer.buff[SENDER];
	}

	virtual void TearDown() {
		ASSERT_FALSE(HasFailure());

		EXEC(this->fini());
		ASSERT_EQ(0U, wq_set.size());
		ASSERT_EQ(0U, mr_list.size());
		EXEC(peer.fini());
	}
};

#endif

