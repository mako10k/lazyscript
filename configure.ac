#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([lazyscript],[0.0.1],[mako10k@mk10.org])
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([foreign])
LT_INIT

# Checks for programs.
AC_PROG_CC
AC_PROG_YACC
if test "x$YACC" = "xbison -y"; then
  YACC=bison
else
  AC_MSG_ERROR([bison version 3.0 or later is required])
  exit 1
fi

AC_PROG_LEX([noyywrap])

DX_DOXYGEN_FEATURE(ON)
DX_DOT_FEATURE(ON)
DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(ON)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN([$PACKAGE_NAME],[Doxyfile],[doc])

# Checks for libraries.
PKG_CHECK_MODULES([GC], [bdw-gc])

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([libintl.h malloc.h stdint.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/common/Makefile
                 src/expr/Makefile
                 src/misc/Makefile
                 src/parser/Makefile
                 src/pat/Makefile
                 
                 src/thunk/Makefile
                 src/lstr/Makefile
                 src/plugins/Makefile])

AC_ARG_ENABLE([debug],
  AS_HELP_STRING([--enable-debug], [Enable debugging]),
  [debug=$enableval], [debug=no])

if test "x$debug" = "xyes"; then
    CFLAGS="$CFLAGS -g -O0"
    YFLAGS="$YFLAGS -t"
    AC_DEFINE([DEBUG], [1], [Define to 1 if debugging is enabled])
fi

AC_CONFIG_MACRO_DIRS([m4])

AC_OUTPUT
