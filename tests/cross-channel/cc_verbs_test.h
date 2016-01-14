/**
 * Copyright (C) 2015      Mellanox Technologies Ltd. All rights reserved.
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
#ifndef _IBVERBS_CC_VERBS_TEST_
#define _IBVERBS_CC_VERBS_TEST_
#include "verbs_test.h"

enum {
	TEST_BASE_WRID = 0x0000FFFF,
	TEST_RECV_WRID = 0x00010000,
	TEST_SEND_WRID = 0x00020000
};

#define TEST_SET_WRID(type, code)   ((type) | (code))
#define TEST_GET_WRID(opcode)       ((opcode) & TEST_BASE_WRID)
#define TEST_GET_TYPE(opcode)       ((opcode) & (~TEST_BASE_WRID))

static INLINE const char* wr_id2str(uint64_t wr_id)
{
	switch (TEST_GET_TYPE(wr_id)) {
		case TEST_RECV_WRID:    return "RECV";
		case TEST_SEND_WRID:    return "SEND";
		default:        	return "N/A";
	}
}

#define QP_PORT			1
#define MQP_PORT		1
#define DEFAULT_DEPTH		0x1F

#define MAX_POLL_CQ_TIMEOUT	10000 /* poll CQ timeout in millisec (10 seconds) */

/**
 * Base class for Cross-Channel ibverbs test fixtures.
 * Initialize and close ibverbs.
 */
class cc_base_verbs_test : public verbs_test {
protected:
	virtual void SetUp() {
#ifndef HAVE_CROSS_CHANNEL
		// Skip this test.
		// Cross-channel is supported via extended verbs only
		skip_this_test = true;
#else
		int rc = 0;
		skip_this_test = false;
		verbs_test::SetUp();
		device_attr.comp_mask = -1;
		rc = ibv_query_device_ex(ibv_ctx, NULL, &device_attr);
		ASSERT_TRUE(!rc);
#endif //HAVE_CROSS_CHANNEL
	}
	virtual void TearDown() {
		if (!skip_this_test)
			verbs_test::TearDown();
	}

protected:
#ifdef HAVE_CROSS_CHANNEL
	struct ibv_device_attr_ex device_attr;
#endif //HAVE_CROSS_CHANNEL
};

/**
 * Class for Cross-Channel create_cq checks
 */
class cc_init_verbs_test : public cc_base_verbs_test {
protected:
	virtual void SetUp() {
		cc_base_verbs_test::SetUp();
		if (skip_this_test)
			return;

#ifdef HAVE_CROSS_CHANNEL
		int rc = 0;
		EXPECT_TRUE((device_attr.orig_attr.device_cap_flags & IBV_DEVICE_CROSS_CHANNEL))
				<< "This class of tests is for Cross-Channel functionality."
				<< " But device does not support one";

		ctx = (struct test_context*)malloc(sizeof(struct test_context));
		ASSERT_TRUE(ctx != NULL);

		memset(ctx, 0, sizeof(*ctx));

		ctx->context = ibv_ctx;
		ctx->port = QP_PORT;
		ctx->qp_tx_depth = 20;
		ctx->qp_rx_depth = 100;
		ctx->cq_tx_depth = ctx->qp_tx_depth;
		ctx->cq_rx_depth = ctx->qp_rx_depth;
		ctx->size = sysconf(_SC_PAGESIZE);

		/*
		 * A Protection Domain (PD) allows the user to restrict which components can interact
		 * with only each other. These components can be AH, QP, MR, and SRQ
		 */
		ctx->pd = ibv_alloc_pd(ctx->context);
		ASSERT_TRUE(ctx->pd != NULL);

		/*
		 * VPI only works with registered memory. Any memory buffer which is valid in
		 * the process's virtual space can be registered. During the registration process
		 * the user sets memory permissions and receives a local and remote key
		 * (lkey/rkey) which will later be used to refer to this memory buffer
		 */
		ctx->net_buf = memalign(sysconf(_SC_PAGESIZE), ctx->size);
		ASSERT_TRUE(ctx->net_buf != NULL);
		memset(ctx->net_buf, 0, ctx->size);

		ctx->mr = ibv_reg_mr(ctx->pd, ctx->net_buf, ctx->size, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);
		ASSERT_TRUE(ctx->mr != NULL);

		ctx->buf = memalign(sysconf(_SC_PAGESIZE), ctx->size);
		ASSERT_TRUE(ctx->buf != NULL);
		memset(ctx->buf, 0, ctx->size);

		ctx->last_result = memalign(sysconf(_SC_PAGESIZE), sizeof(uint64_t));
		ASSERT_TRUE(ctx->last_result != NULL);
		memset(ctx->last_result, 0, sizeof(uint64_t));

		ctx->mcq = ibv_create_cq(ctx->context, 0x10, NULL, NULL, 0);
		ASSERT_TRUE(ctx->mcq != NULL);

		{
			struct ibv_qp_init_attr_ex init_attr;

			memset(&init_attr, 0, sizeof(init_attr));

			init_attr.qp_context = NULL;
			init_attr.send_cq = ctx->mcq;
			init_attr.recv_cq = ctx->mcq;
			init_attr.srq = NULL;
			init_attr.cap.max_send_wr  = 0x40;
			init_attr.cap.max_recv_wr  = 0;
			init_attr.cap.max_send_sge = 16;
			init_attr.cap.max_recv_sge = 16;
			init_attr.cap.max_inline_data = 0;
			init_attr.qp_type = IBV_QPT_RC;
			init_attr.sq_sig_all = 0;
			init_attr.pd = ctx->pd;
			init_attr.comp_mask |= IBV_QP_INIT_ATTR_CREATE_FLAGS | IBV_QP_INIT_ATTR_PD;
			init_attr.create_flags = IBV_QP_CREATE_CROSS_CHANNEL;
			ctx->mqp = ibv_create_qp_ex(ctx->context, &init_attr);
			ASSERT_TRUE(ctx->mqp != NULL);
		}

		{
			struct ibv_qp_attr attr;

			memset(&attr, 0, sizeof(attr));

			attr.qp_state        = IBV_QPS_INIT;
			attr.pkey_index      = 0;
			attr.port_num        = MQP_PORT;
			attr.qp_access_flags = 0;

			rc = ibv_modify_qp(ctx->mqp, &attr,
					IBV_QP_STATE              |
					IBV_QP_PKEY_INDEX         |
					IBV_QP_PORT               |
					IBV_QP_ACCESS_FLAGS);
			ASSERT_EQ(rc, EOK);

			memset(&attr, 0, sizeof(attr));

			attr.qp_state              = IBV_QPS_RTR;
			attr.path_mtu              = IBV_MTU_1024;
			attr.dest_qp_num	   = ctx->mqp->qp_num;
			attr.rq_psn                = 0;
			attr.max_dest_rd_atomic    = 1;
			attr.min_rnr_timer         = 12;
			attr.ah_attr.is_global     = 0;
			attr.ah_attr.dlid          = 0;
			attr.ah_attr.sl            = 0;
			attr.ah_attr.src_path_bits = 0;
			attr.ah_attr.port_num      = MQP_PORT;

			rc = ibv_modify_qp(ctx->mqp, &attr,
					IBV_QP_STATE              |
					IBV_QP_AV                 |
					IBV_QP_PATH_MTU           |
					IBV_QP_DEST_QPN           |
					IBV_QP_RQ_PSN             |
					IBV_QP_MAX_DEST_RD_ATOMIC |
					IBV_QP_MIN_RNR_TIMER);
			ASSERT_EQ(rc, EOK);

			memset(&attr, 0, sizeof(attr));

			attr.qp_state      = IBV_QPS_RTS;
			attr.timeout       = 14;
			attr.retry_cnt     = 7;
			attr.rnr_retry     = 7;
			attr.sq_psn        = 0;
			attr.max_rd_atomic = 1;
			rc = ibv_modify_qp(ctx->mqp, &attr,
					IBV_QP_STATE              |
					IBV_QP_TIMEOUT            |
					IBV_QP_RETRY_CNT          |
					IBV_QP_RNR_RETRY          |
					IBV_QP_SQ_PSN             |
					IBV_QP_MAX_QP_RD_ATOMIC);
			ASSERT_EQ(rc, EOK);
		}
#endif //HAVE_CROSS_CHANNEL
	}
#ifdef HAVE_CROSS_CHANNEL
	void __init_test(int qp_flag = 0, int qp_tx_depth = DEFAULT_DEPTH, int qp_rx_depth = DEFAULT_DEPTH,
			 int cq_tx_flag = 0, int cq_tx_depth = DEFAULT_DEPTH,
			 int cq_rx_flag = 0, int cq_rx_depth = DEFAULT_DEPTH) {

		int rc = EOK;

		ctx->qp_tx_depth = qp_tx_depth;
		ctx->qp_rx_depth = qp_rx_depth;
		ctx->cq_tx_depth = cq_tx_depth;
		ctx->cq_rx_depth = cq_rx_depth;
		/*
		 * A CQ contains completed work requests (WR). Each WR will generate a completion
		 * queue event (CQE) that is placed on the CQ. The CQE will specify if the WR was
		 * completed successfully or not.
		 */
		{
			struct ibv_create_cq_attr_ex attr = {0};
			if (cq_tx_flag)	{
				attr.comp_mask	= IBV_CREATE_CQ_ATTR_FLAGS;
				attr.flags	= cq_tx_flag;
			}
			attr.cqe = ctx->cq_tx_depth;
			ctx->scq = ibv_create_cq_ex(ctx->context, &attr);
			ASSERT_TRUE(ctx->scq != NULL);
		}

		{
			struct ibv_create_cq_attr_ex attr = {0};
			if (cq_rx_flag)	{
				attr.comp_mask	= IBV_CREATE_CQ_ATTR_FLAGS;
				attr.flags	= cq_rx_flag;
			}
			attr.cqe = ctx->cq_rx_depth;
			ctx->rcq = ibv_create_cq_ex(ctx->context, &attr);
			ASSERT_TRUE(ctx->rcq != NULL);
		}

		ctx->wc = (struct ibv_wc*)malloc((ctx->cq_tx_depth + ctx->cq_rx_depth) * sizeof(struct ibv_wc));
		ASSERT_TRUE(ctx->wc != NULL);

		/*
		 * Creating a QP will also create an associated send queue and receive queue.
		 */
		{
			struct ibv_qp_init_attr_ex init_attr;

			memset(&init_attr, 0, sizeof(init_attr));

			init_attr.qp_context = NULL;
			init_attr.send_cq = ctx->scq;
			init_attr.recv_cq = ctx->rcq;
			init_attr.srq = NULL;
			init_attr.cap.max_send_wr  = ctx->qp_tx_depth;
			init_attr.cap.max_recv_wr  = ctx->qp_rx_depth;
			init_attr.cap.max_send_sge = 16;
			init_attr.cap.max_recv_sge = 16;
			init_attr.cap.max_inline_data = 0;
			init_attr.qp_type = IBV_QPT_RC;
			init_attr.sq_sig_all = 0;
			init_attr.pd = ctx->pd;
			init_attr.comp_mask |= IBV_QP_INIT_ATTR_CREATE_FLAGS | IBV_QP_INIT_ATTR_PD;
			init_attr.create_flags = (enum ibv_qp_create_flags)qp_flag;
			ctx->qp = ibv_create_qp_ex(ctx->context, &init_attr);
			ASSERT_TRUE(ctx->qp != NULL);
		}

		/*
		 * Exchange connection info with a peer
		 * Convert all data in net order during this stage to guarantee correctness
		 */
		{
			struct test_entry local_info;
			struct test_entry remote_info;

			/*
			 * My connection info
			 */
			ctx->my_info.lid = get_local_lid(ctx->context, ctx->port);
			ctx->my_info.qpn = ctx->qp->qp_num;
			ctx->my_info.psn = lrand48() & 0xffffff;
			ctx->my_info.vaddr = (uintptr_t)ctx->net_buf + ctx->size;
			ctx->my_info.rkey = ctx->mr->rkey;
			memset(&(ctx->my_info.gid), 0, sizeof(ctx->my_info.gid));

			local_info.lid = htons(ctx->my_info.lid);
			local_info.qpn = htonl(ctx->my_info.qpn);
			local_info.psn = htonl(ctx->my_info.psn);
			local_info.vaddr = htonll(ctx->my_info.vaddr);
			local_info.rkey = htonl(ctx->my_info.rkey);
			memcpy(&(local_info.gid), &(ctx->my_info.gid), sizeof(ctx->my_info.gid));

			/*
			 * Exchange procedure  (We use loopback)...................
			 */
			memcpy(&remote_info, &local_info, sizeof(remote_info));

			/*
			 * Peer connection info
			 */
			ctx->peer_info.lid = ntohs(remote_info.lid);
			ctx->peer_info.qpn = ntohl(remote_info.qpn);
			ctx->peer_info.psn = ntohl(remote_info.psn);
			ctx->peer_info.vaddr = ntohll(remote_info.vaddr);
			ctx->peer_info.rkey = ntohl(remote_info.rkey);
			memcpy(&(ctx->peer_info.gid), &(remote_info.gid), sizeof(ctx->peer_info.gid));
		}

		ASSERT_LE(__post_read(ctx, ctx->qp_rx_depth), ctx->qp_rx_depth);

		/*
		 * A created QP still cannot be used until it is transitioned through several states,
		 * eventually getting to Ready To Send (RTS).
		 * IBV_QPS_INIT -> IBV_QPS_RTR -> IBV_QPS_RTS
		 */
		{
			struct ibv_qp_attr attr;

			memset(&attr, 0, sizeof(attr));

			attr.qp_state        = IBV_QPS_INIT;
			attr.pkey_index      = 0;
			attr.port_num        = ctx->port;
			attr.qp_access_flags = IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

			rc = ibv_modify_qp(ctx->qp, &attr,
					IBV_QP_STATE              |
					IBV_QP_PKEY_INDEX         |
					IBV_QP_PORT               |
					IBV_QP_ACCESS_FLAGS);
			ASSERT_EQ(rc, EOK);

			memset(&attr, 0, sizeof(attr));

			attr.qp_state              = IBV_QPS_RTR;
			attr.path_mtu              = IBV_MTU_1024;
			attr.dest_qp_num	   = ctx->peer_info.qpn;
			attr.rq_psn                = ctx->peer_info.psn;
			attr.max_dest_rd_atomic    = 4;
			attr.min_rnr_timer         = 12;
			attr.ah_attr.is_global     = 0;
			attr.ah_attr.dlid          = ctx->peer_info.lid;
			attr.ah_attr.sl            = 0;
			attr.ah_attr.src_path_bits = 0;
			attr.ah_attr.port_num      = ctx->port;

			rc = ibv_modify_qp(ctx->qp, &attr,
					IBV_QP_STATE              |
					IBV_QP_AV                 |
					IBV_QP_PATH_MTU           |
					IBV_QP_DEST_QPN           |
					IBV_QP_RQ_PSN             |
					IBV_QP_MAX_DEST_RD_ATOMIC |
					IBV_QP_MIN_RNR_TIMER);
			ASSERT_EQ(rc, EOK);

			memset(&attr, 0, sizeof(attr));

			attr.qp_state      = IBV_QPS_RTS;
			attr.timeout       = 14;
			attr.retry_cnt     = 7;
			attr.rnr_retry     = 7;
			attr.sq_psn        = ctx->my_info.psn;
			attr.max_rd_atomic = 4;

			rc = ibv_modify_qp(ctx->qp, &attr,
					IBV_QP_STATE              |
					IBV_QP_TIMEOUT            |
					IBV_QP_RETRY_CNT          |
					IBV_QP_RNR_RETRY          |
					IBV_QP_SQ_PSN             |
					IBV_QP_MAX_QP_RD_ATOMIC);
			ASSERT_EQ(rc, EOK);
		}
	}
#endif //HAVE_CROSS_CHANNEL

	virtual void TearDown() {
#ifdef HAVE_CROSS_CHANNEL
		if (ctx->mqp)
			ibv_destroy_qp(ctx->mqp);
		if (ctx->mcq)
			ibv_destroy_cq(ctx->mcq);
		if (ctx->qp)
			ibv_destroy_qp(ctx->qp);
		if (ctx->scq)
			ibv_destroy_cq(ctx->scq);
		if (ctx->rcq)
			ibv_destroy_cq(ctx->rcq);
		free(ctx->wc);
		if (ctx->ah)
			ibv_destroy_ah(ctx->ah);
		if (ctx->mr)
			ibv_dereg_mr(ctx->mr);
		if (ctx->pd)
			ibv_dealloc_pd(ctx->pd);
		free(ctx->net_buf);
		free(ctx->buf);
		free(ctx->last_result);
		free(ctx);
		ctx = NULL;
#endif //HAVE_CROSS_CHANNEL

		cc_base_verbs_test::TearDown();
	}
protected:
#ifdef HAVE_CROSS_CHANNEL
	struct test_context *ctx;

	uint16_t get_local_lid(struct ibv_context *context, int port)
	{
		struct ibv_port_attr attr;

		if (ibv_query_port(context, port, &attr))
			return 0;

		return attr.lid;
	}

	int __post_write(struct test_context *ctx, int64_t wrid, enum ibv_wr_opcode opcode)
	{
		int rc = EOK;
		struct ibv_sge list;
		struct ibv_send_wr wr;
		struct ibv_send_wr *bad_wr;

		*(uint64_t*)ctx->net_buf = htonll(wrid);

		/* prepare the scatter/gather entry */
		memset(&list, 0, sizeof(list));
		list.addr = (uintptr_t)ctx->net_buf;
		list.length = ctx->size;
		list.lkey = ctx->mr->lkey;

		/* prepare the send work request */
		memset(&wr, 0, sizeof(wr));
		wr.wr_id = wrid;
		wr.next = NULL;
		wr.sg_list = &list;
		wr.num_sge = 1;
		wr.opcode = opcode;
		wr.send_flags = IBV_SEND_SIGNALED;
		wr.imm_data = 0;

		switch (opcode) {
		case IBV_WR_RDMA_WRITE:
		case IBV_WR_RDMA_READ:
			wr.wr.rdma.remote_addr = ctx->peer_info.vaddr;
			wr.wr.rdma.rkey = ctx->peer_info.rkey;
			break;
		case IBV_WR_RECV_ENABLE:
		case IBV_WR_SEND_ENABLE:
			wr.wr.wqe_enable.qp = ctx->qp;
			wr.wr.wqe_enable.wqe_count = 0;

			wr.send_flags |=  IBV_SEND_WAIT_EN_LAST;
			break;

		case IBV_WR_CQE_WAIT:
			wr.wr.cqe_wait.cq = ctx->scq;
			wr.wr.cqe_wait.cq_count = 1;

			wr.send_flags |= IBV_SEND_WAIT_EN_LAST;
			break;
		case IBV_WR_SEND:
			break;

		default:
			VERBS_TRACE("Invalid opcode: %d.\n", wr.opcode);
			rc = EINVAL;
			break;
		}

		rc = ibv_post_send(ctx->qp, &wr, &bad_wr);
		return rc;
	}


	int __post_read(struct test_context *ctx, int count)
	{
		int rc = EOK;
		struct ibv_sge list = {0};
		struct ibv_recv_wr wr = {0};
		struct ibv_recv_wr *bad_wr;
		int i = 0;

		/* prepare the scatter/gather entry */
		list.addr = (uintptr_t)ctx->net_buf;
		list.length = ctx->size;
		list.lkey = ctx->mr->lkey;

		/* prepare the send work request */
		wr.wr_id = TEST_SET_WRID(TEST_RECV_WRID, 0);
		wr.next = NULL;
		wr.sg_list = &list;
		wr.num_sge = 1;

		for (i = 0; i < count; i++)
			if ((rc = ibv_post_recv(ctx->qp, &wr, &bad_wr)) != EOK) {
				EXPECT_EQ(EOK, rc);
				EXPECT_EQ(EOK, errno);
				break;
			}

		return i;
	}

	void __poll_cq(struct ibv_cq *cq, int num_entries, struct ibv_wc *wc,
			int expected_poll_cq_count)
	{
		int poll_result;
		int poll_cq_count = 0;

		unsigned long start_time_msec;
		unsigned long cur_time_msec;
		struct timeval cur_time;

		gettimeofday(&cur_time, NULL);
		start_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);
		do {
			poll_result = ibv_poll_cq(cq, num_entries, &wc[poll_cq_count]);
			ASSERT_TRUE(poll_result >= 0 );
			poll_cq_count += poll_result;

			gettimeofday(&cur_time, NULL);
			cur_time_msec = (cur_time.tv_sec * 1000)
						+ (cur_time.tv_usec / 1000);
		} while ((poll_cq_count < expected_poll_cq_count)
				&& ((cur_time_msec - start_time_msec)
						< MAX_POLL_CQ_TIMEOUT));
		ASSERT_EQ(expected_poll_cq_count, poll_cq_count);
	}
#endif //HAVE_CROSS_CHANNEL
};
#endif //_IBVERBS_CC_VERBS_TEST_
