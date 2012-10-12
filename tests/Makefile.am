if ENABLE_TESTS
AM_CFLAGS = -std=c11 -pg -g -ggdb -O0 -I$(top_builddir)/src/ -I/usr/local/include/ -L$(top_builddir)/lib/  -L/usr/local/lib

bin_PROGRAMS = \
$(top_builddir)/bin/8-puzzle \
$(top_builddir)/bin/8-puzzle-cpp \
$(top_builddir)/bin/15-puzzle \
$(top_builddir)/bin/pathfinding
#$(top_builddir)/bin/8-puzzle-generic 

__top_builddir__bin_8_puzzle_SOURCES         = 8-puzzle.c
__top_builddir__bin_8_puzzle_LDADD           = $(top_builddir)/lib/.libs/libcsearch.a -lcollections
__top_builddir__bin_8_puzzle_cpp_SOURCES     = 8-puzzle-cpp.cc
__top_builddir__bin_8_puzzle_cpp_LDADD       = $(top_builddir)/lib/.libs/libcsearch.a -lcollections
#__top_builddir__bin_8_puzzle_generic_SOURCES = 8-puzzle-generic.c
#__top_builddir__bin_8_puzzle_generic_LDADD   = $(top_builddir)/lib/.libs/libcsearch.a -lcollections
__top_builddir__bin_15_puzzle_SOURCES        = 15-puzzle.c
__top_builddir__bin_15_puzzle_LDADD          = $(top_builddir)/lib/.libs/libcsearch.a -lcollections
__top_builddir__bin_pathfinding_SOURCES      = pathfinding.c
__top_builddir__bin_pathfinding_LDADD        = $(top_builddir)/lib/.libs/libcsearch.a -lcollections -lglut
__top_builddir__bin_pathfinding_LDFLAGS      = -lcollections -framework OpenGL
endif
