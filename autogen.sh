#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

DIE=0

ACLOCAL_FLAGS="-I m4 $ACLOCAL_FLAGS"

AUTOCONF=${AUTOCONF:-autoconf}
AUTOHEADER=${AUTOHEADER:-autoheader}
AUTOMAKE=${AUTOMAKE:-automake}
LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}
ACLOCAL=${ACLOCAL:-aclocal}

(test -f $srcdir/configure.ac) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level package directory"
    exit 1
}

(${AUTOCONF} --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autoconf' installed."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
}

(grep "^AC_PROG_LIBTOOL" $srcdir/configure.ac >/dev/null) && {
  (${LIBTOOLIZE} --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`libtool' installed."
    echo "You can get it from: ftp://ftp.gnu.org/pub/gnu/"
    DIE=1
  }
}

(grep "^AM_GLIB_GNU_GETTEXT" $srcdir/configure.ac >/dev/null) && {
  (grep "sed.*POTFILES" $srcdir/configure.ac) > /dev/null || \
  (glib-gettextize --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`glib' installed."
    echo "You can get it from: ftp://ftp.gtk.org/pub/gtk"
    DIE=1
  }
}

(${AUTOMAKE} --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`automake' installed."
  echo "You can get it from: ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
  NO_AUTOMAKE=yes
}


# if no automake, don't bother testing for aclocal
test -n "$NO_AUTOMAKE" || (${ACLOCAL} --version) </dev/null >/dev/null 2>&1 || {
  echo
  echo "**Error**: Missing \`aclocal'.  The version of \`automake'"
  echo "installed doesn't appear recent enough."
  echo "You can get automake from ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
}

if test "$DIE" -eq 1; then
  exit 1
fi

echo "**Message**: I am going to add --enable-maintainer-mode to \`configure'."
echo "If you wish to pass any other to it, please specify them on the"
echo \`$0\'" command line."
echo
conf_flags="--enable-maintainer-mode"

case $CC in
xlc )
  am_opt=--include-deps;;
esac

for coin in `find $srcdir -path $srcdir/CVS -prune -o -name configure.ac -print`
do 
  dr=`dirname $coin`
  if test -f $dr/NO-AUTO-GEN; then
    echo skipping $dr -- flagged as no auto-gen
  else
    echo processing $dr
    ( cd $dr

      aclocalinclude="$ACLOCAL_FLAGS"

      if grep "^AM_GLIB_GNU_GETTEXT" configure.ac >/dev/null; then
	echo "Creating $dr/aclocal.m4 ..."
	test -r $dr/aclocal.m4 || touch $dr/aclocal.m4
	echo "Running glib-gettextize...  Ignore non-fatal messages."
	echo "no" | glib-gettextize --force --copy
	echo "Making $dr/aclocal.m4 writable ..."
	test -r $dr/aclocal.m4 && chmod u+w $dr/aclocal.m4
      fi
      if grep "^AC_PROG_LIBTOOL" configure.ac >/dev/null; then
	if test -z "$NO_LIBTOOLIZE" ; then 
	  echo "Running ${LIBTOOLIZE}..."
	  ${LIBTOOLIZE} --force --copy
	fi
      fi
      echo "Running ${ACLOCAL} $aclocalinclude ..."
      ${ACLOCAL} $aclocalinclude
      if grep "^AM_CONFIG_HEADER" configure.ac >/dev/null; then
	echo "Running autoheader..."
	${AUTOHEADER}
      fi
      echo "Running ${AUTOMAKE} --foreign $am_opt ..."
      ${AUTOMAKE} --add-missing --foreign  --force --copy $am_opt
      echo "Running ${AUTOCONF} ..."
      ${AUTOCONF}
    )
  fi
done

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" ...
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile. || exit 1
else
  echo Skipping configure process.
fi
