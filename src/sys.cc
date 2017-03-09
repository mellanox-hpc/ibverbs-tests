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

uint32_t gtest_debug_mask = GTEST_LOG_ERR | GTEST_LOG_NOTICE;
char *gtest_dev_name;


void sys_hexdump(void *ptr, int buflen)
{
	unsigned char *buf = (unsigned char*)ptr;
	char out_buf[120];
	int ret = 0;
	int out_pos = 0;
	int i, j;

	VERBS_TRACE("dump data at %p\n", ptr);
	for (i=0; i<buflen; i+=16) {
		out_pos = 0;
		ret = sprintf(out_buf + out_pos, "%06x: ", i);
		if (ret < 0)
			return;
		out_pos += ret;
		for (j=0; j<16; j++) {
			if (i+j < buflen)
				ret = sprintf(out_buf + out_pos, "%02x ", buf[i+j]);
			else
				ret = sprintf(out_buf + out_pos, "   ");
			if (ret < 0)
				return;
			out_pos += ret;
		}
		ret = sprintf(out_buf + out_pos, " ");
		if (ret < 0)
			return;
		out_pos += ret;
		for (j=0; j<16; j++)
			if (i+j < buflen) {
				ret = sprintf(out_buf + out_pos, "%c", isprint(buf[i+j]) ? buf[i+j] : '.');
				if (ret < 0)
					return;
				out_pos += ret;
			}
		ret = sprintf(out_buf + out_pos, "\n");
		if (ret < 0)
			return;
		VERBS_TRACE("%s", out_buf);
	}
}

uint32_t sys_inet_addr(char* ip)
{
	int a, b, c, d;
	char addr[4];

	sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d);
	addr[0] = a;
	addr[1] = b;
	addr[2] = c;
	addr[3] = d;

	return *((uint32_t *)addr);
}
