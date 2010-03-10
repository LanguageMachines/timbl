#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $Id$
# $URL $

AC_PREREQ(2.61)
AC_INIT([Timbl],[6.2.3],[timbl@uvt.nl])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])

# AC_DEFINE_DIR is defined in acinclude.m4
AC_DEFINE_DIR([BIN_PATH], bindir, bindir)
AC_DEFINE_DIR([SYSCONF_PATH], sysconfdir, sysconfdir)
AC_DEFINE_DIR([LOCALSTATE_PATH], localstatedir, localstatedir)

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

if test "$ac_cv_prog_ac_ct_CXX" = "g++"; then
  have_abi64=0
  oldLibs=$LIBS
  oldCXXFLAGS=$CXXFLAGS
  LIBS="-mabi=64 -mips4 $LIBS"
  CXXFLAGS="$CXXFLAGS -mabi=64 -mips4"
  AC_MSG_CHECKING([-mabi=64 compiler flag] )
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[const char hw[] = "Hello World\n";]])],
                 [have_abi64=1])
  if test "$have_abi64" = "0"; then
      LIBS=$oldLibs
      CXXFLAGS=$oldCXXFLAGS
      AC_MSG_RESULT( [No] )
  else
      AC_MSG_RESULT( [Ok] )
  fi
  CXXFLAGS="$CXXFLAGS -W -Wall"
else
  CXXFLAGS="$CXXFLAGS -W"
fi

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([XML2], [libxml-2.0 >= 2.6.16] )
CXXFLAGS="$CXXFLAGS $XML2_CFLAGS"
LIBS="$LIBS $XML2_LIBS"

# Checks for header files.
AC_CHECK_HEADERS([sys/time.h sys/timeb.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([floor gettimeofday localtime_r pow rint sqrt strchr])

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
