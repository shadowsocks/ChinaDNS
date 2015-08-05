#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([ChinaDNS], [1.3.2], [clowwindy42@gmail.com])
AC_CONFIG_SRCDIR([src/chinadns.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
AC_CHECK_LIB(resolv, res_query, [],
    [AC_CHECK_LIB(resolv, __res_query, [],
        [AC_MSG_ERROR([libresolv not found.])])])

# Checks for header files.
AC_HEADER_RESOLV
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T

# Checks for library functions.
# To fix rpl_malloc undefined error in mips cross-compile enviroment.
AC_CHECK_FUNCS([malloc realloc])
AC_CHECK_FUNCS([inet_ntoa memset select socket strchr strdup strrchr])

AC_ARG_ENABLE([debug],
    [  --enable-debug          build with additional debugging code],
    [CFLAGS='-g -DDEBUG -fprofile-arcs -ftest-coverage -O0'])

AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

# Add a option to force static link the target.
AC_ARG_ENABLE([static],
    [  --enable-static         build with static linking],
    [LDFLAGS='-static'])

AM_CONDITIONAL(STATIC, test x"$static" = x"true")

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
