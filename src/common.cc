/**
* Copyright (C) Mellanox Technologies Ltd. 2015.  ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#include <ctype.h>
#include "common.h"

uint32_t gtest_debug_mask = (GTEST_LOG_FATAL | GTEST_LOG_ERR | GTEST_LOG_WARN);
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
