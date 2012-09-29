AM_CFLAGS = -std=gnu99 -Wall -pedantic -O3 -DUSE_ALLOCATORS -I ../src/ -L../lib/ 
LDADD     = -lcollections -lcsearch

bin_PROGRAMS = \
$(top_builddir)/bin/pathfinding 
#$(top_builddir)/bin/test-astar \
#$(top_builddir)/bin/test-dijkstra 

__top_builddir__bin_pathfinding_SOURCES        = pathfinding.c
#__top_builddir__bin_test_astar_SOURCES      = test-astar.c
#__top_builddir__bin_test_dijkstra_SOURCES   = test-dijkstra.c

