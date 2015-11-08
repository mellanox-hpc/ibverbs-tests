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
#ifndef _IBVERBS_VERBS_TEST_
#define _IBVERBS_VERBS_TEST_
#include "common.h"
#include <infiniband/verbs.h>
#include <infiniband/arch.h>

/**
 * Base class for ibverbs test fixtures.
 * Initialize and close ibverbs.
 */
class verbs_test : public testing::Test {
protected:
	virtual void SetUp() {
		int num_devices = 0;
		int i = 0;

		errno = EOK;
		/*
		 * First you must retrieve the list of available IB devices on the local host.
		 * Every device in this list contains both a name and a GUID.
		 * For example the device names can be: mthca0, mlx4_1.
		 */
		dev_list = ibv_get_device_list(&num_devices);
		ASSERT_TRUE(errno == EOK);
		ASSERT_TRUE(dev_list != NULL);
		ASSERT_TRUE(num_devices);

		/*
		 * Iterate over the device list, choose a device according to its GUID or name
		 * and open it.
		 */
		for (i = 0; i < num_devices; ++i) {
			if (!strncmp(ibv_get_device_name(dev_list[i]),
				     gtest_dev_name, strlen(gtest_dev_name))) {
					ibv_dev = dev_list[i];
					break;
			}
		}
		ASSERT_TRUE(ibv_dev != NULL);

		ibv_ctx = ibv_open_device(ibv_dev);
		ASSERT_TRUE(ibv_ctx != NULL);
	}

	virtual void TearDown() {

		/*
		 * Destroy objects in the reverse order you created them:
		 * Delete QP
		 * Delete CQ
		 * Deregister MR
		 * Deallocate PD
		 * Close device
		 */
		ibv_close_device(ibv_ctx);
		ibv_free_device_list(dev_list);
	}

protected:
	struct ibv_device	**dev_list;
	struct ibv_device	*ibv_dev;
	struct ibv_context	*ibv_ctx;
};
#endif //_IBVERBS_VERBS_TEST_
