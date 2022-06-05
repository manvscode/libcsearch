AC_PREREQ(2.67)
AC_INIT([libcsearch], [1.0], [manvscode@gmail.com], [libcsearch], [http://www.manvscode.com/])
AM_INIT_AUTOMAKE([1.13 foreign silent-rules -Wall -Werror])

AM_SILENT_RULES([yes])
AC_CONFIG_HEADERS([src/libcsearch-config.h])

AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_CXX
AC_C_INLINE
AC_HEADER_STDC


AH_TOP([
#ifndef _LIBCSEARCH_H_
#define _LIBCSEARCH_H_
])
AH_BOTTOM([
#endif /* _LIBCSEARCH_H_ */
])


#AC_CHECK_LIB([collections], [lc_hash_map_create],
	#[have_collections=yes],
	#[AC_MSG_ERROR([Missing libcollections library (i.e. -lcollections).])]
#)


AM_PROG_AR
LT_INIT([shared static])
AC_PROG_LIBTOOL
AM_PROG_LIBTOOL
AC_PROG_INSTALL

AC_ARG_ENABLE([examples],
	[AS_HELP_STRING([--disable-examples], [Enable example programs.])],
	[:],
	[enable_examples=no])

AM_CONDITIONAL([ENABLE_EXAMPLES], [test "$enable_examples" = "yes"])


PKG_CHECK_MODULES([COLLECTIONS], [collections-static],
	[AC_MSG_NOTICE([Found libcollections])],
	[AC_MSG_ERROR([Missing libcollections])],
)


AC_CONFIG_FILES([
	Makefile
	src/Makefile
	examples/Makefile
])

AC_OUTPUT
