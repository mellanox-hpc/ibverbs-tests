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

class tc_verbs_query_device : public cc_base_verbs_test {};

/* verbs_query_device: [TI.1]
 * Check if ibv_query_device() returns information about Cross-Channel support
 */
TEST_F(tc_verbs_query_device, ti_1) {
	CHECK_TEST_OR_SKIP(Cross-Channel);

	VERBS_INFO("============================================\n");
	VERBS_INFO("DEVICE     : %s\n", ibv_get_device_name(ibv_dev));
	VERBS_INFO("           : %s\n", ibv_dev->dev_name);
	VERBS_INFO("           : %s\n", ibv_dev->dev_path);
	VERBS_INFO("           : %s\n", ibv_dev->ibdev_path);
	VERBS_INFO("============================================\n");

#ifdef HAVE_CROSS_CHANNEL
	VERBS_INFO("device_attr.device_cap_flags: 0x%x\n",
			device_attr.orig_attr.device_cap_flags);
	VERBS_INFO("CROSS_CHANNEL               : %s \n",
			(device_attr.orig_attr.device_cap_flags & IBV_DEVICE_CROSS_CHANNEL ?
					"ON" : "OFF"));
	ASSERT_TRUE(device_attr.orig_attr.device_cap_flags & IBV_DEVICE_CROSS_CHANNEL);
#endif //HAVE_CROSS_CHANNEL
}
