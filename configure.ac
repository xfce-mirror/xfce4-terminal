dnl
dnl Copyright (c) 2003-2009
dnl         os-cillation e.K. All rights reserved.
dnl
dnl Written by Benedikt Meurer <benny@xfce.org>.
dnl

dnl ***************************
dnl *** Version information ***
dnl ***************************
XDT_VERSION_INIT([1.1.5], [git])
m4_define([terminal_version_dbus], [5])
m4_define([copyright_year], [2025])

dnl minimum required versions
m4_define([glib_min_version], [2.44.0])
m4_define([gtk_min_version], [3.22.0])
m4_define([vte_min_version], [0.51.3])
m4_define([pcre2_min_version], [10.00])
m4_define([libxfce4ui_min_version], [4.17.5])
m4_define([libxfce4util_min_version], [4.16.0])
m4_define([xfconf_min_version], [4.16.0])

m4_define([libx11_min_version], [1.6.7])
m4_define([gtk_layer_shell_min_version], [0.7.0])

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_COPYRIGHT([Copyright (c) 2003-copyright_year()
        The Xfce development team. All rights reserved.
        os-cillation e.K. All rights reserved.

Written by Benedikt Meurer <benny@xfce.org>.])
AC_INIT([xfce4-terminal], [xdt_version], [https://gitlab.xfce.org/apps/xfce4-terminal/])
AC_REVISION([xdt_version_build])
AC_PREREQ([2.69])
AC_ARG_PROGRAM()
AC_DEFINE([VERSION_FULL], [PACKAGE_VERSION], [Alias for VERSION and PACKAGE_VERSION for meson compatibility])
COPYRIGHT_YEAR=copyright_year()
AC_DEFINE_UNQUOTED([COPYRIGHT_YEAR], ["$COPYRIGHT_YEAR"], [Copyright year])
AC_SUBST([COPYRIGHT_YEAR])

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AM_INIT_AUTOMAKE([1.8 dist-bzip2 tar-ustar no-dist-gzip foreign])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE()
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_MACRO_DIRS([m4])

dnl ***********************************
dnl *** Subst D-BUS service version ***
dnl ***********************************
TERMINAL_VERSION_DBUS=terminal_version_dbus()
AC_SUBST([TERMINAL_VERSION_DBUS])

dnl *******************************
dnl *** Check for UNIX variants ***
dnl *******************************
AC_USE_SYSTEM_EXTENSIONS

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AC_PROG_CC()
AM_PROG_CC_C_O()
AC_PROG_INSTALL()

dnl **************************
dnl *** Initialize libtool ***
dnl **************************
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl **********************************
dnl *** Check for standard headers ***
dnl **********************************
AC_CHECK_HEADERS([ctype.h errno.h limits.h pwd.h signal.h time.h unistd.h locale.h stdlib.h])

dnl ******************************
dnl *** Check for i18n support ***
dnl ******************************
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl ***********************************
dnl *** Check for optional packages ***
dnl ***********************************
XDT_CHECK_OPTIONAL_FEATURE([X11],
                           [x11],
                           [
                             XDT_FEATURE_DEPENDENCY([LIBX11], [x11], [libx11_min_version])
                             XDT_FEATURE_DEPENDENCY([GDK_X11], [gdk-x11-3.0], [gtk_min_version])
                           ],
                           [the X11 windowing system])
XDT_CHECK_OPTIONAL_FEATURE([WAYLAND],
                           [wayland],
                           [
                             XDT_FEATURE_DEPENDENCY([GDK_WAYLAND], [gdk-wayland-3.0], [gtk_min_version])
                           ],
                           [the Wayland windowing system])
if test x"$ENABLE_X11" != x"yes" -a x"$ENABLE_WAYLAND" != x"yes"; then
  AC_MSG_ERROR([Either both X11 and Wayland support was disabled, or required dependencies are missing. One of the two must be enabled.])
fi
if test x"$ENABLE_WAYLAND" != x"yes"; then
  dnl Do not check for optional Wayland packages if Wayland is disabled
  enable_gtk_layer_shell=no
fi
XDT_CHECK_OPTIONAL_PACKAGE([GTK_LAYER_SHELL], [gtk-layer-shell-0], [gtk_layer_shell_min_version], [gtk-layer-shell], [Gtk Layer Shell support])

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GIO], [gio-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [gtk_min_version])
XDT_CHECK_PACKAGE([VTE], [vte-2.91], [vte_min_version])
XDT_CHECK_PACKAGE([PCRE2], [libpcre2-8], [pcre2_min_version])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [libxfce4ui_min_version])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [libxfce4util_min_version])
XDT_CHECK_PACKAGE([XFCONF], [libxfconf-0], [xfconf_min_version])
XDT_CHECK_PACKAGE([LIBXFCE4KBD_PRIVATE], [libxfce4kbd-private-3], [libxfce4ui_min_version])

dnl ****************************************
dnl *** Check if we need to use utempter ***
dnl ****************************************
AC_ARG_WITH([utempter], [AS_HELP_STRING([--with-utempter],
            [Support for utmp/wtmp files (requires libutempter) [default=no]])],, [with_utempter=no])
if test x"$with_utempter" = x"yes"; then
    AC_CHECK_LIB([utempter],[utempter_add_record],,[with_utempter=no])

    if test x"$with_utempter" = x"no"; then
        AC_MSG_ERROR([Compiling with --with-utempter but libutempter not found.])
    fi
fi
AM_CONDITIONAL([HAVE_UTEMPTER], [test x"$with_utempter" = x"yes"])

dnl ****************
dnl *** Check OS ***
dnl ****************
AC_CANONICAL_HOST
AS_CASE([$host],
		[*-*freebsd*], [freebsd=yes],
		[freebsd=no])

AM_CONDITIONAL([FREEBSD], [test x$freebsd = xyes])

dnl *************************
dnl *** Check for gtk-doc ***
dnl *************************
GTK_DOC_CHECK([1.9])

dnl ***********************************
dnl *** Check for debugging support ***
dnl ***********************************
XDT_FEATURE_DEBUG([xdt_debug_default])

dnl **************************************
dnl *** Check for linker optimizations ***
dnl **************************************
XDT_FEATURE_LINKER_OPTS()

dnl *********************************
dnl *** Substitute platform flags ***
dnl *********************************
AC_MSG_CHECKING([PLATFORM_CPPFLAGS])
AC_MSG_RESULT([$PLATFORM_CPPFLAGS])
AC_SUBST([PLATFORM_CPPFLAGS])
AC_MSG_CHECKING([PLATFORM_CFLAGS])
AC_MSG_RESULT([$PLATFORM_CFLAGS])
AC_SUBST([PLATFORM_CFLAGS])
AC_MSG_CHECKING([PLATFORM_LDFLAGS])
AC_MSG_RESULT([$PLATFORM_LDFLAGS])
AC_SUBST([PLATFORM_LDFLAGS])

AC_CONFIG_FILES([
Makefile
colorschemes/Makefile
doc/Makefile
po/Makefile.in
terminal/Makefile
terminal/terminal-config.h
icons/Makefile
icons/16x16/Makefile
icons/24x24/Makefile
icons/32x32/Makefile
icons/48x48/Makefile
icons/64x64/Makefile
icons/96x96/Makefile
icons/128x128/Makefile
icons/scalable/Makefile
])
AC_OUTPUT

dnl ***************************
dnl *** Print configuration ***
dnl ***************************
echo
echo "Build Configuration:"
echo
echo "* Debug support:             $enable_debug"
echo "* X11 support:               ${ENABLE_X11:-no}"
echo "* Wayland support:           ${ENABLE_WAYLAND:-no}"
if test x"$ENABLE_WAYLAND" = x"yes"; then
echo "  * Gtk Layer Shell support: ${GTK_LAYER_SHELL_FOUND:-no}"
fi
echo
