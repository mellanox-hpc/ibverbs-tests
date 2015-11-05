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

#include "common.h"
#include "gtest/gtest.h"

#define ENDPOINTS 2
#define BUFF_SIZE 4096

static struct ibv_port_attr port_attr;
static struct ibv_context *context = NULL;
static struct ibv_pd *domain = NULL;
static struct ibv_cq *queue[ENDPOINTS] = { NULL };
static struct ibv_qp *queue_pair[ENDPOINTS] = { NULL };
static struct ibv_mr *region[ENDPOINTS] = { NULL };
static char buff[ENDPOINTS][BUFF_SIZE];

/* release all allocated verbs resource */
static void fini()
{
	int i;
	for (i = 0; i < ENDPOINTS; i++) {
		if (queue_pair[i]) VERBS_DO(ibv_destroy_qp(queue_pair[i]));
		if (region[i]) VERBS_DO(ibv_dereg_mr(region[i]));
		if (queue[i]) VERBS_DO(ibv_destroy_cq(queue[i]));
	}
	if (domain) VERBS_DO(ibv_dealloc_pd(domain));
	if (context) VERBS_DO(ibv_close_device(context));
}

/* poll CQ for completion in busy loop */
static void poll(int i) {
	struct ibv_wc wc;
	int result = 0;

	while (!result) {
		result = ibv_poll_cq(queue[i], 1, &wc);
		if (result<0)
			VERBS_ERROR("error: failed at poll\n");
	}

	VERBS_INFO("poll[%d] status %d opcode %d len %d qp %d lid %d\n", 
			i, wc.status, wc.opcode, wc.byte_len, wc.qp_num, wc.slid);
}

/* transmit buffer through QP */
static void xmit(int i, ibv_wr_opcode op) {
	struct ibv_send_wr wr;
	struct ibv_sge sge;
	struct ibv_send_wr *bad_wr = NULL;

	memset(&sge, 0, sizeof(sge));
	sge.addr = (uintptr_t)buff[i];
	sge.length = sizeof(buff[i]);
	sge.lkey = region[i]->lkey;

	memset(&wr, 0, sizeof(wr));
	wr.next = NULL;
	wr.wr_id = 0;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.opcode = op;
	wr.send_flags = IBV_SEND_SIGNALED;
	VERBS_DO(ibv_post_send(queue_pair[i], &wr, &bad_wr));
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

/* init QP, CQ and MR */
static void init_one(int i) {
	struct ibv_qp_init_attr qp_init_attr;
	VERBS_SET(queue[i], ibv_create_cq(context, 1, NULL, NULL, 0));
	VERBS_SET(region[i], ibv_reg_mr(domain, buff[i], sizeof(buff[i]), 
			IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE));

	memset(&qp_init_attr, 0, sizeof(qp_init_attr));
	qp_init_attr.qp_type = IBV_QPT_RC;
	qp_init_attr.sq_sig_all = 1;
	qp_init_attr.send_cq = queue[i];
	qp_init_attr.recv_cq = queue[i];
	qp_init_attr.cap.max_send_wr = 1;
	qp_init_attr.cap.max_recv_wr = 1;
	qp_init_attr.cap.max_send_sge = 1;
	qp_init_attr.cap.max_recv_sge = 1;
	VERBS_SET(queue_pair[i], ibv_create_qp(domain, &qp_init_attr));
}

/* init all verbs resource */
static void init() {
	int num_devices;
	struct ibv_device **dev_list = NULL;
	struct ibv_device *ib_dev = NULL;
	int i = 42;

	VERBS_SET(dev_list, ibv_get_device_list(&num_devices));
	if (!num_devices)
		VERBS_ERROR("no devices");

	ib_dev = dev_list[0];
	VERBS_SET(context, ibv_open_device(ib_dev));
	ibv_free_device_list(dev_list);
	dev_list = NULL;
	ib_dev = NULL;

	VERBS_DO(ibv_query_port(context, 1, &port_attr));
	VERBS_SET(domain, ibv_alloc_pd(context));

	init_one(0);
	init_one(1);
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
static void to_rtr(int i, uint32_t remote_qpn, uint16_t dlid) {
	struct ibv_qp_attr attr;
	int flags;

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

/* make loop by connecting two QPs */
static void connect()
{
	to_init(0);
	to_init(1);

	VERBS_INFO("connect lid %d qp0 %d qp1 %d\n", 
			port_attr.lid, queue_pair[0]->qp_num, queue_pair[1]->qp_num);

	to_rtr(1, queue_pair[0]->qp_num, port_attr.lid);
	to_rtr(0, queue_pair[1]->qp_num, port_attr.lid);

	to_rts(0);
	to_rts(1);
}

/* test - pass buffer over loopback infiniband */
TEST(general, loop)
{
	size_t i;
	init();
	connect();
	recv(1);
	memset(buff[0], 0x5a, sizeof(buff[0]));
	xmit(0, IBV_WR_SEND);
	poll(0);
	poll(1);
	for(i=0; i<sizeof(buff[1]); i++)
		if (buff[1][i]!=0x5a)
			VERBS_ERROR("data transfer failed at %lx\n", i);
	fini();
}
