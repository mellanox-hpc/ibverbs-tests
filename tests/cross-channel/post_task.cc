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

class tc_verbs_post_task : public cc_init_verbs_test {};

#define SEND_POST_COUNT		10

/* verbs_post_task: [TI.1] Correct */
TEST_F(tc_verbs_post_task, ti_1) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	__init_test(IBV_QP_CREATE_CROSS_CHANNEL | IBV_QP_CREATE_MANAGED_SEND);

	/*
	 * Use the created QP for communication operations.
	 */
	{
		struct ibv_task task[2];
		struct ibv_task *task_bad;
		struct ibv_send_wr wr_tsk0[SEND_POST_COUNT];
		struct ibv_send_wr wr_tsk1[4];
		int i = 0;

		EXPECT_TRUE(SEND_POST_COUNT >= 10);

		/* create TASK(SEND) for qp */
		memset(task, 0, sizeof(*task) * 2);
		memset(wr_tsk0, 0, sizeof(*wr_tsk0) * SEND_POST_COUNT);
		for (i = 0; i < SEND_POST_COUNT; i++) {
			wr_tsk0[i].wr_id = i;
			wr_tsk0[i].next = ( i < (SEND_POST_COUNT - 1) ? &wr_tsk0[i + 1] : NULL);
			wr_tsk0[i].sg_list = NULL;
			wr_tsk0[i].num_sge = 0;
			wr_tsk0[i].opcode = IBV_WR_SEND;
			wr_tsk0[i].send_flags = IBV_SEND_SIGNALED;
			wr_tsk0[i].imm_data = 0;
		}

		task[0].task_type = IBV_TASK_SEND;
		task[0].item.qp = ctx->qp;
		task[0].item.send_wr = wr_tsk0;
		task[0].next = &task[1];

		/* create TASK(WAIT) for mqp */
		memset(wr_tsk1, 0, sizeof(*wr_tsk1) * 4);

		/* SEND_EN(QP,1) */
		wr_tsk1[0].wr_id = 0;
		wr_tsk1[0].next = &wr_tsk1[1];
		wr_tsk1[0].sg_list = NULL;
		wr_tsk1[0].num_sge = 0;
		wr_tsk1[0].opcode = IBV_WR_SEND_ENABLE;
		wr_tsk1[0].send_flags = IBV_SEND_SIGNALED;
		wr_tsk1[0].imm_data = 0;

		wr_tsk1[0].task.wqe_enable.qp   = ctx->qp;
		wr_tsk1[0].task.wqe_enable.wqe_count = 1;

		/* WAIT(QP,1) */
		wr_tsk1[1].wr_id = 1;
		wr_tsk1[1].next = &wr_tsk1[2];
		wr_tsk1[1].sg_list = NULL;
		wr_tsk1[1].num_sge = 0;
		wr_tsk1[1].opcode = IBV_WR_CQE_WAIT;
		wr_tsk1[1].send_flags = IBV_SEND_SIGNALED;
		wr_tsk1[1].imm_data = 0;

		wr_tsk1[1].task.cqe_wait.cq = ctx->mcq;
		wr_tsk1[1].task.cqe_wait.cq_count = 1;

		/* SEND_EN(QP,ALL) */
		wr_tsk1[2].wr_id = 2;
		wr_tsk1[2].next = &wr_tsk1[3];
		wr_tsk1[2].sg_list = NULL;
		wr_tsk1[2].num_sge = 0;
		wr_tsk1[2].opcode = IBV_WR_SEND_ENABLE;
		wr_tsk1[2].send_flags = 0;
		wr_tsk1[2].imm_data = 0;

		wr_tsk1[2].task.wqe_enable.qp   = ctx->qp;
		wr_tsk1[2].task.wqe_enable.wqe_count = 0;

		/* WAIT(QP,SEND_POST_COUNT) */
		wr_tsk1[3].wr_id = 3;
		wr_tsk1[3].next = NULL;
		wr_tsk1[3].sg_list = NULL;
		wr_tsk1[3].num_sge = 0;
		wr_tsk1[3].opcode = IBV_WR_CQE_WAIT;
		wr_tsk1[3].send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
		wr_tsk1[3].imm_data = 0;

		wr_tsk1[3].task.cqe_wait.cq = ctx->rcq;
		wr_tsk1[3].task.cqe_wait.cq_count = SEND_POST_COUNT;

		task[1].task_type = IBV_TASK_SEND;
		task[1].item.qp = ctx->mqp;
		task[1].item.send_wr = wr_tsk1;
		task[1].next = NULL;

		rc = ibv_post_task(ibv_ctx, task, &task_bad);
		ASSERT_EQ(EOK, rc);

		__poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc, SEND_POST_COUNT);
		__poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc, SEND_POST_COUNT);
		__poll_cq(ctx->mcq, 0x10, ctx->wc, 3);

		EXPECT_EQ((uint64_t)0, ctx->wc[0].wr_id);
		EXPECT_EQ((uint64_t)1, ctx->wc[1].wr_id);
		EXPECT_EQ((uint64_t)3, ctx->wc[2].wr_id);
	}
#endif //HAVE_CROSS_CHANNEL
}
/* verbs_post_task: [TI.2] Bad case
 * Expected bad_task = task[1]
 */
TEST_F(tc_verbs_post_task, ti_2) {
	CHECK_TEST_OR_SKIP(Cross-Channel);
#ifdef HAVE_CROSS_CHANNEL
	int rc = EOK;

	__init_test();

	/*
	 * Use the created QP for communication operations.
	 */
	{
		struct ibv_task task[2];
		struct ibv_task *task_bad;

		struct ibv_send_wr wr_tsk0[SEND_POST_COUNT];
		struct ibv_send_wr wr_tsk1;
		int i = 0;

		EXPECT_TRUE(SEND_POST_COUNT >= 10);

		/* create TASK(SEND) for qp */
		memset(task, 0, sizeof(*task) * 2);

		memset(wr_tsk0, 0, sizeof(*wr_tsk0) * SEND_POST_COUNT);
		for (i = 0; i < SEND_POST_COUNT; i++) {
			wr_tsk0[i].wr_id = i;
			wr_tsk0[i].next = ( i < (SEND_POST_COUNT - 1) ? &wr_tsk0[i + 1] : NULL);
			wr_tsk0[i].sg_list = NULL;
			wr_tsk0[i].num_sge = 0;
			wr_tsk0[i].opcode = IBV_WR_SEND;
			wr_tsk0[i].send_flags = IBV_SEND_SIGNALED;
			wr_tsk0[i].imm_data = 0;
		}

		task[0].task_type = IBV_TASK_SEND;
		task[0].item.qp = ctx->qp;
		task[0].item.send_wr = wr_tsk0;

		task[0].next = &task[1];

		/* create TASK(WAIT) for mqp */
		memset(&wr_tsk1, 0, sizeof(wr_tsk1));

		/* SEND_EN(QP,1) */
		wr_tsk1.wr_id = 0;
		wr_tsk1.next = NULL;
		wr_tsk1.sg_list = NULL;
		wr_tsk1.num_sge = 0;
		wr_tsk1.opcode = IBV_WR_SEND_ENABLE;
		wr_tsk1.send_flags = IBV_SEND_SIGNALED | IBV_SEND_WAIT_EN_LAST;
		wr_tsk1.imm_data = 0;

		wr_tsk1.task.wqe_enable.qp   = ctx->qp;
		wr_tsk1.task.wqe_enable.wqe_count = 1;

		task[1].task_type = IBV_TASK_SEND;
		task[1].item.qp = ctx->qp;
		task[1].item.send_wr = &wr_tsk1;

		task[1].next = NULL;

		rc = ibv_post_task(ibv_ctx, task, &task_bad);
		ASSERT_NE(EOK, rc);
		ASSERT_EQ(task_bad, &task[1]);

		__poll_cq(ctx->scq, ctx->cq_tx_depth, ctx->wc, SEND_POST_COUNT);
		__poll_cq(ctx->rcq, ctx->cq_rx_depth, ctx->wc, SEND_POST_COUNT);
	}
#endif //HAVE_CROSS_CHANNEL
}
