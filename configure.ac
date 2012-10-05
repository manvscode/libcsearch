AC_INIT([libcsearch], [0.1], [manvscode@gmail.com], [libcsearch], [http://www.manvscode.com/])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AM_SILENT_RULES([yes])
AC_CONFIG_HEADERS([src/libcsearch-config.h])

AC_PROG_CC
AC_PROG_CC_C99
AC_C_INLINE
AC_HEADER_STDC


AH_TOP([
#ifndef _LIBCSEARCH_H_
#define _LIBCSEARCH_H_
])
AH_BOTTOM([
#endif /* _LIBCSEARCH_H_ */
])


DFLAGS="-D DEBUG_BREADTH_FIRST_SEARCH -D DEBUG_BEST_FIRST_SEARCH -D DEBUG_DIJKSTRA -D DEBUG_ASTAR"

CFLAGS="-std=c99 -Wall -pedantic -pg -g -ggdb -O0 $DFLAGS -I/usr/local/include"
#CFLAGS="-std=c99 -Wall -pedantic -O2 -I/usr/local/include"
LDFLAGS="-lcollections"


LT_INIT([shared static])
AC_PROG_LIBTOOL
AM_PROG_LIBTOOL



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
