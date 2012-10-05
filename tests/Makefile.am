#AM_CFLAGS = -std=c99 -Wall -pedantic -O3 -I$(top_builddir)/src/ -I/usr/local/include/ -L$(top_builddir)/lib/  -L/usr/local/lib
AM_CFLAGS = -std=c99 -pg -g -ggdb -O0 -I$(top_builddir)/src/ -I/usr/local/include/ -L$(top_builddir)/lib/  -L/usr/local/lib
LDADD     = -lcsearch -lcollections -lGL -lglut

bin_PROGRAMS = \
$(top_builddir)/bin/pathfinding 
#$(top_builddir)/bin/test-astar \
#$(top_builddir)/bin/test-dijkstra 

__top_builddir__bin_pathfinding_SOURCES        = pathfinding.c
#__top_builddir__bin_test_astar_SOURCES      = test-astar.c
#__top_builddir__bin_test_dijkstra_SOURCES   = test-dijkstra.c
