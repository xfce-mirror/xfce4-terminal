dnl From Benedikt Meurer (benedikt.meurer@unix-ag.uni-siegen.de)
dnl
dnl if debug support is requested:
dnl
dnl   1) defines DEBUG to 1
dnl   2) adds requested debug level flags to CFLAGS
dnl

AC_DEFUN([BM_DEBUG_SUPPORT],
[
dnl # --enable-debug
  AC_ARG_ENABLE([debug],
AC_HELP_STRING([--enable-debug[=yes|no|full]], [Build with debugging support])
AC_HELP_STRING([--disable-debug], [Include no debugging support [default]]),
    [], [enable_debug=no])

  AC_MSG_CHECKING([whether to build with debugging support])
  if test x"$enable_debug" != x"no"; then
    AC_DEFINE(DEBUG, 1, Define for debugging support)
    if test x"$enable_debug" = x"full"; then
      AC_DEFINE(DEBUG_TRACE, 1, Define for tracing support)
      CFLAGS="$CFLAGS -g3 -Wall -Werror -DXFCE_DISABLE_DEPRECATED"
      AC_MSG_RESULT([full])
    else
      CFLAGS="$CFLAGS -g -Wall -DXFCE_DISABLE_DEPRECATED"
      AC_MSG_RESULT([yes])
    fi
  else
    AC_MSG_RESULT([no])
  fi

dnl # --enable-profiling
  AC_ARG_ENABLE([profiling],
AC_HELP_STRING([--enable-profiling],
    [Generate extra code to write profile information])
AC_HELP_STRING([--disable-profiling],
    [No extra code for profiling (default)]),
    [], [enable_profiling=no])

  AC_MSG_CHECKING([whether to build with profiling support])
  if test x"$enable_profiling" != x"no"; then
    CFLAGS="$CFLAGS -pg"
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([no])
  fi

dnl # --enable-gcov
  AC_ARG_ENABLE([gcov],
AC_HELP_STRING([--enable-gcov],
    [compile with coverage profiling instrumentation (gcc only)])
AC_HELP_STRING([--disable-gcov],
    [do not generate coverage profiling instrumentation (default)]),
    [], [enable_gcov=no])

  AC_MSG_CHECKING([whether to compile with coverage profiling instrumentation])
  if test x"$enable_gcov" != x"no"; then
    CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([no])
  fi

dnl # --enable-final
  AC_REQUIRE([AC_PROG_LD])
  AC_ARG_ENABLE([final],
AC_HELP_STRING([--enable-final], [Build final version]),
    [], [enable_final=no])

  AC_MSG_CHECKING([whether to build final version])
  if test x"$enable_final" = x"yes"; then
    AC_MSG_RESULT([yes])
    CPPFLAGS="$CPPFLAGS -DG_DISABLE_CHECKS -DG_DISABLE_ASSERT"
    CPPFLAGS="$CPPFLAGS -DG_DISABLE_CAST_CHECKS"
    AC_MSG_CHECKING([whether $LD accepts -O1])
    case `$LD -O1 -v 2>&1 </dev/null` in
    *GNU* | *'with BFD'*)
      LDFLAGS="$LDFLAGS -Wl,-O1"
      AC_MSG_RESULT([yes])
    	;;
    *)
      AC_MSG_RESULT([no])
    	;;
    esac
  else
    AC_MSG_RESULT([no])
  fi
])
