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
#ifndef _CHECKS_H
#define _CHECKS_H

#include "cc_verbs_test.h"

#ifdef HAVE_CROSS_CHANNEL
class tc_verbs_query_device : public cc_base_verbs_test {};
class tc_verbs_create_cq : public cc_init_verbs_test {};
class tc_verbs_post_send_en : public cc_init_verbs_test {};
class tc_verbs_post_recv_en : public cc_init_verbs_test {};
class tc_verbs_post_send_wait : public cc_init_verbs_test {};
#else
class cc_dummy_class : public testing::Test {
protected:
	virtual void SetUp() {
		printf("[  SKIPPED ] Feature cross-channel is not supported\n");
		::testing::UnitTest::GetInstance()->runtime_skip(1);
	}
};
class tc_verbs_query_device : public cc_dummy_class {};
class tc_verbs_create_cq : public cc_dummy_class {};
class tc_verbs_post_send_en : public cc_dummy_class {};
class tc_verbs_post_recv_en : public cc_dummy_class {};
class tc_verbs_post_send_wait : public cc_dummy_class {};
#endif

#ifdef HAVE_CROSS_CHANNEL_CALC
/* do nothing */
#else
class cc_calc_dummy_class : public testing::Test {
protected:
	virtual void SetUp() {
		printf("[  SKIPPED ] Feature cross-channel CALC is not supported\n");
		::testing::UnitTest::GetInstance()->runtime_skip(1);
	}
};
#endif

#ifdef HAVE_CROSS_CHANNEL_TASK
class tc_verbs_post_task : public cc_init_verbs_test {};
#else
class cc_task_dummy_class : public testing::Test {
protected:
	virtual void SetUp() {
		printf("[  SKIPPED ] Feature cross-channel tasks is not supported\n");
		::testing::UnitTest::GetInstance()->runtime_skip(1);
	}
};
class tc_verbs_post_task : public cc_task_dummy_class {};
#endif

#endif //_CHECKS_H
