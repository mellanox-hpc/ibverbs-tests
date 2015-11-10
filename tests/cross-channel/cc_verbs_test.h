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

/**
 * Base class for ibverbs test fixtures.
 * Initialize and close ibverbs.
 */
class cc_verbs_test : public verbs_test {
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
#endif //_IBVERBS_CC_VERBS_TEST_
