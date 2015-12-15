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

class tc_verbs_post_send_en : public cc_init_verbs_test {};

#define SEND_POST_COUNT		10
#define SEND_EN_WR_ID		((uint64_t)'M')

/* tc_verbs_post_send_en: [TI.1]
 * IBV_QP_CREATE_CROSS_CHANNEL does not change legacy behaviour
 */
TEST_F(tc_verbs_post_send_en, ti_1) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	__init_test( IBV_QP_CREATE_CROSS_CHANNEL );

	/*
	 * Use the created QP for communication operations.
	 */
	{
		int i;
		int routs;
		int rcnt, scnt;
		int64_t	 wrid = 0;
		unsigned long start_time_msec;
		unsigned long cur_time_msec;
		struct timeval cur_time;
		int poll_result;
		int s_poll_cq_count = 0;
		int r_poll_cq_count = 0;

		routs = ctx->qp_rx_depth;

		rcnt = 0;
		scnt = 0;
		gettimeofday(&cur_time, NULL);
		start_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);
		do {
			if (wrid < SEND_POST_COUNT) {
				rc = __post_write(ctx, TEST_SET_WRID(TEST_SEND_WRID, wrid), IBV_WR_SEND);
				ASSERT_EQ(EOK, rc);
				++wrid;
			}

			/* poll the completion for a while before giving up of doing it .. */
			poll_result = ibv_poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc);
			ASSERT_TRUE(poll_result >= 0);

			s_poll_cq_count += poll_result;

			/* CQE found */
			for (i = 0; i < poll_result; ++i) {
				if (ctx->wc[i].status != IBV_WC_SUCCESS) {
					VERBS_TRACE("Failed status %s (%d) for wr_id %d\n",
						ibv_wc_status_str(ctx->wc[i].status),
						ctx->wc[i].status, (int) ctx->wc[i].wr_id);
					ASSERT_TRUE(0);
				}

				switch (TEST_GET_TYPE(ctx->wc[i].wr_id)) {
				case TEST_SEND_WRID:
					++scnt;
					break;

				case TEST_RECV_WRID:
					if (--routs <= 1) {
						routs += __post_read(ctx, ctx->qp_rx_depth - routs);
						if (routs < ctx->qp_rx_depth) {
							VERBS_TRACE("Couldn't post receive (%d)\n", routs);
							ASSERT_TRUE(0);
						}
					}

					++rcnt;
					break;

				default:
					VERBS_TRACE("Completion for unknown wr_id %d\n",
							(int) ctx->wc[i].wr_id);
					break;
				}
				VERBS_INFO("%-10s : wr_id=%-4" PRIu64 " scnt=%-4d rcnt=%-4d poll=%d\n",
						wr_id2str(ctx->wc[i].wr_id),
						TEST_GET_WRID(ctx->wc[i].wr_id),
						scnt, rcnt, poll_result);
			}

			poll_result = ibv_poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc);
			ASSERT_TRUE(poll_result >= 0);

			r_poll_cq_count += poll_result;

			/* CQE found */
			for (i = 0; i < poll_result; ++i) {
				if (ctx->wc[i].status != IBV_WC_SUCCESS) {
					VERBS_TRACE("Failed status %s (%d) for wr_id %d\n",
						ibv_wc_status_str(ctx->wc[i].status),
						ctx->wc[i].status, (int) ctx->wc[i].wr_id);
					ASSERT_TRUE(0);
				}

				switch (TEST_GET_TYPE(ctx->wc[i].wr_id)) {
				case TEST_SEND_WRID:
					++scnt;
					break;

				case TEST_RECV_WRID:
					if (--routs <= 1) {
						routs += __post_read(ctx, ctx->qp_rx_depth - routs);
						if (routs < ctx->qp_rx_depth) {
							VERBS_TRACE("Couldn't post receive (%d)\n", routs);
							ASSERT_TRUE(0);
						}
					}

					++rcnt;
					break;

				default:
					VERBS_TRACE("Completion for unknown wr_id %d\n",
							(int) ctx->wc[i].wr_id);
					break;
				}
				VERBS_INFO("%-10s : wr_id=%-4" PRIu64 " scnt=%-4d rcnt=%-4d poll=%d\n",
						wr_id2str(ctx->wc[i].wr_id),
						TEST_GET_WRID(ctx->wc[i].wr_id),
						scnt, rcnt, poll_result);
			}
			gettimeofday(&cur_time, NULL);
			cur_time_msec = (cur_time.tv_sec * 1000)
					+ (cur_time.tv_usec / 1000);
		} while ((poll_result >= 0)
				&& ((cur_time_msec - start_time_msec)
						< MAX_POLL_CQ_TIMEOUT));

		EXPECT_EQ(SEND_POST_COUNT, wrid);
		EXPECT_EQ(SEND_POST_COUNT, s_poll_cq_count);
		EXPECT_EQ(SEND_POST_COUNT, r_poll_cq_count);
		EXPECT_EQ(scnt, rcnt);
	}
#endif //HAVE_CROSS_CHANNEL
}

/* tc_verbs_post_send_en: [TI.2]
 * IBV_WR_SEND_ENABLE, IBV_WR_RECV_ENABLE, IBV_WR_CQE_WAIT,
 * should return EINVAL error code for usual QP that does not
 * support Cross-Channel IO Operations
 */
TEST_F(tc_verbs_post_send_en, ti_2) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	__init_test();

	rc = __post_write(ctx, TEST_SEND_WRID, IBV_WR_SEND_ENABLE);
	EXPECT_NE(rc, EOK);
	EXPECT_TRUE((EINVAL == rc) || (EINVAL == errno));

	rc = __post_write(ctx, TEST_SEND_WRID, IBV_WR_RECV_ENABLE);
	EXPECT_NE(rc, EOK);
	EXPECT_TRUE((EINVAL == rc) || (EINVAL == errno));

	rc = __post_write(ctx, TEST_SEND_WRID, IBV_WR_CQE_WAIT);
	EXPECT_NE(rc, EOK);
	EXPECT_TRUE((EINVAL == rc) || (EINVAL == errno));
#endif //HAVE_CROSS_CHANNEL
}

/* tc_verbs_post_send_en: [TI.3]
 * IBV_WR_SEND_ENABLE does not return CQE for QP created
 * w/o IBV_QP_CREATE_MANAGED_SEND
 */
TEST_F(tc_verbs_post_send_en, ti_3) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL);

	/*
	 * Use the created QP for communication operations.
	 */
	{
		int routs;
		int rcnt, scnt;
		int64_t	 wrid = 0;
		unsigned long start_time_msec;
		unsigned long cur_time_msec;
		struct timeval cur_time;
		int poll_result;
		int s_poll_cq_count = 0;
		int r_poll_cq_count = 0;

		routs = ctx->qp_rx_depth;

		rcnt = 0;
		scnt = 0;
		gettimeofday(&cur_time, NULL);
		start_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);
		do {
			if (wrid < SEND_POST_COUNT) {
				rc = __post_write(ctx, wrid, IBV_WR_SEND_ENABLE);
				ASSERT_NE(EOK, rc);
				++wrid;
			}

			gettimeofday(&cur_time, NULL);
			cur_time_msec = (cur_time.tv_sec * 1000)
					+ (cur_time.tv_usec / 1000);
		} while ((wrid < SEND_POST_COUNT)
				|| ((cur_time_msec - start_time_msec)
						< MAX_POLL_CQ_TIMEOUT));

		poll_result = ibv_poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc);
		ASSERT_TRUE(poll_result >= 0);
		s_poll_cq_count += poll_result;

		poll_result = ibv_poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc);
		ASSERT_TRUE(poll_result >= 0);
		r_poll_cq_count += poll_result;

		EXPECT_EQ(SEND_POST_COUNT, wrid);
		EXPECT_EQ(0, s_poll_cq_count);
		EXPECT_EQ(0, r_poll_cq_count);
	}
#endif //HAVE_CROSS_CHANNEL
}

/* tc_verbs_post_send_en: [TI.4]
 * IBV_WR_SEND is not posted to QP created with
 * IBV_QP_CREATE_CROSS_CHANNEL | IBV_QP_CREATE_MANAGED_SEND
 */
TEST_F(tc_verbs_post_send_en, ti_4) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL | IBV_QP_CREATE_MANAGED_SEND);

	/*
	 * Use the created QP for communication operations.
	 */
	{
		int routs;
		int64_t	 wrid = 0;
		unsigned long start_time_msec;
		unsigned long cur_time_msec;
		struct timeval cur_time;
		int poll_result;
		int s_poll_cq_count = 0;
		int r_poll_cq_count = 0;

		routs = ctx->qp_rx_depth;

		gettimeofday(&cur_time, NULL);
		start_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);
		do {
			if (wrid < SEND_POST_COUNT) {
				rc = __post_write(ctx, wrid, IBV_WR_SEND);
				ASSERT_EQ(EOK, rc);
				++wrid;
			}

			gettimeofday(&cur_time, NULL);
			cur_time_msec = (cur_time.tv_sec * 1000)
					+ (cur_time.tv_usec / 1000);
		} while ((wrid < SEND_POST_COUNT)
				|| ((cur_time_msec - start_time_msec)
						< MAX_POLL_CQ_TIMEOUT));

		poll_result = ibv_poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc);
		ASSERT_TRUE(poll_result >= 0);
		s_poll_cq_count += poll_result;

		poll_result = ibv_poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc);
		ASSERT_TRUE(poll_result >= 0);
		r_poll_cq_count += poll_result;

		EXPECT_EQ(SEND_POST_COUNT, wrid);
		EXPECT_EQ(0, s_poll_cq_count);
		EXPECT_EQ(0, r_poll_cq_count);
	}
#endif //HAVE_CROSS_CHANNEL
}

/* tc_verbs_post_send_en: [TI.5]
 * IBV_QP_CREATE_MANAGED_SEND does not affect to the same QP
 */
TEST_F(tc_verbs_post_send_en, ti_5) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL | IBV_QP_CREATE_MANAGED_SEND);

	/*
	 * Use the created QP for communication operations.
	 */
	{
		int routs;
		int rcnt, scnt;
		int64_t	 wrid = 0;
		unsigned long start_time_msec;
		unsigned long cur_time_msec;
		struct timeval cur_time;
		int poll_result;
		int s_poll_cq_count = 0;
		int r_poll_cq_count = 0;

		routs = ctx->qp_rx_depth;

		rcnt = 0;
		scnt = 0;
		gettimeofday(&cur_time, NULL);
		start_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);
		do {
			if (wrid < SEND_POST_COUNT) {
				rc = __post_write(ctx, wrid, ((wrid % 2) ? IBV_WR_SEND_ENABLE : IBV_WR_SEND));
				ASSERT_EQ(EOK, rc);
				++wrid;
			}

			gettimeofday(&cur_time, NULL);
			cur_time_msec = (cur_time.tv_sec * 1000)
					+ (cur_time.tv_usec / 1000);
		} while ((wrid < SEND_POST_COUNT)
				|| ((cur_time_msec - start_time_msec)
						< MAX_POLL_CQ_TIMEOUT));

		poll_result = ibv_poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc);
		ASSERT_TRUE(poll_result >= 0);
		s_poll_cq_count += poll_result;

		poll_result = ibv_poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc);
		ASSERT_TRUE(poll_result >= 0);
		r_poll_cq_count += poll_result;

		EXPECT_EQ(SEND_POST_COUNT, wrid);
		EXPECT_EQ(0, s_poll_cq_count);
		EXPECT_EQ(0, r_poll_cq_count);
	}
#endif //HAVE_CROSS_CHANNEL
}

/* tc_verbs_post_send_en: [TI.6]
 * IBV_QP_CREATE_MANAGED_SEND sent from MQP is able to push send requests
 * sequence SEND-SEND_EN-...
 */
TEST_F(tc_verbs_post_send_en, ti_6) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL | IBV_QP_CREATE_MANAGED_SEND);

	/*
	 * Use the created QP for communication operations.
	 */
	{
		int i;
		int routs;
		int rcnt, scnt;
		int64_t	 wrid = 0;
		unsigned long start_time_msec;
		unsigned long cur_time_msec;
		struct timeval cur_time;
		int poll_result;
		int s_poll_cq_count = 0;
		int r_poll_cq_count = 0;
		int m_poll_cq_count = 0;

		routs = ctx->qp_rx_depth;

		rcnt = 0;
		scnt = 0;
		gettimeofday(&cur_time, NULL);
		start_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);
		do {
			if (wrid < SEND_POST_COUNT) {
				if (wrid % 2) {
					struct ibv_send_wr wr_en;

					wr_en.wr_id = SEND_EN_WR_ID;
					wr_en.next = NULL;
					wr_en.sg_list = NULL;
					wr_en.num_sge = 0;
					wr_en.opcode = IBV_WR_SEND_ENABLE;
					wr_en.send_flags = IBV_SEND_WAIT_EN_LAST;
					wr_en.imm_data = 0;

					wr_en.task.wqe_enable.qp   = ctx->qp;
					wr_en.task.wqe_enable.wqe_count = 1;

					rc = ibv_post_send(ctx->mqp, &wr_en, NULL);
				}
				else
					rc = __post_write(ctx, TEST_SET_WRID(TEST_SEND_WRID, wrid), IBV_WR_SEND);

				ASSERT_EQ(EOK, rc);
				++wrid;
			}

			/* poll the completion for a while before giving up of doing it .. */
			poll_result = ibv_poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc);
			ASSERT_TRUE(poll_result >= 0);

			s_poll_cq_count += poll_result;

			/* CQE found */
			for (i = 0; i < poll_result; ++i) {
				if (ctx->wc[i].status != IBV_WC_SUCCESS) {
					VERBS_TRACE("Failed status %s (%d) for wr_id %d\n",
						ibv_wc_status_str(ctx->wc[i].status),
						ctx->wc[i].status, (int) ctx->wc[i].wr_id);
					ASSERT_TRUE(0);
				}

				switch (TEST_GET_TYPE(ctx->wc[i].wr_id)) {
				case TEST_SEND_WRID:
					++scnt;
					break;

				case TEST_RECV_WRID:
					if (--routs <= 1) {
						routs += __post_read(ctx, ctx->qp_rx_depth - routs);
						if (routs < ctx->qp_rx_depth) {
							VERBS_TRACE("Couldn't post receive (%d)\n", routs);
							ASSERT_TRUE(0);
						}
					}

					++rcnt;
					break;

				default:
					VERBS_TRACE("Completion for unknown wr_id %d\n",
							(int) ctx->wc[i].wr_id);
					break;
				}
				VERBS_INFO("%-10s : wr_id=%-4" PRIu64 " scnt=%-4d rcnt=%-4d poll=%d\n",
						wr_id2str(ctx->wc[i].wr_id),
						TEST_GET_WRID(ctx->wc[i].wr_id),
						scnt, rcnt, poll_result);
			}

			poll_result = ibv_poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc);
			ASSERT_TRUE(poll_result >= 0);

			r_poll_cq_count += poll_result;

			/* CQE found */
			for (i = 0; i < poll_result; ++i) {
				if (ctx->wc[i].status != IBV_WC_SUCCESS) {
					VERBS_TRACE("Failed status %s (%d) for wr_id %d\n",
						ibv_wc_status_str(ctx->wc[i].status),
						ctx->wc[i].status, (int) ctx->wc[i].wr_id);
					ASSERT_TRUE(0);
				}

				switch (TEST_GET_TYPE(ctx->wc[i].wr_id)) {
				case TEST_SEND_WRID:
					++scnt;
					break;

				case TEST_RECV_WRID:
					if (--routs <= 1) {
						routs += __post_read(ctx, ctx->qp_rx_depth - routs);
						if (routs < ctx->qp_rx_depth) {
							VERBS_TRACE("Couldn't post receive (%d)\n", routs);
							ASSERT_TRUE(0);
						}
					}

					++rcnt;
					break;

				default:
					VERBS_TRACE("Completion for unknown wr_id %d\n",
							(int) ctx->wc[i].wr_id);
					break;
				}
				VERBS_INFO("%-10s : wr_id=%-4" PRIu64 " scnt=%-4d rcnt=%-4d poll=%d\n",
						wr_id2str(ctx->wc[i].wr_id),
						TEST_GET_WRID(ctx->wc[i].wr_id),
						scnt, rcnt, poll_result);
			}

			gettimeofday(&cur_time, NULL);
			cur_time_msec = (cur_time.tv_sec * 1000)
					+ (cur_time.tv_usec / 1000);
		} while ((wrid < SEND_POST_COUNT)
				|| ((cur_time_msec - start_time_msec)
						< MAX_POLL_CQ_TIMEOUT));

		EXPECT_EQ(SEND_POST_COUNT, wrid);
		EXPECT_EQ((SEND_POST_COUNT/2), s_poll_cq_count);
		EXPECT_EQ((SEND_POST_COUNT/2), r_poll_cq_count);
		EXPECT_EQ(scnt, rcnt);

		poll_result = ibv_poll_cq(ctx->mcq, 0x10, ctx->wc);
		ASSERT_TRUE(poll_result >= 0);
		m_poll_cq_count += poll_result;
		EXPECT_EQ(0, m_poll_cq_count);
	}
#endif //HAVE_CROSS_CHANNEL
}

/* tc_verbs_post_send_en: [TI.7]
 * IBV_QP_CREATE_MANAGED_SEND sent from MQP is able to push send requests
 * sequence SEND-...-SEND-SEND_EN(all)
 */
TEST_F(tc_verbs_post_send_en, ti_7) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL | IBV_QP_CREATE_MANAGED_SEND);

	/*
	 * Use the created QP for communication operations.
	 */
	{
		int i;
		int routs;
		int rcnt, scnt;
		int64_t	 wrid = 0;
		unsigned long start_time_msec;
		unsigned long cur_time_msec;
		struct timeval cur_time;
		int poll_result;
		int s_poll_cq_count = 0;
		int r_poll_cq_count = 0;
		int m_poll_cq_count = 0;

		routs = ctx->qp_rx_depth;

		rcnt = 0;
		scnt = 0;
		gettimeofday(&cur_time, NULL);
		start_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);
		do {
			if (wrid < SEND_POST_COUNT) {
				rc = __post_write(ctx, TEST_SET_WRID(TEST_SEND_WRID, wrid), IBV_WR_SEND);
				EXPECT_EQ(EOK, rc);

				if (wrid == (SEND_POST_COUNT - 1)) {
					struct ibv_send_wr wr_en;

					wr_en.wr_id = SEND_EN_WR_ID;
					wr_en.next = NULL;
					wr_en.sg_list = NULL;
					wr_en.num_sge = 0;
					wr_en.opcode = IBV_WR_SEND_ENABLE;
					wr_en.send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
					wr_en.imm_data = 0;

					wr_en.task.wqe_enable.qp   = ctx->qp;
					wr_en.task.wqe_enable.wqe_count = 0;

					rc = ibv_post_send(ctx->mqp, &wr_en, NULL);
					ASSERT_EQ(EOK, rc);
				}
				++wrid;
			}

			/* poll the completion for a while before giving up of doing it .. */
			poll_result = ibv_poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc);
			ASSERT_TRUE(poll_result >= 0);

			s_poll_cq_count += poll_result;

			/* CQE found */
			for (i = 0; i < poll_result; ++i) {
				if (ctx->wc[i].status != IBV_WC_SUCCESS) {
					VERBS_TRACE("Failed status %s (%d) for wr_id %d\n",
						ibv_wc_status_str(ctx->wc[i].status),
						ctx->wc[i].status, (int) ctx->wc[i].wr_id);
					ASSERT_TRUE(0);
				}

				switch (TEST_GET_TYPE(ctx->wc[i].wr_id)) {
				case TEST_SEND_WRID:
					++scnt;
					break;

				case TEST_RECV_WRID:
					if (--routs <= 1) {
						routs += __post_read(ctx, ctx->qp_rx_depth - routs);
						if (routs < ctx->qp_rx_depth) {
							VERBS_TRACE("Couldn't post receive (%d)\n", routs);
							ASSERT_TRUE(0);
						}
					}

					++rcnt;
					break;

				default:
					VERBS_TRACE("Completion for unknown wr_id %d\n",
							(int) ctx->wc[i].wr_id);
					break;
				}
				VERBS_INFO("%-10s : wr_id=%-4" PRIu64 " scnt=%-4d rcnt=%-4d poll=%d\n",
						wr_id2str(ctx->wc[i].wr_id),
						TEST_GET_WRID(ctx->wc[i].wr_id),
						scnt, rcnt, poll_result);
			}

			poll_result = ibv_poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc);
			ASSERT_TRUE(poll_result >= 0);

			r_poll_cq_count += poll_result;

			/* CQE found */
			for (i = 0; i < poll_result; ++i) {
				if (ctx->wc[i].status != IBV_WC_SUCCESS) {
					VERBS_TRACE("Failed status %s (%d) for wr_id %d\n",
						ibv_wc_status_str(ctx->wc[i].status),
						ctx->wc[i].status, (int) ctx->wc[i].wr_id);
					ASSERT_TRUE(0);
				}

				switch (TEST_GET_TYPE(ctx->wc[i].wr_id)) {
				case TEST_SEND_WRID:
					++scnt;
					break;

				case TEST_RECV_WRID:
					if (--routs <= 1) {
						routs += __post_read(ctx, ctx->qp_rx_depth - routs);
						if (routs < ctx->qp_rx_depth) {
							VERBS_TRACE("Couldn't post receive (%d)\n", routs);
							ASSERT_TRUE(0);
						}
					}

					++rcnt;
					break;

				default:
					VERBS_TRACE("Completion for unknown wr_id %d\n",
							(int) ctx->wc[i].wr_id);
					break;
				}
				VERBS_INFO("%-10s : wr_id=%-4" PRIu64 " scnt=%-4d rcnt=%-4d poll=%d\n",
						wr_id2str(ctx->wc[i].wr_id),
						TEST_GET_WRID(ctx->wc[i].wr_id),
						scnt, rcnt, poll_result);
			}

			gettimeofday(&cur_time, NULL);
			cur_time_msec = (cur_time.tv_sec * 1000)
					+ (cur_time.tv_usec / 1000);
		} while ((wrid < SEND_POST_COUNT)
				|| ((cur_time_msec - start_time_msec)
						< MAX_POLL_CQ_TIMEOUT));

		EXPECT_EQ(SEND_POST_COUNT, wrid);
		EXPECT_EQ(SEND_POST_COUNT, s_poll_cq_count);
		EXPECT_EQ(SEND_POST_COUNT, r_poll_cq_count);
		EXPECT_EQ(scnt, rcnt);

		poll_result = ibv_poll_cq(ctx->mcq, 0x10, ctx->wc);
		ASSERT_TRUE(poll_result >= 0);
		m_poll_cq_count += poll_result;

		EXPECT_EQ(1, m_poll_cq_count);
		EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[0].status);
		EXPECT_EQ(SEND_EN_WR_ID, ctx->wc[0].wr_id);
	}
#endif //HAVE_CROSS_CHANNEL
}


/* tc_verbs_post_send_en: [TI.8]
 * IBV_QP_CREATE_MANAGED_SEND sent from MQP is able to push send requests
 * sequence SEND-...-SEND-SEND_EN(3)
 * Expected THREE CQE on rcq
 */
TEST_F(tc_verbs_post_send_en, ti_8) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL | IBV_QP_CREATE_MANAGED_SEND);

	/*
	 * Use the created QP for communication operations.
	 */
	{
		int i;
		int routs;
		int rcnt, scnt;
		int64_t	 wrid = 0;
		unsigned long start_time_msec;
		unsigned long cur_time_msec;
		struct timeval cur_time;
		int poll_result;
		int s_poll_cq_count = 0;
		int r_poll_cq_count = 0;
		int m_poll_cq_count = 0;

		routs = ctx->qp_rx_depth;

		rcnt = 0;
		scnt = 0;
		gettimeofday(&cur_time, NULL);
		start_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);
		do {
			if (wrid < SEND_POST_COUNT) {
				rc = __post_write(ctx, TEST_SET_WRID(TEST_SEND_WRID, wrid), IBV_WR_SEND);
				ASSERT_EQ(EOK, rc);

				if (wrid == (SEND_POST_COUNT - 1)) {
					struct ibv_send_wr wr_en;

					wr_en.wr_id = SEND_EN_WR_ID;
					wr_en.next = NULL;
					wr_en.sg_list = NULL;
					wr_en.num_sge = 0;
					wr_en.opcode = IBV_WR_SEND_ENABLE;
					wr_en.send_flags =  IBV_SEND_WAIT_EN_LAST;
					wr_en.imm_data = 0;

					wr_en.task.wqe_enable.qp   = ctx->qp;
					wr_en.task.wqe_enable.wqe_count = 3;

					rc = ibv_post_send(ctx->mqp, &wr_en, NULL);
					ASSERT_EQ(EOK, rc);
				}
				++wrid;
			}

			/* poll the completion for a while before giving up of doing it .. */
			poll_result = ibv_poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc);
			ASSERT_TRUE(poll_result >= 0);

			s_poll_cq_count += poll_result;

			/* CQE found */
			for (i = 0; i < poll_result; ++i) {
				if (ctx->wc[i].status != IBV_WC_SUCCESS) {
					VERBS_TRACE("Failed status %s (%d) for wr_id %d\n",
						ibv_wc_status_str(ctx->wc[i].status),
						ctx->wc[i].status, (int) ctx->wc[i].wr_id);
					ASSERT_TRUE(0);
				}

				switch (TEST_GET_TYPE(ctx->wc[i].wr_id)) {
				case TEST_SEND_WRID:
					++scnt;
					break;

				case TEST_RECV_WRID:
					if (--routs <= 1) {
						routs += __post_read(ctx, ctx->qp_rx_depth - routs);
						if (routs < ctx->qp_rx_depth) {
							VERBS_TRACE("Couldn't post receive (%d)\n", routs);
							ASSERT_TRUE(0);
						}
					}

					EXPECT_LT(3, wrid);
					++rcnt;
					break;

				default:
					VERBS_TRACE("Completion for unknown wr_id %d\n",
							(int) ctx->wc[i].wr_id);
					break;
				}
				VERBS_INFO("%-10s : wr_id=%-4" PRIu64 " scnt=%-4d rcnt=%-4d poll=%d\n",
						wr_id2str(ctx->wc[i].wr_id),
						TEST_GET_WRID(ctx->wc[i].wr_id),
						scnt, rcnt, poll_result);
			}

			poll_result = ibv_poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc);
			ASSERT_TRUE(poll_result >= 0);

			r_poll_cq_count += poll_result;

			/* CQE found */
			for (i = 0; i < poll_result; ++i) {
				if (ctx->wc[i].status != IBV_WC_SUCCESS) {
					VERBS_TRACE("Failed status %s (%d) for wr_id %d\n",
						ibv_wc_status_str(ctx->wc[i].status),
						ctx->wc[i].status, (int) ctx->wc[i].wr_id);
					ASSERT_TRUE(0);
				}

				switch (TEST_GET_TYPE(ctx->wc[i].wr_id)) {
				case TEST_SEND_WRID:
					++scnt;
					break;

				case TEST_RECV_WRID:
					if (--routs <= 1) {
						routs += __post_read(ctx, ctx->qp_rx_depth - routs);
						if (routs < ctx->qp_rx_depth) {
							VERBS_TRACE("Couldn't post receive (%d)\n", routs);
							ASSERT_TRUE(0);
						}
					}

					EXPECT_LT(3, wrid);
					++rcnt;
					break;

				default:
					VERBS_TRACE("Completion for unknown wr_id %d\n",
							(int) ctx->wc[i].wr_id);
					break;
				}
				VERBS_INFO("%-10s : wr_id=%-4" PRIu64 " scnt=%-4d rcnt=%-4d poll=%d\n",
						wr_id2str(ctx->wc[i].wr_id),
						TEST_GET_WRID(ctx->wc[i].wr_id),
						scnt, rcnt, poll_result);
			}

			gettimeofday(&cur_time, NULL);
			cur_time_msec = (cur_time.tv_sec * 1000)
					+ (cur_time.tv_usec / 1000);
		} while ((wrid < SEND_POST_COUNT)
				|| ((cur_time_msec - start_time_msec)
						< MAX_POLL_CQ_TIMEOUT));

		EXPECT_EQ(SEND_POST_COUNT, wrid);
		EXPECT_EQ(3, s_poll_cq_count);
		EXPECT_EQ(3, r_poll_cq_count);
		EXPECT_EQ(scnt, rcnt);

		poll_result = ibv_poll_cq(ctx->mcq, 0x10, ctx->wc);
		ASSERT_TRUE(poll_result >= 0);
		m_poll_cq_count += poll_result;

		EXPECT_EQ(0, m_poll_cq_count);
	}
#endif //HAVE_CROSS_CHANNEL
}


/* tc_verbs_post_send_en: [TI.9]
 * IBV_QP_CREATE_MANAGED_SEND sent from MQP is able to push send requests
 * Every SEND_EN sets  IBV_SEND_WAIT_EN_LAST
 * sequence SEND-...-SEND-SEND_EN^(2)-SEND_EN^(2)-SEND_EN^(2)
 * Expected SIX CQE on rcq
 * Note: internal index is relative previous post and incremented after special WAIT posted
 */
TEST_F(tc_verbs_post_send_en, ti_9) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL | IBV_QP_CREATE_MANAGED_SEND);

	/*
	 * Use the created QP for communication operations.
	 */
	{
		int i;
		int routs;
		int rcnt, scnt;
		int64_t	 wrid = 0;
		unsigned long start_time_msec;
		unsigned long cur_time_msec;
		struct timeval cur_time;
		int poll_result;
		int s_poll_cq_count = 0;
		int r_poll_cq_count = 0;
		int m_poll_cq_count = 0;

		routs = ctx->qp_rx_depth;

		rcnt = 0;
		scnt = 0;
		gettimeofday(&cur_time, NULL);
		start_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);
		do {
			if (wrid < SEND_POST_COUNT) {
				rc = __post_write(ctx, TEST_SET_WRID(TEST_SEND_WRID, wrid), IBV_WR_SEND);
				ASSERT_EQ(EOK, rc);

				if (wrid == (SEND_POST_COUNT - 1)) {
					struct ibv_send_wr wr_en[3];

					wr_en[0].wr_id = SEND_EN_WR_ID + 1;
					wr_en[0].next = &wr_en[1];
					wr_en[0].sg_list = NULL;
					wr_en[0].num_sge = 0;
					wr_en[0].opcode = IBV_WR_SEND_ENABLE;
					wr_en[0].send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
					wr_en[0].imm_data = 0;

					wr_en[1].wr_id = SEND_EN_WR_ID + 2;
					wr_en[1].next = &wr_en[2];
					wr_en[1].sg_list = NULL;
					wr_en[1].num_sge = 0;
					wr_en[1].opcode = IBV_WR_SEND_ENABLE;
					wr_en[1].send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
					wr_en[1].imm_data = 0;

					wr_en[2].wr_id = SEND_EN_WR_ID + 3;
					wr_en[2].next = NULL;
					wr_en[2].sg_list = NULL;
					wr_en[2].num_sge = 0;
					wr_en[2].opcode = IBV_WR_SEND_ENABLE;
					wr_en[2].send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
					wr_en[2].imm_data = 0;

					wr_en[0].task.wqe_enable.qp   = ctx->qp;
					wr_en[0].task.wqe_enable.wqe_count = 2;
					wr_en[1].task.wqe_enable.qp   = ctx->qp;
					wr_en[1].task.wqe_enable.wqe_count = 2;
					wr_en[2].task.wqe_enable.qp   = ctx->qp;
					wr_en[2].task.wqe_enable.wqe_count = 2;

					rc = ibv_post_send(ctx->mqp, wr_en, NULL);
					ASSERT_EQ(EOK, rc);
				}
				++wrid;
			}

			/* poll the completion for a while before giving up of doing it .. */
			poll_result = ibv_poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc);
			ASSERT_TRUE(poll_result >= 0);

			s_poll_cq_count += poll_result;

			/* CQE found */
			for (i = 0; i < poll_result; ++i) {
				if (ctx->wc[i].status != IBV_WC_SUCCESS) {
					VERBS_TRACE("Failed status %s (%d) for wr_id %d\n",
						ibv_wc_status_str(ctx->wc[i].status),
						ctx->wc[i].status, (int) ctx->wc[i].wr_id);
					ASSERT_TRUE(0);
				}

				switch (TEST_GET_TYPE(ctx->wc[i].wr_id)) {
				case TEST_SEND_WRID:
					++scnt;
					break;

				case TEST_RECV_WRID:
					if (--routs <= 1) {
						routs += __post_read(ctx, ctx->qp_rx_depth - routs);
						if (routs < ctx->qp_rx_depth) {
							VERBS_TRACE("Couldn't post receive (%d)\n", routs);
							ASSERT_TRUE(0);
						}
					}

					EXPECT_LT(3, wrid);
					++rcnt;
					break;

				default:
					VERBS_TRACE("Completion for unknown wr_id %d\n",
							(int) ctx->wc[i].wr_id);
					break;
				}
				VERBS_INFO("%-10s : wr_id=%-4" PRIu64 " scnt=%-4d rcnt=%-4d poll=%d\n",
						wr_id2str(ctx->wc[i].wr_id),
						TEST_GET_WRID(ctx->wc[i].wr_id),
						scnt, rcnt, poll_result);
			}

			poll_result = ibv_poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc);
			ASSERT_TRUE(poll_result >= 0);

			r_poll_cq_count += poll_result;

			/* CQE found */
			for (i = 0; i < poll_result; ++i) {
				if (ctx->wc[i].status != IBV_WC_SUCCESS) {
					VERBS_TRACE("Failed status %s (%d) for wr_id %d\n",
						ibv_wc_status_str(ctx->wc[i].status),
						ctx->wc[i].status, (int) ctx->wc[i].wr_id);
					ASSERT_TRUE(0);
				}

				switch (TEST_GET_TYPE(ctx->wc[i].wr_id)) {
				case TEST_SEND_WRID:
					++scnt;
					break;

				case TEST_RECV_WRID:
					if (--routs <= 1) {
						routs += __post_read(ctx, ctx->qp_rx_depth - routs);
						if (routs < ctx->qp_rx_depth) {
							VERBS_TRACE("Couldn't post receive (%d)\n", routs);
							ASSERT_TRUE(0);
						}
					}

					EXPECT_LT(3, wrid);
					++rcnt;
					break;

				default:
					VERBS_TRACE("Completion for unknown wr_id %d\n",
							(int) ctx->wc[i].wr_id);
					break;
				}
				VERBS_INFO("%-10s : wr_id=%-4" PRIu64 " scnt=%-4d rcnt=%-4d poll=%d\n",
						wr_id2str(ctx->wc[i].wr_id),
						TEST_GET_WRID(ctx->wc[i].wr_id),
						scnt, rcnt, poll_result);
			}

			gettimeofday(&cur_time, NULL);
			cur_time_msec = (cur_time.tv_sec * 1000)
					+ (cur_time.tv_usec / 1000);
		} while ((wrid < SEND_POST_COUNT)
				|| ((cur_time_msec - start_time_msec)
						< MAX_POLL_CQ_TIMEOUT));

		EXPECT_EQ(SEND_POST_COUNT, wrid);
		EXPECT_EQ(6, s_poll_cq_count);
		EXPECT_EQ(6, r_poll_cq_count);
		EXPECT_EQ(scnt, rcnt);

		poll_result = ibv_poll_cq(ctx->mcq, 0x10, ctx->wc);
		ASSERT_TRUE(poll_result >= 0);
		m_poll_cq_count += poll_result;
		EXPECT_EQ(3, m_poll_cq_count);
		EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[0].status);
		EXPECT_EQ(SEND_EN_WR_ID + 1, ctx->wc[0].wr_id);
		EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[0].status);
		EXPECT_EQ(SEND_EN_WR_ID + 2, ctx->wc[1].wr_id);
		EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[0].status);
		EXPECT_EQ(SEND_EN_WR_ID + 3, ctx->wc[2].wr_id);
	}
#endif //HAVE_CROSS_CHANNEL
}


/* tc_verbs_post_send_en: [TI.10]
 * IBV_QP_CREATE_MANAGED_SEND sent from MQP is able to push send requests
 * sequence SEND-...-SEND-SEND_EN(2)-SEND_EN(2)-SEND_EN^(2)
 * Only last SEND_EN sets  IBV_SEND_WAIT_EN_LAST
 * Expected TWO CQE on rcq
 * Note: internal index is relative previous post and incremented after special WAIT posted
 */
TEST_F(tc_verbs_post_send_en, ti_10) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL | IBV_QP_CREATE_MANAGED_SEND);

	/*
	 * Use the created QP for communication operations.
	 */
	{
		int i;
		int routs;
		int rcnt, scnt;
		int64_t	 wrid = 0;
		unsigned long start_time_msec;
		unsigned long cur_time_msec;
		struct timeval cur_time;
		int poll_result;
		int s_poll_cq_count = 0;
		int r_poll_cq_count = 0;
		int m_poll_cq_count = 0;

		routs = ctx->qp_rx_depth;

		rcnt = 0;
		scnt = 0;
		gettimeofday(&cur_time, NULL);
		start_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);
		do {
			if (wrid < SEND_POST_COUNT) {
				rc = __post_write(ctx, TEST_SET_WRID(TEST_SEND_WRID, wrid), IBV_WR_SEND);
				ASSERT_EQ(EOK, rc);

				if (wrid == (SEND_POST_COUNT - 1)) {
					struct ibv_send_wr wr_en[3];

					wr_en[0].wr_id = SEND_EN_WR_ID + 1;
					wr_en[0].next = &wr_en[1];
					wr_en[0].sg_list = NULL;
					wr_en[0].num_sge = 0;
					wr_en[0].opcode = IBV_WR_SEND_ENABLE;
					wr_en[0].send_flags = IBV_SEND_SIGNALED;
					wr_en[0].imm_data = 0;

					wr_en[1].wr_id = SEND_EN_WR_ID + 2;
					wr_en[1].next = &wr_en[2];
					wr_en[1].sg_list = NULL;
					wr_en[1].num_sge = 0;
					wr_en[1].opcode = IBV_WR_SEND_ENABLE;
					wr_en[1].send_flags = IBV_SEND_SIGNALED;
					wr_en[1].imm_data = 0;

					wr_en[2].wr_id = SEND_EN_WR_ID + 3;
					wr_en[2].next = NULL;
					wr_en[2].sg_list = NULL;
					wr_en[2].num_sge = 0;
					wr_en[2].opcode = IBV_WR_SEND_ENABLE;
					wr_en[2].send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
					wr_en[2].imm_data = 0;


					wr_en[0].task.wqe_enable.qp   = ctx->qp;
					wr_en[0].task.wqe_enable.wqe_count = 2;
					wr_en[1].task.wqe_enable.qp   = ctx->qp;
					wr_en[1].task.wqe_enable.wqe_count = 2;
					wr_en[2].task.wqe_enable.qp   = ctx->qp;
					wr_en[2].task.wqe_enable.wqe_count = 2;

					rc = ibv_post_send(ctx->mqp, wr_en, NULL);
					ASSERT_EQ(EOK, rc);
				}
				++wrid;
			}

			/* poll the completion for a while before giving up of doing it .. */
			poll_result = ibv_poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc);
			ASSERT_TRUE(poll_result >= 0);

			s_poll_cq_count += poll_result;

			/* CQE found */
			for (i = 0; i < poll_result; ++i) {
				if (ctx->wc[i].status != IBV_WC_SUCCESS) {
					VERBS_TRACE("Failed status %s (%d) for wr_id %d\n",
						ibv_wc_status_str(ctx->wc[i].status),
						ctx->wc[i].status, (int) ctx->wc[i].wr_id);
					ASSERT_TRUE(0);
				}

				switch (TEST_GET_TYPE(ctx->wc[i].wr_id)) {
				case TEST_SEND_WRID:
					++scnt;
					break;

				case TEST_RECV_WRID:
					if (--routs <= 1) {
						routs += __post_read(ctx, ctx->qp_rx_depth - routs);
						if (routs < ctx->qp_rx_depth) {
							VERBS_TRACE("Couldn't post receive (%d)\n", routs);
							ASSERT_TRUE(0);
						}
					}

					EXPECT_LT(3, wrid);
					++rcnt;
					break;

				default:
					VERBS_TRACE("Completion for unknown wr_id %d\n",
							(int) ctx->wc[i].wr_id);
					break;
				}
				VERBS_INFO("%-10s : wr_id=%-4" PRIu64 " scnt=%-4d rcnt=%-4d poll=%d\n",
						wr_id2str(ctx->wc[i].wr_id),
						TEST_GET_WRID(ctx->wc[i].wr_id),
						scnt, rcnt, poll_result);
			}

			poll_result = ibv_poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc);
			ASSERT_TRUE(poll_result >= 0);

			r_poll_cq_count += poll_result;

			/* CQE found */
			for (i = 0; i < poll_result; ++i) {
				if (ctx->wc[i].status != IBV_WC_SUCCESS) {
					VERBS_TRACE("Failed status %s (%d) for wr_id %d\n",
						ibv_wc_status_str(ctx->wc[i].status),
						ctx->wc[i].status, (int) ctx->wc[i].wr_id);
					ASSERT_TRUE(0);
				}

				switch (TEST_GET_TYPE(ctx->wc[i].wr_id)) {
				case TEST_SEND_WRID:
					++scnt;
					break;

				case TEST_RECV_WRID:
					if (--routs <= 1) {
						routs += __post_read(ctx, ctx->qp_rx_depth - routs);
						if (routs < ctx->qp_rx_depth) {
							VERBS_TRACE("Couldn't post receive (%d)\n", routs);
							ASSERT_TRUE(0);
						}
					}

					EXPECT_LT(3, wrid);
					++rcnt;
					break;

				default:
					VERBS_TRACE("Completion for unknown wr_id %d\n",
							(int) ctx->wc[i].wr_id);
					break;
				}
				VERBS_INFO("%-10s : wr_id=%-4" PRIu64 " scnt=%-4d rcnt=%-4d poll=%d\n",
						wr_id2str(ctx->wc[i].wr_id),
						TEST_GET_WRID(ctx->wc[i].wr_id),
						scnt, rcnt, poll_result);
			}

			gettimeofday(&cur_time, NULL);
			cur_time_msec = (cur_time.tv_sec * 1000)
					+ (cur_time.tv_usec / 1000);
		} while ((wrid < SEND_POST_COUNT)
				|| ((cur_time_msec - start_time_msec)
						< MAX_POLL_CQ_TIMEOUT));

		EXPECT_EQ(SEND_POST_COUNT, wrid);
		EXPECT_EQ(2, s_poll_cq_count);
		EXPECT_EQ(2, r_poll_cq_count);
		EXPECT_EQ(scnt, rcnt);

		poll_result = ibv_poll_cq(ctx->mcq, 0x10, ctx->wc);
		ASSERT_TRUE(poll_result >= 0);
		m_poll_cq_count += poll_result;
		EXPECT_EQ(3, m_poll_cq_count);
		EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[0].status);
		EXPECT_EQ(SEND_EN_WR_ID + 1, ctx->wc[0].wr_id);
		EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[0].status);
		EXPECT_EQ(SEND_EN_WR_ID + 2, ctx->wc[1].wr_id);
		EXPECT_EQ(IBV_WC_SUCCESS, ctx->wc[0].status);
		EXPECT_EQ(SEND_EN_WR_ID + 3, ctx->wc[2].wr_id);
	}
#endif //HAVE_CROSS_CHANNEL
}
