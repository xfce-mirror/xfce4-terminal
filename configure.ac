dnl $Id$
dnl
dnl Copyright (c) 2003-2004
dnl         os-cillation e.K. All rights reserved.
dnl
dnl Written by Benedikt Meurer <benny@xfce.org>.
dnl

dnl Version information
m4_define([terminal_version_major], [0])
m4_define([terminal_version_minor], [2])
m4_define([terminal_version_micro], [0])
m4_define([terminal_version_tag], [svn])
m4_define([terminal_version_dbus], [1])
m4_define([terminal_version], [terminal_version_major().terminal_version_minor().terminal_version_micro()terminal_version_tag()])

dnl Initialize autoconf
AC_COPYRIGHT([Copyright (c) 2003-2004
        os-cillation e.K. All rights reserved.

Written by Benedikt Meurer <benny@xfce.org>.])
AC_INIT([Terminal], [terminal_version()], [http://bugs.os-cillation.com/])
AC_PREREQ([2.50])
AC_REVISION([$Id$])

dnl Initialize automake
TERMINAL_VERSION=terminal_version()
AM_INIT_AUTOMAKE([Terminal], [$TERMINAL_VERSION])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

dnl Subst D-BUS service version
TERMINAL_VERSION_DBUS=terminal_version_dbus()
AC_SUBST([TERMINAL_VERSION_DBUS])

dnl check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl Check for standard headers
AC_CHECK_HEADERS([errno.h limits.h pwd.h time.h])

dnl Check for i18n support
BM_I18N([Terminal], [de fr nl])

dnl Check for required packages
BM_DEPEND([EXO], [exo-0.2], [0.2.0])
BM_DEPEND([VTE], [vte], [0.11.11])

dnl Optional support for startup notification
BM_DEPEND_CHECK([LIBSTARTUP_NOTIFICATION], [libstartup-notification-1.0], [0.4],
                [startup-notification], [startup notification library], [yes])

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
Makefile
Terminal.spec
doc/Makefile
doc/C/Makefile
doc/C/images/Makefile
pixmaps/Makefile
po/Makefile.in
terminal/Makefile
terminal/terminal-config.h
])
