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

#include "env.h"

typedef testing::Types<
	types<ibverbs_env_rc, nothing>,
	types<ibverbs_env_rc, ibverbs_event>,
	types<ibverbs_env_ud, nothing>,
	types<ibverbs_env_ud, ibverbs_event>,
	types<ibverbs_env_uc, nothing>,
	types<ibverbs_env_uc, ibverbs_event>
> ibverbs_env_list;

TYPED_TEST_CASE(base_test, ibverbs_env_list);

TYPED_TEST(base_test, t0_no_pd) {
	CHK_NODE;
	EXEC(this->setup_buff(SENDER, RECEIVER));
	EXEC(this->recv(RECEIVER, 0, SZ/2));
	EXEC(this->xmit(SENDER, 0, SZ/2));
	EXEC(this->poll(SENDER, 1));

	EXEC(this->recv(RECEIVER, SZ/2, SZ/2));
	EXEC(this->xmit(SENDER, SZ/2, SZ/2));
	EXEC(this->poll(SENDER, 1));

	EXEC(this->check_fin(RECEIVER, 2));
}

TYPED_TEST_CASE(peerdirect_test, ibverbs_env_list);

TYPED_TEST(peerdirect_test, t1_1_send) {
	CHK_NODE;
	EXEC(this->setup_buff(SENDER, RECEIVER));
	EXEC(this->recv(RECEIVER, 0, SZ));
	EXEC(this->xmit(SENDER, 0, SZ));
	EXEC(this->xmit_peer(SENDER));
	EXEC(this->check_fin(RECEIVER, 1));
}

TYPED_TEST(peerdirect_test, t2_2_sends) {
	CHK_NODE;
	xmit_peer_ctx ctx;
	memset(&ctx, 0, sizeof(ctx));
	EXEC(this->setup_buff(SENDER, RECEIVER));
	EXEC(this->recv(RECEIVER, 0, SZ/2));
	EXEC(this->recv(RECEIVER, SZ/2, SZ/2));
	EXEC(this->xmit(SENDER, 0, SZ/2));
	EXEC(this->xmit(SENDER, SZ/2, SZ/2));
	EXEC(this->peer_prep(SENDER, &ctx, 2));
	EXEC(this->peer_exec(SENDER, &ctx));
	EXEC(this->peer_poll(SENDER, &ctx));
	EXEC(this->check_fin(RECEIVER, 2));
}

TYPED_TEST(peerdirect_test, t3_2_sends_2_pd) {
	CHK_NODE;
	xmit_peer_ctx ctx[2];
	EXEC(this->xmit_peer_prep(SENDER, RECEIVER, 2, ctx));

	EXEC(this->peer_exec(SENDER, &ctx[0]));
	EXEC(this->peer_poll(SENDER, &ctx[0]));
	EXEC(this->peer_exec(SENDER, &ctx[1]));
	EXEC(this->peer_poll(SENDER, &ctx[1]));
	EXEC(this->check_fin(RECEIVER, 2));
}

TYPED_TEST(peerdirect_test, t4_rollback) {
	CHK_NODE;
	xmit_peer_ctx ctx[2];
	EXEC(this->xmit_peer_prep(SENDER, RECEIVER, 2, ctx));

	EXEC(this->peer_exec(SENDER, &ctx[0]));
	EXEC(this->peer_poll(SENDER, &ctx[0]));
	EXEC(this->peer_fail(SENDER, 0, &ctx[1]));

	EXEC(this->xmit(SENDER, SZ/2, SZ/2));
	EXEC(this->xmit_peer(SENDER));
	EXEC(this->check_fin(RECEIVER, 2));
}

TYPED_TEST(peerdirect_test, t5_poll_abort) {
	CHK_NODE;
	xmit_peer_ctx ctx[4];
	EXEC(this->xmit_peer_prep(SENDER, RECEIVER, 4, ctx));

	EXEC(this->peer_exec(SENDER, &ctx[0]));
	EXEC(this->peer_poll(SENDER, &ctx[0]));
	EXEC(this->peer_exec(SENDER, &ctx[1]));
	EXEC(this->peer_abort(SENDER, &ctx[1]));
	EXEC(this->peer_exec(SENDER, &ctx[2]));
	EXEC(this->peer_poll(SENDER, &ctx[2]));
	EXEC(this->peer_exec(SENDER, &ctx[3]));
	EXEC(this->peer_abort(SENDER, &ctx[3]));

	EXEC(this->check_fin(RECEIVER, 4));
}

TYPED_TEST(peerdirect_test, t6_pool_abort_16a) {
	CHK_NODE;
	xmit_peer_ctx ctx[16];
	EXEC(this->xmit_peer_prep(SENDER, RECEIVER, 16, ctx));

	for (int i = 0; i < 16; i+=2) {
		EXEC(this->peer_exec(SENDER, &ctx[i]));
		EXEC(this->peer_poll(SENDER, &ctx[i]));
		EXEC(this->peer_exec(SENDER, &ctx[i+1]));
		EXEC(this->peer_abort(SENDER, &ctx[i+1]));
	}

	EXEC(this->check_fin(RECEIVER, 16));
}
TYPED_TEST(peerdirect_test, t7_pool_abort_16b) {
	CHK_NODE;
	xmit_peer_ctx ctx[16];
	EXEC(this->xmit_peer_prep(SENDER, RECEIVER, 16, ctx));

	for (int i = 0; i < 16; i+=2) {
		EXEC(this->peer_exec(SENDER, &ctx[i]));
		EXEC(this->peer_abort(SENDER, &ctx[i]));
		EXEC(this->peer_exec(SENDER, &ctx[i+1]));
		EXEC(this->peer_poll(SENDER, &ctx[i+1]));
	}

	EXEC(this->check_fin(RECEIVER, 16));
}

TYPED_TEST(peerdirect_test, t8_pool16) {
	CHK_NODE;
	xmit_peer_ctx ctx[16];
	EXEC(this->xmit_peer_prep(SENDER, RECEIVER, 16, ctx));

	for (int i = 0; i < 16; i++) {
		EXEC(this->peer_exec(SENDER, &ctx[i]));
		EXEC(this->peer_poll(SENDER, &ctx[i]));
	}

	EXEC(this->check_fin(RECEIVER, 16));
}

TYPED_TEST(peerdirect_test, t9_abort16) {
	CHK_NODE;
	xmit_peer_ctx ctx[16];
	EXEC(this->xmit_peer_prep(SENDER, RECEIVER, 16, ctx));

	for (int i = 0; i < 16; i++) {
		EXEC(this->peer_exec(SENDER, &ctx[i]));
		EXEC(this->peer_abort(SENDER, &ctx[i]));
	}

	EXEC(this->check_fin(RECEIVER, 16));
}

