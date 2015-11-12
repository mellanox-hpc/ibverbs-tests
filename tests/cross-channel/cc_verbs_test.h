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

#define QP_PORT			1
#define MQP_PORT		1

/**
 * Base class for Cross-Channel ibverbs test fixtures.
 * Initialize and close ibverbs.
 */
class cc_base_verbs_test : public verbs_test {
protected:
	virtual void SetUp() {
#ifdef HAVE_INFINIBAND_VERBS_EXP_H
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
#endif //HAVE_INFINIBAND_VERBS_EXP_H
	}
	virtual void TearDown() {
		if (!skip_this_test)
			verbs_test::TearDown();
	}

protected:
#ifndef HAVE_INFINIBAND_VERBS_EXP_H
	struct ibv_device_attr_ex device_attr;
#endif //HAVE_INFINIBAND_VERBS_EXP_H
};

/**
 * Class for Cross-Channel create_cq checks
 */
class cc_init_verbs_test : public cc_base_verbs_test {
protected:
	virtual void SetUp() {
		int rc = 0;
		cc_base_verbs_test::SetUp();
		if (skip_this_test)
			return;

#ifndef HAVE_INFINIBAND_VERBS_EXP_H
		EXPECT_TRUE((device_attr.orig_attr.device_cap_flags & IBV_DEVICE_CROSS_CHANNEL))
				<< "This class of tests is for Cross-Channel functionality."
				<< " But device does not support one";
#endif //HAVE_INFINIBAND_VERBS_EXP_H

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
#ifndef HAVE_INFINIBAND_VERBS_EXP_H
			init_attr.comp_mask |= IBV_QP_INIT_ATTR_CREATE_FLAGS | IBV_QP_INIT_ATTR_PD;
			init_attr.create_flags = IBV_QP_CREATE_CROSS_CHANNEL;
			ctx->mqp = ibv_create_qp_ex(ctx->context, &init_attr);
			ASSERT_TRUE(ctx->mqp != NULL);
#endif //HAVE_INFINIBAND_VERBS_EXP_H
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
			attr.ah_attr.port_num      = 0;

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
	}

	virtual void TearDown() {
		if(skip_this_test)
			return;

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
		if (ctx->wc)
			free(ctx->wc);
		if (ctx->ah)
			ibv_destroy_ah(ctx->ah);
		if (ctx->mr)
			ibv_dereg_mr(ctx->mr);
		if (ctx->pd)
			ibv_dealloc_pd(ctx->pd);
	        if (ctx->net_buf)
			free(ctx->net_buf);
	        if (ctx->buf)
			free(ctx->buf);
	        if (ctx->last_result)
			free(ctx->last_result);
	        if (ctx)
			free(ctx);

	        ctx = NULL;

		cc_base_verbs_test::TearDown();
	}
protected:
	struct test_context *ctx;

};
#endif //_IBVERBS_CC_VERBS_TEST_
