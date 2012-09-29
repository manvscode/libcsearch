AC_INIT([libgsearch], [0.1], [manvscode@gmail.com], [libgsearch], [http://www.manvscode.com/])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AM_SILENT_RULES([yes])
AC_CONFIG_HEADERS([src/libgsearch-config.h])

AC_PROG_CC
AC_PROG_CC_C99
AC_C_INLINE
AC_HEADER_STDC


AH_TOP([
#ifndef _LIBGSEARCH_H_
#define _LIBGSEARCH_H_
])
AH_BOTTOM([
#endif /* _LIBGSEARCH_H_ */
])



dnlCFLAGS=" -Wall -pedantic -g -ggdb -pg -O0 "
CFLAGS=" -Wall -pedantic -O3 "
LDFLAGS=""


LT_INIT([shared static])


AC_ARG_ENABLE([allocators],
    AS_HELP_STRING([--disable-allocators], [Disable exposed allocators]))

AS_IF([test "x$enable_allocators" != "xno"], [
	CFLAGS="$CFLAGS -DUSE_ALLOCATORS"
	AC_DEFINE(USE_ALLOCATORS, 1, [Expose memory allocation and deallocation.])
])



AC_PROG_INSTALL

echo " ------------------------------ "
echo ""
echo "  CFLAGS = $CFLAGS"
echo " LDFLAGS = $LDFLAGS"
echo ""
echo " ------------------------------ "

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	tests/Makefile
])

AC_OUTPUT