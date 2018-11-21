dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(camannex, 0.1)


AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()

AC_PROG_CXX
AC_PROG_CC

AC_CHECK_FUNCS([mkdir ftruncate memset sqrt pow gettimeofday select socket strerror])
AC_CHECK_HEADERS([unistd.h fcntl.h syslog.h netdb.h float.h sys/socket.h sys/time.h])
AC_CHECK_HEADER_STDBOOL
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T
AC_TYPE_INT8_T

AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT
