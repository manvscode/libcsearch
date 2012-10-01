
# Add new files in alphabetical order. Thanks.
libcsearch_src = astar.c best-first-search.c breadth-first-search.h dijkstra.c heuristics.c

# Add new files in alphabetical order. Thanks.
libcsearch_headers = csearch.h heuristics.h

library_includedir      = $(includedir)/libcsearch/
library_include_HEADERS = $(libcsearch_headers)

# Library
lib_LTLIBRARIES                           = $(top_builddir)/lib/libcsearch.la 
__top_builddir__lib_libcsearch_la_SOURCES = $(libcsearch_src)
__top_builddir__lib_libcsearch_la_LIBADD  = -lcollections

