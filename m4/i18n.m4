dnl I18n support
dnl
dnl Copyright (c) 2003  Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
dnl

dnl BM_I18N(pkgname, languages)
AC_DEFUN([BM_I18N],
[
  GETTEXT_PACKAGE=$1
  AC_SUBST([GETTEXT_PACKAGE])
  AC_DEFINE([GETTEXT_PACKAGE], ["$1"], [Name of default gettext domain])

  ALL_LINGUAS="$2"

  AM_GLIB_GNU_GETTEXT

  dnl This is required on some linux systems
  AC_CHECK_FUNC([bind_textdomain_codeset])

  AC_MSG_CHECKING([for locales directory])
  AC_ARG_WITH([locales-dir],
    AC_HELP_STRING([--with-locales-dir=DIR], [Install locales into DIR]),
    [localedir=$withval],
    [localedir=$datadir/locale])
  AC_MSG_RESULT([$localedir])
  AC_SUBST([localedir])
])
