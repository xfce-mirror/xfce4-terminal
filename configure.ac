dnl $Id$
dnl
dnl Copyright (c) 2003-2004
dnl         os-cillation e.K. All rights reserved.
dnl
dnl Written by Benedikt Meurer <benny@xfce.org>.
dnl

dnl Version information
m4_define([terminal_version_major], [0])
m4_define([terminal_version_minor], [1])
m4_define([terminal_version_micro], [5])
m4_define([terminal_version], [terminal_version_major.terminal_version_minor.terminal_version_micro])

dnl Initialize autoconf
AC_COPYRIGHT([Copyright (c) 2004
        os-cillation e.K. All rights reserved.

Written by Benedikt Meurer <benny@xfce.org>.])
AC_INIT([Terminal], [terminal_version], [http://bugs.os-cillation.de/])
AC_PREREQ([2.50])
AC_REVISION([$Id$])

dnl Initialize automake
TERMINAL_VERSION=terminal_version
AM_INIT_AUTOMAKE([Terminal], [$TERMINAL_VERSION])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

dnl check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl Check for standard headers
AC_CHECK_HEADERS([limits.h pwd.h time.h])

dnl Check for i18n support
BM_I18N([Terminal], [de])

dnl Check for required packages
BM_DEPEND([EXO], [exo-0.1], [0.1.0])
BM_DEPEND([VTE], [vte], [0.11.0])

dnl Check for debugging support
BM_DEBUG_SUPPORT

dnl Check whether to build a final version
AC_ARG_ENABLE([final],
AC_HELP_STRING([--enable-final], [Build final]),
  [], [enable_final=no])
AC_MSG_CHECKING([whether to build final])
if test x"$enable_final" = x"no"; then
  AC_MSG_RESULT([no])
else
  CPPFLAGS="$CPPFLAGS -DG_DISABLE_CHECKS -DG_DISABLE_ASSERT"
  CPPFLAGS="$CPPFLAGS -DG_DISABLE_CAST_CHECKS"
  AC_MSG_RESULT([yes])
fi

AC_OUTPUT([
Makefile
Terminal.spec
doc/Makefile
doc/C/Makefile
doc/C/images/Makefile
icons/Makefile
icons/48x48/Makefile
po/Makefile.in
terminal/Makefile
])
