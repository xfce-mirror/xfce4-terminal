dnl From Benedikt Meurer (benedikt.meurer@unix-ag.uni-siegen.de)
dnl Check for X11

AC_DEFUN([BM_LIBX11],
[
  AC_REQUIRE([AC_PATH_XTRA])
  LIBX11_CFLAGS= LIBX11_LDFLAGS= LIBX11_LIBS=
  if test "$no_x" != "yes"; then
    AC_CHECK_LIB(X11, main,
    [
      AC_DEFINE(HAVE_LIBX11, 1, Define if libX11 is available)
      LIBX11_CFLAGS="$X_CFLAGS"
      for option in $X_PRE_LIBS $X_EXTRA_LIBS $X_LIBS; do
      	case "$option" in
        -L*)
          path=`echo $option | sed 's/^-L//'`
          if test x"$path" != x""; then
            LIBX11_LDFLAGS="$LIBX11_LDFLAGS -L$path"
          fi
          ;;
        *)
          LIBX11_LIBS="$LIBX11_LIBS $option"
          ;;
        esac
      done
      if ! echo $LIBX11_LIBS | grep -- '-lX11' > /dev/null; then
        LIBX11_LIBS="$LIBX11_LIBS -lX11"
      fi
    ], [], [$X_CFLAGS $X_PRE_LIBS $X_EXTRA_LIBS $X_LIBS])
  fi
  AC_SUBST(LIBX11_CFLAGS)
  AC_SUBST(LIBX11_LDFLAGS)
  AC_SUBST(LIBX11_LIBS)
])

AC_DEFUN([BM_LIBX11_REQUIRE],
[
  AC_REQUIRE([BM_LIBX11])
  if test "$no_x" = "yes"; then
    AC_MSG_ERROR([X Window system libraries and header files are required])
  fi
])

AC_DEFUN([BM_LIBSM],
[
  AC_REQUIRE([BM_LIBX11])
  LIBSM_CFLAGS= LIBSM_LDFLAGS= LIBSM_LIBS=
  if test "$no_x" != "yes"; then
    AC_CHECK_LIB(SM, SmcSaveYourselfDone,
    [
      AC_DEFINE(HAVE_LIBSM, 1, Define if libSM is available)
      LIBSM_CFLAGS="$LIBX11_CFLAGS"
      LIBSM_LDFLAGS="$LIBX11_LDFLAGS"
      LIBSM_LIBS="$LIBX11_LIBS"
      if ! echo $LIBSM_LIBS | grep -- '-lSM' > /dev/null; then
        LIBSM_LIBS="$LIBSM_LIBS -lSM -lICE"
      fi
    ], [], [$LIBX11_CFLAGS $LIBX11_LDFLAGS $LIBX11_LIBS -lICE])
  fi
  AC_SUBST(LIBSM_CFLAGS)
  AC_SUBST(LIBSM_LDFLAGS)
  AC_SUBST(LIBSM_LIBS)
])

AC_DEFUN([BM_LIBXPM],
[
  AC_REQUIRE([BM_LIBX11])
  LIBXPM_CFLAGS= LIBXPM_LDFLAGS= LIBXPM_LIBS=
  if test "$no_x" != "yes"; then
    AC_CHECK_LIB(Xpm, main,
    [
      AC_DEFINE([HAVE_LIBXPM], [1], [Define if libXpm is available])
      LIBXPM_CFLAGS="$LIBX11_CFLAGS"
      LIBXPM_LDFLAGS="$LIBX11_LDFLAGS"
      LIBXPM_LIBS="$LIBX11_LIBS"
      if ! echo $LIBXPM_LIBS | grep -- '-lXpm' > /dev/null; then
        LIBXPM_LIBS="$LIBXPM_LIBS -lXpm"
      fi
    ], [], [$LIBX11_CFLAGS $LIBX11_LDFLAGS $LIBX11_LIBS -lXpm])
  fi
  AC_SUBST([LIBXPM_CFLAGS])
  AC_SUBST([LIBXPM_LDFLAGS])
  AC_SUBST([LIBXPM_LIBS])
])

AC_DEFUN([BM_LIBXPM_REQUIRE],
[
  AC_REQUIRE([BM_LIBX11_REQUIRE])
  AC_REQUIRE([BM_LIBXPM])
  if test -z "$LIBXPM_LIBS"; then
    AC_MSG_ERROR([The Xpm library was not found on you system])
  fi
])

AC_DEFUN([BM_LIBXINERAMA],
[
  AC_ARG_ENABLE(xinerama,
AC_HELP_STRING([--enable-xinerama], [enable xinerama extension])
AC_HELP_STRING([--disable-xinerama], [disable xinerama extension [default]]),
      [], [enable_xinerama=no])
  LIBXINERAMA_CFLAGS= LIBXINERAMA_LDFLAGS= LIBXINERAMA_LIBS=
  if test "x$enable_xinerama" = "xyes"; then
    AC_REQUIRE([BM_LIBX11_REQUIRE])
    AC_CHECK_LIB(Xinerama, XineramaQueryScreens,
    [
      AC_DEFINE(HAVE_LIBXINERAMA, 1, Define if XFree86 Xinerama is available)
      LIBXINERAMA_CFLAGS="$LIBX11_CFLAGS"
      LIBXINERAMA_LDFLAGS="$LIBX11_LDFLAGS"
      LIBXINERAMA_LIBS="$LIBX11_LIBS"
      if ! echo $LIBXINERAMA_LIBS | grep -- '-lXinerama' > /dev/null; then
        LIBXINERAMA_LIBS="$LIBXINERAMA_LIBS -lXinerama"
      fi
      if ! echo $LIBXINERAMA_LIBS | grep -- '-lXext' > /dev/null; then
        LIBXINERAMA_LIBS="$LIBXINERAMA_LIBS -lXext"
      fi
    ],[], [$LIBX11_CFLAGS $LIBX11_LDFLAGS $LIBX11_LIBS -lXext])
  fi
  AC_SUBST(LIBXINERAMA_CFLAGS)
  AC_SUBST(LIBXINERAMA_LDFLAGS)
  AC_SUBST(LIBXINERAMA_LIBS)
])

