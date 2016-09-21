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

#ifndef _IBVERBS_COMMON_H_
#define _IBVERBS_COMMON_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>   /* printf PRItn */
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <inttypes.h>   /* printf PRItn */
#include <fcntl.h>
#include <poll.h>
#include <ctype.h>
#include <malloc.h>
#include <math.h>
#include <complex.h>

#include "gtest.h"

#define INLINE  __inline

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) ((void)P)
#endif

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))


/* Platform specific 16-byte alignment macro switch.
   On Visual C++ it would substitute __declspec(align(16)).
   On GCC it substitutes __attribute__((aligned (16))).
*/

#if defined(_MSC_VER)
#define ALIGN(x) __declspec(align(x))
#else
#define ALIGN(x) __attribute__((aligned (x)))
#endif

#pragma pack( push, 1 )
typedef struct _DATA128_TYPE
{
   uint64_t    field1;
   uint64_t    field2;
}  DATA128_TYPE;
#pragma pack( pop )


#if !defined( EOK )
#  define EOK 0         /* no error */
#endif

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


#define CHECK_TEST_OR_SKIP(FEATURE_NAME) \
	do{\
		  if(this->skip_this_test) {\
			       std::cout << "[  SKIPPED ] Feature " << #FEATURE_NAME << " is not supported" << std::endl;\
			       ::testing::UnitTest::GetInstance()->runtime_skip(); \
			       return;\
			    }\
	} while(0)


void sys_hexdump(void *ptr, int buflen);
uint32_t sys_inet_addr(char* ip);


static INLINE void sys_getenv(void)
{
	char *env;

	env = getenv("IBV_TEST_MASK");
	if (env)
		gtest_debug_mask = strtol(env, NULL, 0);

	env = getenv("IBV_TEST_DEV");
	gtest_dev_name = strdup(env ? env : "mlx");
}

static INLINE int sys_is_big_endian(void)
{
	return( htonl(1) == 1 );
}

static INLINE double sys_gettime(void)
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (double)(tv.tv_sec * 1000000 + tv.tv_usec);
}

static INLINE uint64_t sys_rdtsc(void)
{
	unsigned long long int result=0;

#if defined(__i386__)
	__asm volatile(".byte 0x0f, 0x31" : "=A" (result) : );

#elif defined(__x86_64__)
	unsigned hi, lo;
	__asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
	result = hi;
	result = result<<32;
	result = result|lo;

#elif defined(__powerpc__)
	unsigned long int hi, lo, tmp;
	__asm volatile(
	    "0:                 \n\t"
	    "mftbu   %0         \n\t"
	    "mftb    %1         \n\t"
	    "mftbu   %2         \n\t"
	    "cmpw    %2,%0      \n\t"
	    "bne     0b         \n"
	    : "=r"(hi),"=r"(lo),"=r"(tmp)
	    );
	result = hi;
	result = result<<32;
	result = result|lo;

#endif

	return (result);
}
#endif //_IBVERBS_COMMON_H_
