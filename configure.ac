#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $Id$
# $URL: $

AC_PREREQ(2.59)
AC_INIT([Timbl], [6.1.4], [timbl@uvt.nl])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([.])
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

# Checks for programs.
# when available, we prefer the Intel Compiler
AC_PROG_CXX( [icpc g++] )

if $cxx_flags_were_set; then
  CXXFLAGS=$CXXFLAGS
fi


# when running tests, use CXX
AC_LANG([C++])

if test "$ac_cv_prog_ac_ct_CXX" == "g++"; then
  have_abi64=0
  oldLibs=$LIBS
  oldCXXFLAGS=$CXXFLAGS
  LIBS="-mabi=64 -mips4 $LIBS"
  CXXFLAGS="$CXXFLAGS -mabi=64 -mips4"
  AC_MSG_CHECKING([-mabi=64 compiler flag] )
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[const char hw[] = "Hello World\n";]])],
                 [have_abi64=1])
  if test "$have_abi64" == "0"; then
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

# libtool stuff
AC_PROG_LIBTOOL
AC_SUBST([LIBTOOL_DEPS])

# Checks for header files.
AC_CHECK_HEADERS([unistd.h sys/types.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([getaddrinfo])

dnl
dnl ----- Sockets
checkBoth=0
AC_CHECK_FUNC(connect)
if test "$ac_cv_func_connect" = "no"; then
    AC_CHECK_LIB(socket, main, LIBS="$LIBS -lsocket", checkBoth=1)
fi
if test "$checkBoth" = "1"; then
    oldLibs=$LIBS
    LIBS="$LIBS -lsocket -lnsl"
    AC_CHECK_FUNC(accept, , [LIBS=$oldLibs])
fi
AC_CHECK_FUNC(gethostbyname, ,[AC_CHECK_LIB(nsl, main, [LIBS="$LIBS -lnsl"])])

dnl ------ socklen_t
dnl We check for socklen_t by making prototypes with the
dnl various types. First socklen_t, then size_t, finally int.
dnl If the prototype succeeds, we are probably safe.
dnl That works if accept is not preprocessor defined (such sa AIX)
AC_MSG_CHECKING([for socklen_t])
AC_CACHE_VAL(ac_cv_check_socklen_t,[ac_cv_check_socklen_t=''
        AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
#ifdef __cplusplus
extern "C" {
#endif
#define try 1
#ifdef AIX
#if AIX >= 51
#define try 0
#endif
#endif
#if try
extern int accept(int, struct sockaddr *, socklen_t *);
#endif
#ifdef __cplusplus
}
#endif
        ],,[ac_cv_check_socklen_t=socklen_t],[
                AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
#ifdef __cplusplus
extern "C" {
#endif
#define try 1
#ifdef AIX
#if AIX >= 42
#define try 0
#endif
#endif
#if try
extern int accept(int, struct sockaddr *, size_t t *);
#endif
#ifdef __cplusplus
}
#endif
],,[ac_cv_check_socklen_t=size_t],[ac_cv_check_socklen_t=int])
                ])
        ])
AC_MSG_RESULT($ac_cv_check_socklen_t)
AC_DEFINE_UNQUOTED(TIMBL_SOCKLEN_T,$ac_cv_check_socklen_t, "sock_len" )

dnl ------ server mode
AC_ARG_ENABLE(server,[  --enable-server[=PREFIX]  enable server mode])
if test "$enable_server" -a "$enable_server" == "no"; then
AC_MSG_WARN([not building servermode] )
else
    if test "$enable_server" != "no"; then
        LIBS="$LIBS -lpthread"
        CXXFLAGS="$CXXFLAGS -DPTHREADS -DUSE_LOGSTREAMS"
    fi
fi
dnl

AC_OUTPUT([
  Makefile
  timbl.pc
  src/Makefile
  docs/Makefile
  include/Makefile
  include/timbl/Makefile
  init/Makefile
  init/timbl
  demos/Makefile
])
