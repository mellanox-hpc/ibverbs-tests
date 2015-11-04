/**
* Copyright (C) Mellanox Technologies Ltd. 2015.  ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <infiniband/verbs.h>
#include <infiniband/verbs_exp.h>
#include <infiniband/peer_ops.h>
#include <infiniband/arch.h>

#include "common.h"
#include "gtest/gtest.h"

#define ENDPOINTS 2
#define CHANELS 2
#define BUFF_SIZE 4096

static struct ibv_port_attr port_attr[ENDPOINTS];
static struct ibv_context *context[ENDPOINTS] = { NULL };
static struct ibv_pd *domain[ENDPOINTS] = { NULL };
static struct ibv_cq *queue[ENDPOINTS*CHANELS] = { NULL };
static struct ibv_qp *queue_pair[ENDPOINTS*CHANELS] = { NULL };
static struct ibv_mr *region[ENDPOINTS*CHANELS] = { NULL };
static char buff[ENDPOINTS][BUFF_SIZE];

/* release all allocated verbs resource */
static void fini()
{
	int i;
	for (i =0 ; i < ENDPOINTS*CHANELS; i++) {
		if (queue_pair[i]) VERBS_DO(ibv_destroy_qp(queue_pair[i]));
		if (region[i]) VERBS_DO(ibv_dereg_mr(region[i]));
		if (queue[i]) VERBS_DO(ibv_destroy_cq(queue[i]));
	}
	for (i =0 ; i < ENDPOINTS; i++) {
		if (domain[i]) VERBS_DO(ibv_dealloc_pd(domain[i]));
		if (context[i]) VERBS_DO(ibv_close_device(context[i]));
	}
}

/* poll CQ for completion in busy loop */
#define C 3
static void poll(int i) {
	struct ibv_wc wc[C];
	int result = 0;

	VERBS_INFO("polling %d...\n", i);

	while (!result) {
		result = ibv_poll_cq(queue[i], C, wc);
		if (result<0)
			VERBS_ERROR("error: failed at poll\n");
	}

	for(i=0; i<result; i++) 
		if (wc[i].status)
			VERBS_INFO("poll status %d opcode %d len %d qp %d lid %d\n", 
					wc[i].status, wc[i].opcode, wc[i].byte_len, 
					wc[i].qp_num, wc[i].slid);
}

/* transmit buffer through QP */
static void xmit() {
	struct ibv_send_wr wr;
	struct ibv_sge sge;
	struct ibv_send_wr *bad_wr = NULL;
	int i = 42;

	memset(&sge, 0, sizeof(sge));
	sge.addr = (uintptr_t)buff[0];
	sge.length = sizeof(buff[0]);
	sge.lkey = region[0]->lkey;

	memset(&wr, 0, sizeof(wr));
	wr.next = NULL;
	wr.wr_id = 0;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.opcode = IBV_WR_SEND;
	wr.send_flags = IBV_SEND_SIGNALED;
	VERBS_DO(ibv_post_send(queue_pair[0], &wr, &bad_wr));

	{
		int N = 8;
		struct ibv_send_wr ctrl_wr[N];
		struct ibv_mr *ctrl_mr[2][N];
		struct ibv_sge ctrl_sge[N];

		struct peer_op_wr op_buff1[N], *op;
		struct ibv_exp_peer_commit commit_ops;
	        commit_ops.wr_storage = op_buff1,
	 	commit_ops.available_wr_storage = N,
		commit_ops.peer_queue_token = (uintptr_t)queue_pair[0],

		memset(&ctrl_wr, 0, sizeof(ctrl_wr));
		memset(&ctrl_sge, 0, sizeof(ctrl_sge));
		VERBS_DO(ibv_exp_peer_commit_qp(queue_pair[0], &commit_ops));

		for(op = commit_ops.ops_to_post, i=0; op; op = op->next_op) {
			if (op->op_type == IBV_EXP_PEER_OP_FENCE) {
				//wmb();
			} else {
				if (op->op_type == IBV_EXP_PEER_OP_STORE_DWORD) {
					VERBS_SET(ctrl_mr[0][i], ibv_reg_mr(domain[1], 
						 (void*)&op->wr.dword_va.data, 4, 0));
					VERBS_SET(ctrl_mr[1][i], ibv_reg_mr(domain[1], 
						 (void*)op->wr.dword_va.target_va, 4, 
						 IBV_ACCESS_REMOTE_WRITE|IBV_ACCESS_LOCAL_WRITE));
					ctrl_sge[i].addr = (uintptr_t)&op->wr.dword_va.data;
					ctrl_wr[i].wr.rdma.remote_addr = (uintptr_t)op->wr.dword_va.target_va;
					ctrl_sge[i].length = 4;
				} else if (op->op_type == IBV_EXP_PEER_OP_STORE_QWORD) {
					VERBS_SET(ctrl_mr[0][i], ibv_reg_mr(domain[1], 
						 (void*)&op->wr.qword_va.data, 8, 0));
					VERBS_SET(ctrl_mr[1][i], ibv_reg_mr(domain[1], 
						 (void*)op->wr.qword_va.target_va, 8, 
						 IBV_ACCESS_REMOTE_WRITE|IBV_ACCESS_LOCAL_WRITE));
					ctrl_sge[i].addr = (uintptr_t)&op->wr.qword_va.data;
					ctrl_wr[i].wr.rdma.remote_addr = (uintptr_t)op->wr.qword_va.target_va;
					ctrl_sge[i].length = 8;
				} else {
					VERBS_ERROR("unknown op_type: %d\n", op->op_type);
				}

				ctrl_sge[i].lkey = ctrl_mr[0][i]->lkey;
				ctrl_wr[i].sg_list = &ctrl_sge[i];
				ctrl_wr[i].num_sge = 1;
				ctrl_wr[i].opcode = IBV_WR_RDMA_WRITE;
			 	ctrl_wr[i].wr.rdma.rkey = ctrl_mr[1][i]->rkey;
				if (i)
					ctrl_wr[i-1].next = &ctrl_wr[i];
				ctrl_wr[i].next = NULL;
				i++;
			}
		}

		VERBS_DO(ibv_post_send(queue_pair[2], &ctrl_wr[0], &bad_wr));
		poll(2);

		poll(0);
		poll(1);
		for (i--; i>=0; i--) { 
			VERBS_DO(ibv_dereg_mr(ctrl_mr[0][i]));
			VERBS_DO(ibv_dereg_mr(ctrl_mr[1][i]));
		}
	}
}

/* pass buffer to QP for receive */
static void recv(int i) {
	struct ibv_recv_wr wr;
	struct ibv_sge sge;
	struct ibv_recv_wr *bad_wr = NULL;

	memset(&sge, 0, sizeof(sge));
	sge.addr = (uintptr_t)buff[i];
	sge.length = sizeof(buff[i]);
	sge.lkey = region[i]->lkey;

	memset(&wr, 0, sizeof(wr));
	wr.next = NULL;
	wr.wr_id = 0;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	VERBS_DO(ibv_post_recv(queue_pair[i], &wr, &bad_wr));
}

#define qp2dev(i) ((i)/2)

/* init peer-direct QP, CQ and MR */
static void init_pd_qp(int i) {		
	struct ibv_exp_qp_init_attr attr;

	VERBS_SET(queue[i], ibv_create_cq(context[qp2dev(i)], 1, NULL, NULL, 0));
	VERBS_SET(region[i], ibv_reg_mr(domain[qp2dev(i)], buff[i], sizeof(buff[i]), 
		IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE));

	memset(&attr, 0, sizeof(attr));
	attr.pd = domain[qp2dev(i)];
	attr.comp_mask = IBV_EXP_QP_INIT_ATTR_PD|IBV_EXP_QP_INIT_ATTR_CREATE_FLAGS;
	attr.qp_type = IBV_QPT_RC;
	attr.sq_sig_all = 1;
	attr.send_cq = queue[i];
	attr.recv_cq = queue[i];
	attr.cap.max_send_wr = 1;
	attr.cap.max_recv_wr = 1;
	attr.cap.max_send_sge = 1;
	attr.cap.max_recv_sge = 1;
	attr.exp_create_flags = IBV_EXP_QP_CREATE_PEER_DIRECT;

	VERBS_SET(queue_pair[i], ibv_exp_create_qp(context[qp2dev(i)], &attr));
}

/* init suplimentary QP and CQ to press doorbell with RDMA_WRITE */
static void init_ctrl_qp(int i) {		
	struct ibv_qp_init_attr attr;
	VERBS_SET(queue[i], ibv_create_cq(context[qp2dev(i)], 1, NULL, NULL, 0));

	memset(&attr, 0, sizeof(attr));
	attr.qp_type = IBV_QPT_RC;
	attr.sq_sig_all = 1;
	attr.send_cq = queue[i];
	attr.recv_cq = queue[i];
	attr.cap.max_send_wr = 10;
	attr.cap.max_recv_wr = 10;
	attr.cap.max_send_sge = 1;
	attr.cap.max_recv_sge = 1;
	VERBS_SET(queue_pair[i], ibv_create_qp(domain[qp2dev(i)], &attr));
}

/* init all verbs resource */
static void init() {
	int num_devices;
	struct ibv_device **dev_list = NULL;
	int i = 42;

	VERBS_SET(dev_list, ibv_get_device_list(&num_devices));
	if (num_devices != 2)
		VERBS_ERROR("num_devices %d\n", num_devices);

	for (i=0; i<2; i++) {
		VERBS_SET(context[i], ibv_open_device(dev_list[i]));
		VERBS_DO(ibv_query_port(context[i], 1, &port_attr[i]));
		VERBS_SET(domain[i], ibv_alloc_pd(context[i]));
	}

	ibv_free_device_list(dev_list);

	init_pd_qp(0);
	init_pd_qp(1);

	init_ctrl_qp(2);
	init_ctrl_qp(3);
}


/* transfer QP to INIT state */
static void to_init(int i) {
	struct ibv_qp_attr attr;
	int flags;

	memset(&attr, 0, sizeof(attr));
	attr.qp_state = IBV_QPS_INIT;
	attr.port_num = 1;
	attr.pkey_index = 0;
	attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
	flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
	VERBS_DO(ibv_modify_qp(queue_pair[i], &attr, flags));
}

/* transfer QP to Ready-To-Receive state */
static void to_rtr(int i, int r) {
	struct ibv_qp_attr attr;
	int flags;
	uint32_t remote_qpn = queue_pair[r]->qp_num;
	uint16_t dlid = port_attr[qp2dev(r)].lid;

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
	VERBS_DO(ibv_modify_qp(queue_pair[i], &attr, flags));
}

/* transfer QP to Ready-To-Send state */
static void to_rts(int i) {
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

	VERBS_DO(ibv_modify_qp(queue_pair[i], &attr, flags));
}

/* make two loops by connecting four QPs */
static void connect()
{
	to_init(0);
	to_init(1);
	to_init(2);
	to_init(3);

	VERBS_INFO("connect lid %d-%d qp0 %d qp1 %d qp2 %d qp3 %d\n", 
			port_attr[0].lid, port_attr[1].lid,
			queue_pair[0]->qp_num, queue_pair[1]->qp_num,
			queue_pair[2]->qp_num, queue_pair[3]->qp_num);

	to_rtr(1, 0);
	to_rtr(0, 1);
	to_rtr(3, 2);
	to_rtr(2, 3);

	to_rts(0);
	to_rts(2);
}

/* test - pass buffer over loopback infiniband 
 * 	  pressing doorbell via RDMA_WRITE */
TEST(peerdirect, loop)
{
	size_t i;
	init();
	connect();
	memset(buff[0], 0x5a, sizeof(buff[0]));
	recv(1);
	xmit();
	for(i=0; i<sizeof(buff[1]); i++)
		if (buff[1][i]!=0x5a)
			VERBS_ERROR("data transfer failed at %lx\n", i);
	fini();
}
