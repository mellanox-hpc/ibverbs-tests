/**
 * Copyright (C) 2015-2016 Mellanox Technologies Ltd. All rights reserved.
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

#include "cc_classes.h"

/* tc_verbs_create_cq: [TI.1]
 * Every time you post to the send Q increment a counter.
 * Every time you get something back from ibv_poll_cq increment
 * another counter.
 * The (A - B) must never exceed the number of entries in the CQ,
 * and it must not exceed the number of entries in the send
 * Q (very important).
 * This test WON'T declare CQ to ignore overrun.
 */
TEST_F(tc_verbs_create_cq, ti_1){
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;
	int flags;
	int64_t	 wrid = 0;

	__init_test( 0, 0x1F, 0x1F,
		     0, 0x0F,
		     0, 0x1F);
	ASSERT_EQ(0x1F, ctx->qp_tx_depth);
	ASSERT_EQ(0x0F, ctx->cq_tx_depth);

	/*
	 * Changing the mode of events read to be non-blocking
	 */
	flags = fcntl(ctx->context->async_fd, F_GETFL);
	rc = fcntl(ctx->context->async_fd, F_SETFL, flags | O_NONBLOCK);
	ASSERT_FALSE(rc < 0);

	/*
	 * Use the created QP for communication operations.
	 */

	/* Do few posts/polls */
	rc = __post_write(ctx, 77, IBV_WR_SEND);
	ASSERT_EQ(EOK, rc);
	rc = __post_write(ctx, 66, IBV_WR_SEND);
	ASSERT_EQ(EOK, rc);
	__poll_cq(ctx->scq, 2, ctx->wc, 2);
	EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[0].status);
	EXPECT_EQ((uint64_t)(77), ctx->wc[0].wr_id);
	EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[1].status);
	EXPECT_EQ((uint64_t)(66), ctx->wc[1].wr_id);

	__poll_cq(ctx->rcq, 2, ctx->wc, 2);

	/*
	 * Check that it is impossible to post and poll number of WRs that
	 * greater than maximum number of CQE in SCQ
	 */

	/* Post number of WRs that exceeds maximum of CQE in CQ */
	do {
		rc = __post_write(ctx, wrid, IBV_WR_SEND);
		ASSERT_EQ(EOK, rc);
		++wrid;
	} while (wrid < (ctx->cq_tx_depth + 2));
	ASSERT_EQ(wrid, ctx->cq_tx_depth + 2);

	sleep(2);

	__poll_cq(ctx->scq, ctx->cq_tx_depth + 2, ctx->wc, wrid - 1);
	EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[wrid - 2].status);
	EXPECT_EQ((uint64_t)(wrid - 2), ctx->wc[wrid - 2].wr_id);

	__poll_cq(ctx->rcq, ctx->cq_rx_depth + 2, ctx->wc, wrid - 1);

	/* Check result */
	{
		struct ibv_async_event event;
		struct pollfd my_pollfd[2];
		int ms_timeout = 1000;

		/*
		 * poll the queue until it has an event and sleep ms_timeout
		 * milliseconds between any iteration
		 */
		my_pollfd[0].fd      = ctx->context->async_fd;
		my_pollfd[0].events  = POLLIN;
		my_pollfd[0].revents = 0;
		my_pollfd[1].fd      = ctx->context->async_fd;
		my_pollfd[1].events  = POLLIN;
		my_pollfd[1].revents = 0;
		rc = poll(my_pollfd, 2, ms_timeout);
		EXPECT_EQ(2, rc);

		if (rc > 0) {
			int i = 0;
			/*
			 * we know that there is an event (IBV_EVENT_CQ_ERR & IBV_EVENT_QP_FATAL),
			 * so we just need to read it
			 */
			while (i < 2) {
				rc = ibv_get_async_event(ctx->context, &event);
				ASSERT_EQ(EOK, rc);
				if (event.event_type == IBV_EVENT_CQ_ERR)
					EXPECT_EQ(ctx->scq, event.element.cq);
				else if (event.event_type == IBV_EVENT_QP_FATAL)
					EXPECT_EQ(ctx->qp, event.element.qp);
				else
					EXPECT_TRUE(0);

				i++;
				sleep(1);
				ibv_ack_async_event(&event);
			}
		}
	}
#endif
}

/* tc_verbs_create_cq: [TI.2]
 * This test sets CQ TX to ignore overrun
 */
TEST_F(tc_verbs_create_cq, ti_2){
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;
	int flags;
	int64_t	 wrid = 0;

	__init_test( 0, 0x1F, 0x1F,
		     IBV_CREATE_CQ_ATTR_IGNORE_OVERRUN, 0x0F,
		     0, 0x1F);
	ASSERT_EQ(0x1F, ctx->qp_tx_depth);
	ASSERT_EQ(0x0F, ctx->cq_tx_depth);

	/*
	 * Changing the mode of events read to be non-blocking
	 */
	flags = fcntl(ctx->context->async_fd, F_GETFL);
	rc = fcntl(ctx->context->async_fd, F_SETFL, flags | O_NONBLOCK);
	ASSERT_FALSE(rc < 0);

	/*
	 * Use the created QP for communication operations.
	 */

	/* Do few posts/polls */
	rc = __post_write(ctx, 77, IBV_WR_SEND);
	ASSERT_EQ(EOK, rc);
	rc = __post_write(ctx, 66, IBV_WR_SEND);
	ASSERT_EQ(EOK, rc);
	__poll_cq(ctx->scq, 2, ctx->wc, 2);
	EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[0].status);
	EXPECT_EQ((uint64_t)(77), ctx->wc[0].wr_id);
	EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[1].status);
	EXPECT_EQ((uint64_t)(66), ctx->wc[1].wr_id);
	__poll_cq(ctx->rcq, 2, ctx->wc, 2);

	/*
	 * Check that it is possible to post and poll number of WRs that
	 * greater than Maximum number of CQE in SCQ
	 */
	/* Post number of WRs that exceeds maximum of CQE in CQ */
	do {
		rc = __post_write(ctx, wrid, IBV_WR_SEND);
		ASSERT_EQ(EOK, rc);
		++wrid;
	} while (wrid < (ctx->cq_tx_depth + 2));

	EXPECT_EQ(ctx->cq_tx_depth + 2, wrid);

	sleep(2);

	__poll_cq(ctx->scq, ctx->cq_tx_depth + 2, ctx->wc, 0);
	__poll_cq(ctx->rcq, ctx->cq_rx_depth + 2, ctx->wc, ctx->cq_tx_depth + 2);

	/* Check if ERROR is raised */
	{
		struct pollfd my_pollfd[2];
		int ms_timeout = 1000;

		/*
		 * poll the queue until it has an event and sleep ms_timeout
		 * milliseconds between any iteration
		 */
		my_pollfd[0].fd      = ctx->context->async_fd;
		my_pollfd[0].events  = POLLIN;
		my_pollfd[0].revents = 0;
		my_pollfd[1].fd      = ctx->context->async_fd;
		my_pollfd[1].events  = POLLIN;
		my_pollfd[1].revents = 0;
		rc = poll(my_pollfd, 2, ms_timeout);
		EXPECT_EQ(0, rc);
	}
#endif
}

/* tc_verbs_create_cq: [TI.3]
 * This test sets CQ RX to ignore overrun
 */
TEST_F(tc_verbs_create_cq, ti_3){
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;
	int flags;
	int64_t	 wrid = 0;

	__init_test( 0, 0x1F, 0x1F,
		     0, 0x1F,
		     IBV_CREATE_CQ_ATTR_IGNORE_OVERRUN, 0x0F);
	ASSERT_EQ(0x1F, ctx->qp_tx_depth);
	ASSERT_EQ(0x0F, ctx->cq_rx_depth);

	/*
	 * Changing the mode of events read to be non-blocking
	 */
	flags = fcntl(ctx->context->async_fd, F_GETFL);
	rc = fcntl(ctx->context->async_fd, F_SETFL, flags | O_NONBLOCK);
	ASSERT_FALSE(rc < 0);

	/*
	 * Use the created QP for communication operations.
	 */

	/* Do few posts/polls */
	rc = __post_write(ctx, 77, IBV_WR_SEND);
	ASSERT_EQ(EOK, rc);
	rc = __post_write(ctx, 66, IBV_WR_SEND);
	ASSERT_EQ(EOK, rc);

	sleep(2);

	__poll_cq(ctx->scq, 2, ctx->wc, 2);
	EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[0].status);
	EXPECT_EQ((uint64_t)(77), ctx->wc[0].wr_id);
	EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[1].status);
	EXPECT_EQ((uint64_t)(66), ctx->wc[1].wr_id);
	__poll_cq(ctx->rcq, 2, ctx->wc, 2);

	/*
	 * Check that it is possible to post and poll number of WRs that
	 * greater than Maximum number of CQE in SCQ
	 */
	/* Post number of WRs that exceeds maximum of CQE in CQ */
	do {
		rc = __post_write(ctx, wrid, IBV_WR_SEND);
		ASSERT_EQ(EOK, rc);
		++wrid;
	} while (wrid < (ctx->cq_rx_depth + 2));

	EXPECT_EQ(ctx->cq_rx_depth + 2, wrid);
	__poll_cq(ctx->scq, ctx->cq_tx_depth + 2, ctx->wc, ctx->cq_rx_depth + 2);
	__poll_cq(ctx->rcq, ctx->cq_rx_depth + 2, ctx->wc, 0);

	/* Check if ERROR is raised */
	{
		struct pollfd my_pollfd[2];
		int ms_timeout = 1000;

		/*
		 * poll the queue until it has an event and sleep ms_timeout
		 * milliseconds between any iteration
		 */
		my_pollfd[0].fd      = ctx->context->async_fd;
		my_pollfd[0].events  = POLLIN;
		my_pollfd[0].revents = 0;
		my_pollfd[1].fd      = ctx->context->async_fd;
		my_pollfd[1].events  = POLLIN;
		my_pollfd[1].revents = 0;
		rc = poll(my_pollfd, 2, ms_timeout);
		EXPECT_EQ(0, rc);
	}
#endif
}
