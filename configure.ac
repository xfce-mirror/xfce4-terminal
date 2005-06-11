dnl $Id$
dnl
dnl Copyright (c) 2003-2005
dnl         os-cillation e.K. All rights reserved.
dnl
dnl Written by Benedikt Meurer <benny@xfce.org>.
dnl

dnl Version information
m4_define([terminal_version_major], [0])
m4_define([terminal_version_minor], [2])
m4_define([terminal_version_micro], [5])
m4_define([terminal_version_tag], [cvs])
m4_define([terminal_version_dbus], [3])
m4_define([terminal_version], [terminal_version_major().terminal_version_minor().terminal_version_micro()terminal_version_tag()])

dnl Initialize autoconf
AC_COPYRIGHT([Copyright (c) 2003-2005
        os-cillation e.K. All rights reserved.

Written by Benedikt Meurer <benny@xfce.org>.])
AC_INIT([Terminal], [terminal_version()], [http://bugs.os-cillation.com/], [Terminal])
AC_REVISION([$Id$])

dnl Initialize automake
AM_INIT_AUTOMAKE([AC_PACKAGE_TARNAME()], [AC_PACKAGE_VERSION()])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE()

dnl Subst D-BUS service version
TERMINAL_VERSION_DBUS=terminal_version_dbus()
AC_SUBST([TERMINAL_VERSION_DBUS])

dnl check for UNIX variants
AC_AIX()
AC_ISC_POSIX()
AC_MINIX()

dnl check for basic programs
AC_PROG_CC()
AC_PROG_INSTALL()
AC_PROG_INTLTOOL()
AC_PROG_LIBTOOL()

dnl Check for standard headers
AC_CHECK_HEADERS([ctype.h errno.h limits.h pwd.h time.h])

dnl Check for i18n support
XDT_I18N([de en_GB es fi fr he hu ja nl ru zh_CN zh_TW])

dnl Check for required packages
XDT_CHECK_PACKAGE([EXO], [exo-0.3], [0.3.0])
XDT_CHECK_PACKAGE([VTE], [vte], [0.11.11])

dnl Optional support for startup notification
XDT_CHECK_OPTIONAL_PACKAGE([LIBSTARTUP_NOTIFICATION],
                           [libstartup-notification-1.0],
                           [0.4], [startup-notification],
                           [startup notification library])

dnl Optional support for D-BUS
XDT_CHECK_OPTIONAL_PACKAGE([DBUS], [dbus-glib-1], [0.22],
                           [dbus], [D-BUS GLib bindings])
if test x"$DBUS_FOUND" = x"yes"; then
  AC_MSG_CHECKING([whether to use new D-BUS API])
  if $PKG_CONFIG --atleast-version 0.31 dbus-1 >/dev/null 2>&1; then
    AC_DEFINE([DBUS_USE_NEW_API], [1], [Define to use new D-BUS API])
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([no])
  fi
fi


dnl Check for xsltproc
AC_ARG_ENABLE([xsltproc], [AC_HELP_STRING([--enable-xsltproc], [Use xsltproc to build documentation [default=no]])],, [enable_xsltproc=no])
if test x"$enable_xsltproc" = x"yes"; then
  AC_PATH_PROG([XSLTPROC], [xsltproc], [no])
  if test x"$XSLTPROC" = x"no"; then
    enable_xsltproc=no
  fi
fi
AM_CONDITIONAL([ENABLE_XSLTPROC], [test x"$enable_xsltproc" = x"yes"])


dnl Check for debugging support
BM_DEBUG_SUPPORT()

AC_OUTPUT([
Makefile
Terminal.spec
doc/Makefile
doc/C/Makefile
doc/C/Terminal.xml
doc/C/images/Makefile
helpers/Makefile
pixmaps/Makefile
po/Makefile.in
terminal/stock-icons/Makefile
terminal/Makefile
terminal/terminal-config.h
])
