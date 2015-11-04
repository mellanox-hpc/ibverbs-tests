AC_INIT([ibverbs-tests], [1.0])

AC_CONFIG_MACRO_DIR(config)
AM_INIT_AUTOMAKE([1.10 subdir-objects foreign])

AC_PROG_CC([gcc])
AC_PROG_CXX
AC_GNU_SOURCE
AC_PROG_LN_S

AC_CHECK_LIB([ibverbs], [ibv_get_device_list], [], [AC_MSG_ERROR([libibverbs not found])])

AC_HEADER_STDC
AC_C_CONST
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
