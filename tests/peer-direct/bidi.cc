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

#define LEFT 1
#define RIGHT 0

typedef testing::Types<
	types<ibverbs_env_rc, nothing>,
	types<ibverbs_env_rc, ibverbs_event>,
	types<ibverbs_env_uc, nothing>,
	types<ibverbs_env_uc, ibverbs_event>,
	types<ibverbs_env_ud, nothing>,
	types<ibverbs_env_ud, ibverbs_event>
> ibverbs_env_list;

template <typename T>
class bidi_test : public base_test<T> { };

TYPED_TEST_CASE(bidi_test, ibverbs_env_list);

TYPED_TEST(bidi_test, t0) {
	VERBS_INFO("connect lid %x-%x qp %x-%x\n",
		this->port_attr[LEFT].lid,
		this->port_attr[RIGHT].lid,
		this->queue_pair[LEFT]->qp_num,
		this->queue_pair[RIGHT]->qp_num);

	EXEC(this->setup_buff(LEFT, RIGHT));
	EXEC(this->recv(RIGHT, 0, SZ));
	EXEC(this->xmit(LEFT, 0, SZ));
	EXEC(this->poll(LEFT, 1));
	EXEC(this->check_fin(RIGHT, 1));

	EXEC(this->setup_buff(RIGHT, LEFT));
	EXEC(this->recv(LEFT, 0, SZ));
	EXEC(this->xmit(RIGHT, 0, SZ));
	EXEC(this->poll(RIGHT, 1));
	EXEC(this->check_fin(LEFT, 1));
}

