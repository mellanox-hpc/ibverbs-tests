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

#include "common.h"
#include "gtest.h"

#include "verbs_test.h"

#if (!HAVE_DECL_HTOBE64)
#include <infiniband/arch.h>
#define be64toh ntohll
#endif

/* ibv_get_device_list: [TI.1] Correct */
TEST(tc_ibv_get_device_list, ti_1) {

	struct ibv_device **dev_list;
	int num_devices = 0;

	dev_list = ibv_get_device_list(&num_devices);
	EXPECT_TRUE(dev_list != NULL);
	EXPECT_TRUE(num_devices);

	ibv_free_device_list(dev_list);
}

/* ibv_get_device_name: [TI.1] Correct */
TEST(tc_ibv_get_device_name, ti_1) {

	struct ibv_device **dev_list;
	int num_devices = 0;
	int i = 0;

	dev_list = ibv_get_device_list(&num_devices);
	EXPECT_TRUE(dev_list != NULL);
	EXPECT_TRUE(num_devices);

	for (i = 0; i < num_devices; ++i) {
		VERBS_INFO("    %-16s\t%016llx\n",
		       ibv_get_device_name(dev_list[i]),
		       (unsigned long long) be64toh(ibv_get_device_guid(dev_list[i])));
	}

	ibv_free_device_list(dev_list);
}

/* ibv_open_device: [TI.1] Correct */
TEST(tc_ibv_open_device, ti_1) {

	struct ibv_device **dev_list;
	struct ibv_device *ibv_dev;
	struct ibv_context*ibv_ctx;
	int num_devices = 0;
	int i;

	dev_list = ibv_get_device_list(&num_devices);
	ASSERT_TRUE(dev_list != NULL);
	ASSERT_TRUE(num_devices);

	for (i = 0; i < num_devices; ++i) {
		ibv_dev = dev_list[i];

		ibv_ctx = ibv_open_device(ibv_dev);
		ASSERT_TRUE(ibv_ctx != NULL);

		VERBS_INFO("    %-16s\t%016llx\n",
		       ibv_get_device_name(dev_list[i]),
		       (unsigned long long) be64toh(ibv_get_device_guid(dev_list[i])));
		       ibv_close_device(ibv_ctx);
	}
	ibv_free_device_list(dev_list);
}
