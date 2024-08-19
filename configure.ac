#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([lazyscript],[0.0.1],[mako10k@mk10.org])
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([foreign])

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

AC_CONFIG_FILES([Makefile src/Makefile])

AC_OUTPUT
