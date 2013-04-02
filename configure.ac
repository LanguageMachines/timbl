#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $Id$
# $URL $

AC_PREREQ(2.61)
AC_INIT([timbl],[6.4.4],[timbl@uvt.nl])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])

if test x"${CXXFLAGS+set}" = xset; then
  # the user set CXXFLAGS; don't override it.
  cxx_flags_were_set=true
else
  cxx_flags_were_set=false
fi
if $cxx_flags_were_set; then
  CXXFLAGS=$CXXFLAGS
fi

# Checks for programs.
AC_PROG_CXX( [g++] )
AC_PROG_RANLIB
# libtool stuff
# unfortunately, a lot of old libtools hang around.
# so LT_INIT and LT_PREREQ cannot be used for now
#LT_PREREQ([2.2.0])
#LT_INIT
AC_PROG_LIBTOOL
AC_SUBST([LIBTOOL_DEPS])

# when running tests, use CXX
AC_LANG([C++])

#checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([floor gettimeofday localtime_r pow rint sqrt strchr])

# ugly hack when PKG_CONFIG_PATH isn't defined.
# couldn't get it to work otherwise
if test "x$PKG_CONFIG_PATH" = x; then
    export PKG_CONFIG_PATH=""
fi

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([XML2], [libxml-2.0 >= 2.6.16] )
CXXFLAGS="$CXXFLAGS $XML2_CFLAGS"
LIBS="$LIBS $XML2_LIBS"

AC_ARG_WITH(ticcutils,
       [  --with-ticcutils=DIR      use libticcutils installed in <DIR>; 
        note that you can install libticcutils in a non-default directory with
        ./configure --prefix=<DIR> in the libticutils installation directory],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"])
AC_MSG_NOTICE( [pkg-config search path: $PKG_CONFIG_PATH] )
PKG_CHECK_MODULES([ticcutils], [ticcutils >= 0.4] )
CXXFLAGS="$CXXFLAGS $ticcutils_CFLAGS"
LIBS="$LIBS $ticcutils_LIBS"


OPENMP_CHECK( [CXXFLAGS="$CXXFLAGS $OPENMP_CXXFLAGS"
	       AC_DEFINE(HAVE_OPENMP, 1 , Define to 1 if you have OpenMP )],
	      [AC_MSG_NOTICE(We don't have OpenMP. Multithreaded testing is disabled)] )

AC_SUBST([OPENMPFLAGS],[$OPENMP_CXXFLAGS])

# for GCC we add -lgomp to LIBS. This makes libtimbl depending on libgomp,
# as we would like to see.
# do we need a solution for other compilers?????
if test "$OPENMPFLAGS" = "-fopenmp"; then
   LIBS="$LIBS -lgomp"
fi

AC_CONFIG_FILES([
  Makefile
  timbl.pc
  m4/Makefile
  src/Makefile
  docs/Makefile
  include/Makefile
  include/timbl/Makefile
  demos/Makefile
])
AC_OUTPUT
