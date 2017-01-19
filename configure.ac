#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([utf8-validator], [0.1], [simon@monoxid.net], [utf8-validator], [https://github.com/detomon/utf8-validator])

AC_CONFIG_SRCDIR([src/utf8-validator.c])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC

AM_CFLAGS="$CFLAGS"

# Flags
AM_CFLAGS="$AM_CFLAGS -Wall \
	-Wmissing-prototypes -Wmissing-declarations \
	-Wstrict-prototypes -Wpointer-arith \
	-Wwrite-strings -Wcast-qual -Wcast-align \
	-Wbad-function-cast \
	-Wformat-security \
	-Wmissing-format-attribute \
	-Winline"

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	test/Makefile
])

AC_SUBST([AM_CFLAGS])

AC_OUTPUT
