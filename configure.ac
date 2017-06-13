# Copyright (C) 2015      Mellanox Technologies Ltd. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

dnl Process this file with autoconf to produce a configure script.

define([scm_r], esyscmd([sh -c "git rev-parse --short=7 HEAD"])) dnl
define([ibverbs_tests_ver_major], 1)
define([ibverbs_tests_ver_minor], 0)
define([ts], esyscmd([sh -c "date +%Y%m%d%H%M%S"])) dnl
define([revcount], esyscmd([git rev-list HEAD | wc -l | sed -e 's/ *//g' | xargs -n1 printf])) dnl

AC_INIT([ibverbs-tests], [ibverbs_tests_ver_major.ibverbs_tests_ver_minor])

AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(config)
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([1.10 foreign tar-ustar subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl Checks for programs
AC_PROG_CC([mpicc gcc icc])
AC_PROG_CXX
AC_GNU_SOURCE
AC_PROG_LN_S
AC_PROG_LIBTOOL

##########################
# Enable support for valgrind
#
AC_ARG_WITH([valgrind],
    AC_HELP_STRING([--with-valgrind],
        [Enable Valgrind annotations (small runtime overhead, default NO)]))
if test x$with_valgrind = x || test x$with_valgrind = xno; then
    want_valgrind=no
    AC_DEFINE([NVALGRIND], 1, [Define to 1 to disable Valgrind annotations.])
else
    want_valgrind=yes
    if test -d $with_valgrind; then
        CPPFLAGS="$CPPFLAGS -I$with_valgrind/include"
    fi
fi

dnl Checks for libraries
AC_CHECK_LIB([ibverbs], [ibv_get_device_list], [], [AC_MSG_ERROR([libibverbs not found])])

AC_CHECK_LIB(dl, dlsym, [],
    AC_MSG_ERROR([dlsym() not found.  ibverbs-tests requires libdl.]))

AC_CHECK_LIB(pthread, pthread_mutex_init, [],
    AC_MSG_ERROR([pthread_mutex_init() not found.  ibverbs-tests requires libpthread.]))

dnl Checks for header files.
AC_HEADER_STDC

if test x$want_valgrind = xyes; then
AC_CHECK_HEADER(valgrind/memcheck.h,
    [AC_DEFINE(HAVE_VALGRIND_MEMCHECK_H, 1,
        [Define to 1 if you have the <valgrind/memcheck.h> header file.])],
    [if test $want_valgrind = yes; then
        AC_MSG_ERROR([Valgrind memcheck support requested, but <valgrind/memcheck.h> not found.])
    fi])
fi

AC_CHECK_DECLS([htobe64], [], [], [])


AC_CHECK_HEADERS([infiniband/verbs_exp.h])

AC_CHECK_HEADER([infiniband/peer_ops.h], [
    peerdirect=1
    AC_ARG_WITH([peer],
	AC_HELP_STRING([--with-peerdirect-meminval],
	    [Enable Peer-direct memory invalidation.]))
    AC_CHECK_DECL([ibv_exp_peer_commit_qp], [
	AC_DEFINE(PEER_DIRECT_EXP,1,[Experimetal Peer-Direct API])], [], [
#include<infiniband/peer_ops.h>
])])
AM_CONDITIONAL([PEER_DIRECT], [test x$peerdirect = x1])
AM_CONDITIONAL([PEER_DIRECT_INVALIDATION_TEST],
    [test x$with_peerdirect_meminval = xyes])

AC_CHECK_DECL([IBV_SRQT_TAG_MATCHING], [tag_matching=1], [], [
#include <infiniband/verbs.h>
])
AC_CHECK_DECL([IBV_EXP_SRQT_TAG_MATCHING], [tag_matching=1], [], [
#include <infiniband/verbs_exp.h>
])
AM_CONDITIONAL([TAG_MATCHING], [test x$tag_matching = x1])
AC_CHECK_DECLS([ibv_post_srq_ops], [tag_matching_2=1], [], [
#include <infiniband/verbs.h>
])
AC_CHECK_DECLS([ibv_exp_post_srq_ops], [tag_matching_2=1], [], [
#include <infiniband/verbs_exp.h>
])
AM_CONDITIONAL([TAG_MATCHING_V0_2], [test x$tag_matching_2 = x1])

AC_CHECK_TYPES([struct ibv_exp_tmh_ravh], [], [], [
#include <infiniband/verbs_exp.h>
])

AC_CHECK_DECLS([ibv_prefetch_mr], [], [], [
#include <infiniband/verbs.h>
])

AC_CHECK_DECLS([ibv_exp_prefetch_mr], [], [], [
#include <infiniband/verbs_exp.h>
])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

AC_CHECK_DECL([IBV_DEVICE_CROSS_CHANNEL], [cross_channel=1],
		[cross_channel=0],
		[[#include <infiniband/verbs.h>]])

AC_CHECK_DECLS([IBV_ACCESS_HUGETLB], [hugetlb=1], [], [
#include <infiniband/verbs.h>
])

if test x$cross_channel = x1; then
	AC_DEFINE(HAVE_CROSS_CHANNEL, 1, [Cross-channel is supported])
fi

AC_CHECK_MEMBERS([struct ibv_flow_spec_tunnel.val], [Vxlan=1],
		[Vxlan=0],
		[[#include <infiniband/verbs.h>]])

AC_CHECK_MEMBERS([struct ibv_exp_flow_spec_tunnel.val], [Vxlan_exp=1],
		[Vxlan_exp=0],
		[[#include <infiniband/verbs_exp.h>]])

if test x$Vxlan_exp = x1 || test x$Vxlan = x1; then
	AC_DEFINE(HAVE_VXLAN, 1, [Vxlan is supported])
fi

if test $(basename ${CC}x) = mpiccx; then
    AC_DEFINE(HAVE_MPICC, 1, [Use mpirun as a launcher])
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
