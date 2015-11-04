/**
* Copyright (C) Mellanox Technologies Ltd. 2015.  ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	GTEST_LOG_FATAL		= 1 << 0,
	GTEST_LOG_ERR		= 1 << 1,
	GTEST_LOG_WARN		= 1 << 2,
	GTEST_LOG_INFO		= 1 << 3,
	GTEST_LOG_TRACE		= 1 << 4
};

extern uint32_t gtest_debug_mask;
extern char *gtest_dev_name;

#define VERBS_INFO(fmt, ...)  \
	do {                                                           \
		if (gtest_debug_mask & GTEST_LOG_INFO)                 \
			printf("\033[0;3%sm" "[     INFO ] " fmt "\033[m", "4", ##__VA_ARGS__);    \
	} while(0)

#define VERBS_TRACE(fmt, ...) \
	do {                                                           \
		if (gtest_debug_mask & GTEST_LOG_TRACE)                 \
			printf("\033[0;3%sm" "[    TRACE ] " fmt "\033[m", "7", ##__VA_ARGS__);    \
	} while(0)


#define VERBS_ERROR(...) do { VERBS_TRACE(__VA_ARGS__); ASSERT_TRUE(0); } while(0)
#define VERBS_DO(x) do { \
	VERBS_TRACE("%d(i=%d): doing " #x "\n", __LINE__, i); \
	ASSERT_FALSE(x) << #x; \
} while(0)
#define VERBS_SET(x,y) do { \
	VERBS_TRACE("%d(i=%d): doing " #x "\n", __LINE__, i); \
	ASSERT_TRUE((x=(y))) << #y; \
} while (0)

void sys_hexdump(void *ptr, int buflen);
uint32_t sys_inet_addr(char* ip);

static inline void sys_getenv(void)
{
	char *env;

	env = getenv("IBV_TEST_MASK");
	if (env)
		gtest_debug_mask = strtol(env, NULL, 0);

	env = getenv("IBV_TEST_DEV");
	gtest_dev_name = strdup(env ? env : "mlx");
}

