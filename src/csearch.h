/*
 * Copyright (C) 2012 by Joseph A. Marrero and Shrewd LLC. http://www.manvscode.com/
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef _CSEARCH_H_
#define _CSEARCH_H_
#include <libcollections/vector.h>
#ifdef __cplusplus
extern "C" {
#endif 


/*
 *  Function Callbacks
 */
typedef void*   (*alloc_fxn)              ( size_t size );
typedef void    (*free_fxn)               ( void *data );
typedef size_t  (*state_hash_fxn)         ( const void* state );
typedef int     (*heuristic_fxn)          ( const void* state1, const void* state2 );
typedef int     (*cost_fxn)               ( const void* state1, const void* state2 );
typedef int     (*heuristic_comparer_fxn) ( int h1, int h2 );
typedef void    (*successors_fxn)         ( const void* state, pvector_t* p_successors );


/*
 *  Best First Search Algorithm
 *
 *  Best-first search is a method of combinatorial search where 
 *  a heuristic function is used to guide the search toward the 
 *  goal. The heuristic function takes two nodes as input and 
 *  evaluates how likely that node will lead toward the goal.
 *  -----------------------------------------------------------
 *  Special Cases
 *
 *  - Depth-first search is a special case of best-first search 
 *    where the chosen best candidate node is always the first 
 *    (or last) child node.
 *  -----------------------------------------------------------
 *  Advantages
 *
 *  - This algorithm is lightening fast!
 *  - It's faster than Dijkstra and A*.
 *  - Easy to understand
 *  -----------------------------------------------------------
 *  Disadvantages
 *
 *  - The results of this algorithm can appear silly or stupid at 
 *    times.
 */
struct bfs_algorithm;
typedef struct bfs_algorithm bfs_t;

struct bfs_node;
typedef struct bfs_node bfs_node_t;

bfs_t*      bfs_create             ( state_hash_fxn state_hasher, heuristic_fxn heuristic, successors_fxn successors_of );
void        bfs_destroy            ( bfs_t** p_bfs );
void        bfs_set_heuristic_fxn  ( bfs_t* p_bfs, heuristic_fxn heuristic );
void        bfs_set_successors_fxn ( bfs_t* p_bfs, successors_fxn successors_of );
boolean     bfs_find               ( bfs_t* p_bfs, const void* start, const void* end );
void        bfs_cleanup            ( bfs_t* p_bfs );
bfs_node_t* bfs_first_node         ( const bfs_t* p_bfs );
const void* bfs_state              ( const bfs_node_t* p_node );
bfs_node_t* bfs_next_node          ( const bfs_node_t* p_node );


/*
 *  Dijkstra's Algorithm
 *
 *  Dijkstra 's algorithm computes the shortest path between a 
 *  start node and a goal node in a graph.
 *  -----------------------------------------------------------
 *  Advantages
 *
 *  - This algorithm is guaranteed to find a shortest path if 
 *    one exists.
 *  - Produces "smart" looking paths and solutions.
 *  -----------------------------------------------------------
 *  Disadvantages
 *
 *  - Slower than BFS and A*, because it has to visit every node.
 *  - Requires initialization step to set the cost to infinity.  
 *  - If the algorithm needs to be used over and over again, this
 *    is some overhead we can live without.
 */
struct dijkstra_algorithm;
typedef struct dijkstra_algorithm dijkstra_t;

dijkstra_t* dijkstra_create( void );
void        dijkstra_destroy( dijkstra_t** p_dijkstra );


/*
 *  A* Search Algorithm
 *
 *  The A* (pronounced A star) algorithm is essentially Dijkstra's
 *  algorithm and best-first search combined. It will produce the 
 *  shortest path, like Dijkstra's, and avoids visiting unnecessary
 *  nodes, like BFS.
 *  -----------------------------------------------------------
 *  Special Cases
 *
 *  - If the heuristic function evaluates 0 for every node, then 
 *    the algorithm reduces to Dijstra's algorithm without the 
 *    initialization step. In other words, if you want to run 
 *    Dijkstra's algorithm with A*, you will have to manually 
 *    initialize the cost of the nodes to infinity yourself.
 *  - If the cost function evaluates 0 for every node, then the 
 *    algorithm reduces to a best-first search algorithm. In fact
 *    the code would be the same!
 *  -----------------------------------------------------------
 *  Advantages
 *
 *  - Produces "intelligent" looking paths and solutions.
 *  - Faster than Dijkstra's algorithm.
 *  - The initialization step of Dijkstra's algorithm is not 
 *    necessary.
 *  -----------------------------------------------------------
 *  Disadvantages
 *  
 *  - On very large maps, it is still not fast enough. BFS can be
 *    used to find a closer sub-start node that can then be fed 
 *    into the A* algorithm to produce an intelligent path closer
 *    to the goal node.
 */
struct astar_algorithm;
typedef struct astar_algorithm astar_t;

astar_t*    astar_create  ( void );
void        astar_destroy ( astar_t** p_astar );


#ifdef __cplusplus
}
#endif 
#endif /* _CSEARCH_H_ */
