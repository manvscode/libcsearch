
# Add new files in alphabetical order. Thanks.
libcsearch_src = astar.c \
				 list.c  \
				 successors.c

#best-first-search.c  \
#breadth-first-search.c  \
#depth-first-search.c  \
#dijkstra.c heuristics.c 

# Add new files in alphabetical order. Thanks.
libcsearch_headers = csearch.h heuristics.h

library_includedir      = $(includedir)/@PACKAGE_NAME@/
library_include_HEADERS = $(libcsearch_headers)



# Library
lib_LTLIBRARIES                           = $(top_builddir)/lib/libcsearch.la
__top_builddir__lib_libcsearch_la_SOURCES = $(libcsearch_src)
__top_builddir__lib_libcsearch_la_CFLAGS  = $(COLLECTIONS_CFLAGS)
__top_builddir__lib_libcsearch_la_LIBADD  = -lm $(COLLECTIONS_LIBS)

