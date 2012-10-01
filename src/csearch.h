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
typedef int     (*compare_fxn)            ( const void* state1, const void* state2 );
typedef int     (*heuristic_fxn)          ( const void* state1, const void* state2 );
typedef int     (*cost_fxn)               ( const void* state1, const void* state2 );
typedef int     (*heuristic_comparer_fxn) ( int h1, int h2 );
typedef void    (*successors_fxn)         ( const void* state, pvector_t* p_successors );


/*
 *  Breadth First Search Algorithm
 *
 *  Breadth First Search is an uninformed search method that tries
 *  to systematically examine every state until it finds the goal
 *  state, or all states have been examined. In other words, it 
 *  exhaustively searches the entire graph or sequence without 
 *  considering the goal until it finds it.
 *  -----------------------------------------------------------
 *  Advantages
 *
 *  - Guaranteed to converge to a solution.
 *  - Easy to understand
 *  -----------------------------------------------------------
 *  Disadvantages
 *
 *  - Most of the time, it will examine more states than needed
 *    at the expense of CPU.
 */
struct breadthfs_algorithm;
typedef struct breadthfs_algorithm breadthfs_t;

struct breadthfs_node;
typedef struct breadthfs_node breadthfs_node_t;

breadthfs_t*      breadthdfs_create             ( compare_fxn compare, successors_fxn successors_of );
void              breadthdfs_destroy            ( breadthfs_t** p_bfs );
void              breadthdfs_set_compare_fxn    ( breadthfs_t* p_bfs, compare_fxn compare );
void              breadthdfs_set_successors_fxn ( breadthfs_t* p_bfs, successors_fxn successors_of );
boolean           breadthdfs_find               ( breadthfs_t* p_bfs, const void* start, const void* end );
void              breadthdfs_cleanup            ( breadthfs_t* p_bfs );
breadthfs_node_t* breadthdfs_first_node         ( const breadthfs_t* p_bfs );
const void*       breadthdfs_state              ( const breadthfs_node_t* p_node );
breadthfs_node_t* breadthdfs_next_node          ( const breadthfs_node_t* p_node );


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
 *  - The algorithm may not converge on a solution when one exists
 *    if a poor heuristic is used.
 *  - The results of this algorithm can appear silly or stupid at 
 *    times.
 */
struct bestfs_algorithm;
typedef struct bestfs_algorithm bestfs_t;

struct bestfs_node;
typedef struct bestfs_node bestfs_node_t;

bestfs_t*      bestfs_create             ( state_hash_fxn state_hasher, heuristic_fxn heuristic, successors_fxn successors_of );
void           bestfs_destroy            ( bestfs_t** p_bfs );
void           bestfs_set_heuristic_fxn  ( bestfs_t* p_bfs, heuristic_fxn heuristic );
void           bestfs_set_successors_fxn ( bestfs_t* p_bfs, successors_fxn successors_of );
boolean        bestfs_find               ( bestfs_t* p_bfs, const void* start, const void* end );
void           bestfs_cleanup            ( bestfs_t* p_bfs );
bestfs_node_t* bestfs_first_node         ( const bestfs_t* p_bfs );
const void*    bestfs_state              ( const bestfs_node_t* p_node );
bestfs_node_t* bestfs_next_node          ( const bestfs_node_t* p_node );


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

dijkstra_t*      dijkstra_create             ( cost_fxn cost, successors_fxn successors_of );
void             dijkstra_destroy            ( dijkstra_t** p_dijkstra );
void             dijkstra_set_cost_fxn       ( dijkstra_t* p_dijkstra, cost_fxn cost );
void             dijkstra_set_successors_fxn ( dijkstra_t* p_dijkstra, successors_fxn successors_of );
boolean          dijkstra_find               ( dijkstra_t* p_dijkstra, const void* start, const void* end );
void             dijkstra_cleanup            ( dijkstra_t* p_dijkstra );
//dijkstra_node_t* dijkstra_first_node         ( const dijkstra_t* p_dijkstra );
//const void*      dijkstra_state              ( const dijkstra_node_t* p_node );
//dijkstra_node_t* dijkstra_next_node          ( const dijkstra_node_t* p_node );


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
