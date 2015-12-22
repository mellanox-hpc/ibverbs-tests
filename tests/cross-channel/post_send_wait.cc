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

#include "cc_verbs_test.h"

class tc_verbs_post_send_wait : public cc_init_verbs_test {};

#define SEND_POST_COUNT		10

/* tc_verbs_post_send_wait: [TI.1]
 * IBV_WR_CQE_WAIT
 * should return EINVAL error code for usual QP that does not
 * support Cross-Channel IO Operations
 */
TEST_F(tc_verbs_post_send_wait, ti_1) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	__init_test();

	rc = __post_write(ctx, 0, IBV_WR_CQE_WAIT);
	EXPECT_NE(rc, EOK);
	EXPECT_TRUE((EINVAL == rc) || (EINVAL == errno));
#endif //HAVE_CROSS_CHANNEL
}

/* tc_verbs_post_send_wait: [TI.2]
 * IBV_WR_CQE_WAIT is posted to QP created with IBV_QP_CREATE_CROSS_CHANNEL
 * cqe_wait.cq_count = 1
 * Expected ONE CQE on mcq
 */
TEST_F(tc_verbs_post_send_wait, ti_2) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;
	struct ibv_send_wr wr;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL);

	wr.wr_id = 0;
	wr.next = NULL;
	wr.sg_list = NULL;
	wr.num_sge = 0;
	wr.opcode = IBV_WR_CQE_WAIT;
	wr.send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
	wr.imm_data = 0;

	wr.wr.cqe_wait.cq = ctx->rcq;
	wr.wr.cqe_wait.cq_count = 1;
	rc = ibv_post_send(ctx->mqp, &wr, NULL);
	ASSERT_EQ(EOK, rc);

	rc = __post_write(ctx, 1, IBV_WR_SEND);
	ASSERT_EQ(EOK, rc);

	__poll_cq(ctx->mcq, 0x10, ctx->wc, 1);
	EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[0].status);

#endif //HAVE_CROSS_CHANNEL
}
/* tc_verbs_post_send_wait: [TI.3]
 * IBV_WR_CQE_WAIT is posted to QP created with IBV_QP_CREATE_CROSS_CHANNEL
 * cqe_wait.cq_count < send request posted
 * Expected ONE CQE on mcq
 */
TEST_F(tc_verbs_post_send_wait, ti_3) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	int64_t	 wrid = 0;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL);

	/*
	 * Use the created QP for communication operations.
	 */

	do {
		if (wrid == 0)
		{
			struct ibv_send_wr wr;

			wr.wr_id = wrid;
			wr.next = NULL;
			wr.sg_list = NULL;
			wr.num_sge = 0;
			wr.opcode = IBV_WR_CQE_WAIT;
			wr.send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
			wr.imm_data = 0;

			wr.wr.cqe_wait.cq = ctx->rcq;
			wr.wr.cqe_wait.cq_count = 1;
			rc = ibv_post_send(ctx->mqp, &wr, NULL);
			ASSERT_EQ(EOK, rc);
		}
		rc = __post_write(ctx, wrid, IBV_WR_SEND);
		ASSERT_EQ(EOK, rc);
		++wrid;
	} while (wrid < SEND_POST_COUNT);

	__poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc, SEND_POST_COUNT);
	__poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc, SEND_POST_COUNT);
	__poll_cq(ctx->mcq, 0x10, ctx->wc, 1);
	EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[0].status);
#endif //HAVE_CROSS_CHANNEL
}

/* tc_verbs_post_send_wait: [TI.4]
 * IBV_WR_CQE_WAIT is posted to QP created with IBV_QP_CREATE_CROSS_CHANNEL
 * cqe_wait.cq_count > send request posted
 * Expected NONE CQE on mcq
 */
TEST_F(tc_verbs_post_send_wait, ti_4) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	int64_t	 wrid = 0;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL);

	/*
	 * Use the created QP for communication operations.
	 */
	do {
		if (wrid == 0)
		{
			struct ibv_send_wr wr;

			wr.wr_id = wrid;
			wr.next = NULL;
			wr.sg_list = NULL;
			wr.num_sge = 0;
			wr.opcode = IBV_WR_CQE_WAIT;
			wr.send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
			wr.imm_data = 0;

			wr.wr.cqe_wait.cq = ctx->rcq;
			wr.wr.cqe_wait.cq_count = (SEND_POST_COUNT + 1);

			rc = ibv_post_send(ctx->mqp, &wr, NULL);
			ASSERT_EQ(EOK, rc);
		}

		rc = __post_write(ctx, wrid, IBV_WR_SEND);
		ASSERT_EQ(EOK, rc);
		++wrid;
	} while (wrid < SEND_POST_COUNT);

	__poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc, SEND_POST_COUNT);
	__poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc, SEND_POST_COUNT);
	__poll_cq(ctx->mcq, 0x10, ctx->wc, 0);
#endif //HAVE_CROSS_CHANNEL
}

/* tc_verbs_post_send_wait: [TI.5]
 * IBV_WR_CQE_WAIT is posted to QP created with IBV_QP_CREATE_CROSS_CHANNEL
 * cqe_wait.cq_count == send request posted
 * Expected ONE CQE on mcq
 */
TEST_F(tc_verbs_post_send_wait, ti_5) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	int64_t	 wrid = 0;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL);

	/*
	 * Use the created QP for communication operations.
	 */
	do {
		if (wrid == 0)
		{
			struct ibv_send_wr wr;

			wr.wr_id = wrid;
			wr.next = NULL;
			wr.sg_list = NULL;
			wr.num_sge = 0;
			wr.opcode = IBV_WR_CQE_WAIT;
			wr.send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
			wr.imm_data = 0;

			wr.wr.cqe_wait.cq = ctx->rcq;
			wr.wr.cqe_wait.cq_count = SEND_POST_COUNT;

			rc = ibv_post_send(ctx->mqp, &wr, NULL);
			ASSERT_EQ(EOK, rc);
		}

		__poll_cq(ctx->mcq, 0x10, ctx->wc, 0);

		rc = __post_write(ctx, wrid, IBV_WR_SEND);
		ASSERT_EQ(EOK, rc);
		++wrid;
	} while (wrid < SEND_POST_COUNT);

	__poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc, SEND_POST_COUNT);
	__poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc, SEND_POST_COUNT);
	__poll_cq(ctx->mcq, 0x10, ctx->wc, 1);
	EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[0].status);
#endif //HAVE_CROSS_CHANNEL
}

/* tc_verbs_post_send_wait: [TI.6]
 * Three IBV_WR_CQE_WAIT are posted to QP created with IBV_QP_CREATE_CROSS_CHANNEL
 * cqe_wait.cq_count == send request posted
 * Expected THREE CQE on mcq
 * Note: every WAIT generates CQE
 */
TEST_F(tc_verbs_post_send_wait, ti_6) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	int64_t	 wrid = 0;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL);

	/*
	 * Use the created QP for communication operations.
	 */
	do {
		if (wrid == 0)
		{
			struct ibv_send_wr wr[3];

			/* clean up WCs */
			memset(ctx->wc, 0, 3 * sizeof(struct ibv_wc));

			wr[0].wr_id = 3;
			wr[0].next = &wr[1];
			wr[0].sg_list = NULL;
			wr[0].num_sge = 0;
			wr[0].opcode = IBV_WR_CQE_WAIT;
			wr[0].send_flags = IBV_SEND_SIGNALED;
			wr[0].imm_data = 0;

			wr[1].wr_id = 2;
			wr[1].next = &wr[2];
			wr[1].sg_list = NULL;
			wr[1].num_sge = 0;
			wr[1].opcode = IBV_WR_CQE_WAIT;
			wr[1].send_flags = IBV_SEND_SIGNALED;
			wr[1].imm_data = 0;

			wr[2].wr_id = 1;
			wr[2].next = NULL;
			wr[2].sg_list = NULL;
			wr[2].num_sge = 0;
			wr[2].opcode = IBV_WR_CQE_WAIT;
			wr[2].send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
			wr[2].imm_data = 0;

			wr[0].wr.cqe_wait.cq = ctx->rcq;
			wr[0].wr.cqe_wait.cq_count = SEND_POST_COUNT;
			wr[1].wr.cqe_wait.cq = ctx->rcq;
			wr[1].wr.cqe_wait.cq_count = SEND_POST_COUNT;
			wr[2].wr.cqe_wait.cq = ctx->rcq;
			wr[2].wr.cqe_wait.cq_count = SEND_POST_COUNT;

			rc = ibv_post_send(ctx->mqp, wr, NULL);
			ASSERT_EQ(EOK, rc);
		}

		__poll_cq(ctx->mcq, 0x10, ctx->wc, 0);

		rc = __post_write(ctx, wrid, IBV_WR_SEND);
		ASSERT_EQ(EOK, rc);
		++wrid;
	} while (wrid < SEND_POST_COUNT);

	__poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc, SEND_POST_COUNT);
	__poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc, SEND_POST_COUNT);
	__poll_cq(ctx->mcq, 0x10, ctx->wc, 3);
	EXPECT_EQ((uint64_t)3, ctx->wc[0].wr_id);
	EXPECT_EQ(ctx->mqp->qp_num, ctx->wc[0].qp_num);
	EXPECT_EQ((uint64_t)2, ctx->wc[1].wr_id);
	EXPECT_EQ(ctx->mqp->qp_num, ctx->wc[1].qp_num);
	EXPECT_EQ((uint64_t)1, ctx->wc[2].wr_id);
	EXPECT_EQ(ctx->mqp->qp_num, ctx->wc[2].qp_num);
#endif //HAVE_CROSS_CHANNEL
}

/* tc_verbs_post_send_wait: [TI.7]
 * Three IBV_WR_CQE_WAIT are posted to QP created with IBV_QP_CREATE_CROSS_CHANNEL
 * cqe_wait(1).cq_count == SEND_POST_COUNT - 1
 * cqe_wait(2).cq_count == SEND_POST_COUNT
 * cqe_wait(3).cq_count == SEND_POST_COUNT + 1
 * Expected TWO CQE on mcq
 * Note: internal index is relative previous post and incremented after special WAIT posted
 */
TEST_F(tc_verbs_post_send_wait, ti_7) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	int64_t	 wrid = 0;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL);

	/*
	 * Use the created QP for communication operations.
	 */
	do {
		if (wrid == 0)
		{
			struct ibv_send_wr wr[3];

			/* clean up WCs */
			memset(ctx->wc, 0, 3 * sizeof(struct ibv_wc));

			wr[0].wr_id = 3;
			wr[0].next = &wr[1];
			wr[0].sg_list = NULL;
			wr[0].num_sge = 0;
			wr[0].opcode = IBV_WR_CQE_WAIT;
			wr[0].send_flags = IBV_SEND_SIGNALED;
			wr[0].imm_data = 0;

			wr[0].wr.cqe_wait.cq = ctx->rcq;
			wr[0].wr.cqe_wait.cq_count = SEND_POST_COUNT - 1;

			wr[1].wr_id = 2;
			wr[1].next = &wr[2];
			wr[1].sg_list = NULL;
			wr[1].num_sge = 0;
			wr[1].opcode = IBV_WR_CQE_WAIT;
			wr[1].send_flags = IBV_SEND_SIGNALED;
			wr[1].imm_data = 0;

			wr[1].wr.cqe_wait.cq = ctx->rcq;
			wr[1].wr.cqe_wait.cq_count = SEND_POST_COUNT;

			wr[2].wr_id = 1;
			wr[2].next = NULL;
			wr[2].sg_list = NULL;
			wr[2].num_sge = 0;
			wr[2].opcode = IBV_WR_CQE_WAIT;
			wr[2].send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
			wr[2].imm_data = 0;

			wr[2].wr.cqe_wait.cq = ctx->rcq;
			wr[2].wr.cqe_wait.cq_count = SEND_POST_COUNT + 1;

			rc = ibv_post_send(ctx->mqp, wr, NULL);
			ASSERT_EQ(EOK, rc);
		}

		__poll_cq(ctx->mcq, 0x10, ctx->wc, 0);

		rc = __post_write(ctx, wrid, IBV_WR_SEND);
		ASSERT_EQ(EOK, rc);
		++wrid;
	} while (wrid < SEND_POST_COUNT);

	__poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc, SEND_POST_COUNT);
	__poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc, SEND_POST_COUNT);
	__poll_cq(ctx->mcq, 0x10, ctx->wc, 2);
	EXPECT_EQ((uint64_t)3, ctx->wc[0].wr_id);
	EXPECT_EQ(ctx->mqp->qp_num, ctx->wc[0].qp_num);
	EXPECT_EQ((uint64_t)2, ctx->wc[1].wr_id);
	EXPECT_EQ(ctx->mqp->qp_num, ctx->wc[1].qp_num);
#endif //HAVE_CROSS_CHANNEL
}

/* tc_verbs_post_send_wait: [TI.8]
 * Set cq_count that exceeds SCQ deep
 */
TEST_F(tc_verbs_post_send_wait, ti_8) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;
	int64_t	wrid = 0;

	__init_test( IBV_QP_CREATE_CROSS_CHANNEL, 0x1F, 0x1F,
		     IBV_CREATE_CQ_ATTR_IGNORE_OVERRUN, 0x0F,
		     0, 0x1F);
	ASSERT_EQ(0x1F, ctx->qp_tx_depth);
	ASSERT_EQ(0x0F, ctx->cq_tx_depth);

	{
		struct ibv_send_wr wr;

		wr.wr_id = 777;
		wr.next = NULL;
		wr.sg_list = NULL;
		wr.num_sge = 0;
		wr.opcode = IBV_WR_CQE_WAIT;
		wr.send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
		wr.imm_data = 0;

		wr.wr.cqe_wait.cq = ctx->rcq;
		wr.wr.cqe_wait.cq_count = ctx->cq_tx_depth + 3;

		rc = ibv_post_send(ctx->mqp, &wr, NULL);
		ASSERT_EQ(EOK, rc);
	}

	/*
	 * Post number of WRs that
	 * equal maximum number of CQE in SCQ
	 */
	/* Post number of WRs that exceeds maximum of CQE in CQ */
	do {
		rc = __post_write(ctx, wrid, IBV_WR_SEND);
		ASSERT_EQ(EOK, rc);
		++wrid;
	} while (wrid < (ctx->cq_tx_depth + 2));

	__poll_cq(ctx->scq, ctx->cq_tx_depth + SEND_POST_COUNT, ctx->wc, 0);
	__poll_cq(ctx->rcq, ctx->cq_rx_depth + SEND_POST_COUNT, ctx->wc, ctx->cq_tx_depth + 2);
	__poll_cq(ctx->mcq, 0x10, ctx->wc, 0);

	/*
	 * Post number of WRs that
	 * greater than Maximum number of CQE in SCQ
	 */
	/* Post number of WRs that exceeds maximum of CQE in CQ */
	do {
		rc = __post_write(ctx, wrid, IBV_WR_SEND);
		ASSERT_EQ(EOK, rc);
		++wrid;
	} while (wrid < (ctx->cq_tx_depth + SEND_POST_COUNT));

	__poll_cq(ctx->scq, ctx->cq_tx_depth + SEND_POST_COUNT, ctx->wc, 0);
	__poll_cq(ctx->rcq, ctx->cq_rx_depth + SEND_POST_COUNT, ctx->wc, SEND_POST_COUNT - 2);
	__poll_cq(ctx->mcq, 0x10, ctx->wc, 1);
	EXPECT_EQ((uint64_t)777, ctx->wc[0].wr_id);
	EXPECT_EQ(ctx->mqp->qp_num, ctx->wc[0].qp_num);
#endif //HAVE_CROSS_CHANNEL
}
