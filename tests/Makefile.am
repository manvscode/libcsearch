#AM_CFLAGS = -std=c99 -Wall -pedantic -O3 -I$(top_builddir)/src/ -I/usr/local/include/ -L$(top_builddir)/lib/  -L/usr/local/lib
AM_CFLAGS = -std=c99 -pg -g -ggdb -O0 -I$(top_builddir)/src/ -I/usr/local/include/ -L$(top_builddir)/lib/  -L/usr/local/lib
LDADD     = -lcsearch -lcollections -lGL -lglut

bin_PROGRAMS = \
$(top_builddir)/bin/pathfinding \
$(top_builddir)/bin/8-puzzle 

__top_builddir__bin_pathfinding_SOURCES        = pathfinding.c
__top_builddir__bin_8_puzzle_SOURCES           = 8-puzzle.c
