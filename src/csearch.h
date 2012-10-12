/*
 * Copyright (C) 2012 Joseph A. Marrero.  http://www.manvscode.com/
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
#include <stddef.h>
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#include <stdbool.h>
#ifdef __restrict
#undef __restrict
#define __restrict restrict
#endif
#ifdef __inline
#undef __inline
#define __inline inline
#endif
#else
#define bool int
#define true 1
#define false 0
#ifdef __restrict
#undef __restrict
#define __restrict
#endif
#ifdef __inline
#undef __inline
#define __inline
#endif
#endif
#ifdef __cplusplus
extern "C" {
#endif 

/*
 *  Successor Collection 
 */
struct successors;
typedef struct successors successors_t;

bool successors_push   ( successors_t* __restrict p_successors, const void* __restrict state );
bool successors_pop    ( successors_t* p_successors );
bool successors_resize ( successors_t* p_successors, size_t new_size );
void successors_clear  ( successors_t* p_successors );


/*
 *  Function Callbacks
 */
typedef void*        (*alloc_fxn)              ( size_t size );
typedef void         (*free_fxn)               ( void *data );
typedef size_t       (*state_hash_fxn)         ( const void* __restrict state );
typedef int          (*compare_fxn)            ( const void* __restrict state1, const void* __restrict state2 );
typedef int          (*heuristic_fxn)          ( const void* __restrict state1, const void* __restrict state2 );
typedef int          (*cost_fxn)               ( const void* __restrict state1, const void* __restrict state2 );
typedef unsigned int (*nonnegative_cost_fxn)   ( const void* __restrict state1, const void* __restrict state2 );
typedef int          (*heuristic_comparer_fxn) ( int h1, int h2 );
typedef void         (*successors_fxn)         ( const void* __restrict state, successors_t* __restrict p_successors );

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

breadthfs_t*      breadthfs_create             ( compare_fxn compare, state_hash_fxn state_hasher, successors_fxn successors_of, alloc_fxn alloc, free_fxn free );
void              breadthfs_destroy            ( breadthfs_t** p_bfs );
void              breadthfs_set_compare_fxn    ( breadthfs_t* p_bfs, compare_fxn compare );
void              breadthfs_set_successors_fxn ( breadthfs_t* p_bfs, successors_fxn successors_of );
bool              breadthfs_find               ( breadthfs_t* __restrict p_bfs, const void* __restrict start, const void* __restrict end );
void              breadthfs_cleanup            ( breadthfs_t* p_bfs );
breadthfs_node_t* breadthfs_first_node         ( const breadthfs_t* p_bfs );
const void*       breadthfs_state              ( const breadthfs_node_t* p_node );
breadthfs_node_t* breadthfs_next_node          ( const breadthfs_node_t* p_node );
void              breadthfs_iterative_init     ( breadthfs_t* __restrict p_bfs, const void* __restrict start, const void* __restrict end, bool* __restrict found );
void              breadthfs_iterative_find     ( breadthfs_t* __restrict p_bfs, const void* __restrict start, const void* __restrict end, bool* __restrict found );
bool              breadthfs_iterative_is_done  ( breadthfs_t* __restrict p_bfs, bool* __restrict found );


/*
 *  Depth First Search Algorithm
 *
 *  Depth First Search is an uninformed search method that explores
 *  as far as possible along each branch before backtracking.
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
struct depthfs_algorithm;
typedef struct depthfs_algorithm depthfs_t;

struct depthfs_node;
typedef struct depthfs_node depthfs_node_t;

depthfs_t*      depthfs_create             ( compare_fxn compare, state_hash_fxn state_hasher, successors_fxn successors_of, alloc_fxn alloc, free_fxn free );
void            depthfs_destroy            ( depthfs_t** p_bfs );
void            depthfs_set_compare_fxn    ( depthfs_t* p_bfs, compare_fxn compare );
void            depthfs_set_successors_fxn ( depthfs_t* p_bfs, successors_fxn successors_of );
bool            depthfs_find               ( depthfs_t* __restrict p_bfs, const void* __restrict start, const void* __restrict end );
void            depthfs_cleanup            ( depthfs_t* p_bfs );
depthfs_node_t* depthfs_first_node         ( const depthfs_t* p_bfs );
const void*     depthfs_state              ( const depthfs_node_t* p_node );
depthfs_node_t* depthfs_next_node          ( const depthfs_node_t* p_node );
void            depthfs_iterative_init     ( depthfs_t* __restrict p_bfs, const void* __restrict start, const void* __restrict end, bool* found );
void            depthfs_iterative_find     ( depthfs_t* __restrict p_bfs, const void* __restrict start, const void* __restrict end, bool* found );
bool            depthfs_iterative_is_done  ( depthfs_t* __restrict p_bfs, bool* found );


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

bestfs_t*      bestfs_create             ( compare_fxn compare, state_hash_fxn state_hasher, heuristic_fxn heuristic, successors_fxn successors_of, alloc_fxn alloc, free_fxn free );
void           bestfs_destroy            ( bestfs_t** p_best );
void           bestfs_set_compare_fxn    ( bestfs_t* p_best, compare_fxn compare );
void           bestfs_set_heuristic_fxn  ( bestfs_t* p_best, heuristic_fxn heuristic );
void           bestfs_set_successors_fxn ( bestfs_t* p_best, successors_fxn successors_of );
bool           bestfs_find               ( bestfs_t* __restrict p_best, const void* __restrict start, const void* __restrict end );
void           bestfs_cleanup            ( bestfs_t* p_best );
bestfs_node_t* bestfs_first_node         ( const bestfs_t* p_best );
const void*    bestfs_state              ( const bestfs_node_t* p_node );
bestfs_node_t* bestfs_next_node          ( const bestfs_node_t* p_node );
void           bestfs_iterative_init     ( bestfs_t* __restrict p_best, const void* __restrict start, const void* __restrict end, bool* __restrict found );
void           bestfs_iterative_find     ( bestfs_t* __restrict p_best, const void* __restrict start, const void* __restrict end, bool* __restrict found );
bool           bestfs_iterative_is_done  ( bestfs_t* __restrict p_best, bool* __restrict found );


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

struct dijkstra_node;
typedef struct dijkstra_node dijkstra_node_t;

dijkstra_t*      dijkstra_create             ( compare_fxn compare, state_hash_fxn state_hasher, nonnegative_cost_fxn cost, successors_fxn successors_of, alloc_fxn alloc, free_fxn free );
void             dijkstra_destroy            ( dijkstra_t** p_dijkstra );
void             dijkstra_set_compare_fxn    ( dijkstra_t* p_dijkstra, compare_fxn compare );
void             dijkstra_set_cost_fxn       ( dijkstra_t* p_dijkstra, nonnegative_cost_fxn cost );
void             dijkstra_set_successors_fxn ( dijkstra_t* p_dijkstra, successors_fxn successors_of );
bool             dijkstra_find               ( dijkstra_t* __restrict p_dijkstra, const void* __restrict start, const void* __restrict end );
void             dijkstra_cleanup            ( dijkstra_t* p_dijkstra );
dijkstra_node_t* dijkstra_first_node         ( const dijkstra_t* p_dijkstra );
const void*      dijkstra_state              ( const dijkstra_node_t* p_node );
dijkstra_node_t* dijkstra_next_node          ( const dijkstra_node_t* p_node );
void             dijkstra_iterative_init     ( dijkstra_t* __restrict p_dijkstra, const void* __restrict start, const void* __restrict end, bool* found );
void             dijkstra_iterative_find     ( dijkstra_t* __restrict p_dijkstra, const void* __restrict start, const void* __restrict end, bool* found );
bool             dijkstra_iterative_is_done  ( dijkstra_t* __restrict p_dijkstra, bool* found );


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

struct astar_node;
typedef struct astar_node astar_node_t;

astar_t*      astar_create             ( compare_fxn compare, state_hash_fxn state_hasher, heuristic_fxn heuristic, cost_fxn cost, successors_fxn successors_of, alloc_fxn alloc, free_fxn free );
void          astar_destroy            ( astar_t** p_astar );
void          astar_set_compare_fxn    ( astar_t* p_astar, compare_fxn compare );
void          astar_set_heuristic_fxn  ( astar_t* p_astar, heuristic_fxn heuristic );
void          astar_set_cost_fxn       ( astar_t* p_astar, cost_fxn cost );
void          astar_set_successors_fxn ( astar_t* p_astar, successors_fxn successors_of );
bool          astar_find               ( astar_t* __restrict p_astar, const void* __restrict start, const void* __restrict end );
void          astar_cleanup            ( astar_t* p_astar );
astar_node_t* astar_first_node         ( const astar_t* p_astar );
const void*   astar_state              ( const astar_node_t* p_node );
astar_node_t* astar_next_node          ( const astar_node_t* p_node );
void          astar_iterative_init     ( astar_t* __restrict p_astar, const void* __restrict start, const void* __restrict end, bool* found );
void          astar_iterative_find     ( astar_t* __restrict p_astar, const void* __restrict start, const void* __restrict end, bool* found );
bool          astar_iterative_is_done  ( astar_t* __restrict p_astar, bool* found );


/*
 * Generic csearch functions
 */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define csearch_destroy( X ) _Generic( (X), \
	breadthfs_t**: breadthfs_destroy, \
	depthfs_t**: depthfs_destroy, \
	bestfs_t**: bestfs_destroy, \
	dijkstra_t**: dijkstra_destroy, \
	astar_t**: astar_destroy \
	)( X )
#define csearch_set_compare_fxn( X, compare_fxn ) _Generic( (X), \
	breadthfs_t*: breadthfs_set_compare_fxn, \
	depthfs_t*: depthfs_set_compare_fxn, \
	bestfs_t*: bestfs_set_compare_fxn, \
	dijkstra_t*: dijkstra_set_compare_fxn, \
	astar_t*: astar_set_compare_fxn \
	)( X, compare_fxn )
#define csearch_set_successors_fxn( X, successors_fxn ) _Generic( (X), \
	breadthfs_t*: breadthfs_set_successors_fxn, \
	depthfs_t*: depthfs_set_successors_fxn, \
	bestfs_t*: bestfs_set_successors_fxn, \
	dijkstra_t*: dijkstra_set_successors_fxn, \
	astar_t*: astar_set_successors_fxn \
	)( X, successors_fxn )
#define csearch_set_heuristic_fxn( X, heuristic_fxn ) _Generic( (X), \
	bestfs_t*: bestfs_set_heuristic_fxn, \
	astar_t*: astar_set_heuristic_fxn \
	)( X, heuristic_fxn )
#define csearch_set_cost_fxn( X, cost_fxn ) _Generic( (X), \
	dijkstra_t*: dijkstra_set_cost_fxn, \
	astar_t*: astar_set_cost_fxn \
	)( X, cost_fxn )
#define csearch_find( X, start, end ) _Generic( (X), \
	breadthfs_t*: breadthfs_find, \
	depthfs_t*: depthfs_find, \
	bestfs_t*: bestfs_find, \
	dijkstra_t*: dijkstra_find, \
	astar_t*: astar_find \
	)( X, start, end )
#define csearch_cleanup( X ) _Generic( (X), \
	breadthfs_t*: breadthfs_cleanup, \
	depthfs_t*: depthfs_cleanup, \
	bestfs_t*: bestfs_cleanup, \
	dijkstra_t*: dijkstra_cleanup, \
	astar_t*: astar_cleanup \
	)( X )
#define csearch_first_node( X ) _Generic( (X), \
	breadthfs_t*: breadthfs_first_node, \
	depthfs_t*: depthfs_first_node, \
	bestfs_t*: bestfs_first_node, \
	dijkstra_t*: dijkstra_first_node, \
	astar_t*: astar_first_node \
	)( X )
#define csearch_state( X ) _Generic( (X), \
	breadthfs_node_t*: breadthfs_state, \
	depthfs_node_t*: depthfs_state, \
	bestfs_node_t*: bestfs_state, \
	dijkstra_node_t*: dijkstra_state, \
	astar_node_t*: astar_state \
	)( X )
#define csearch_next_node( X ) _Generic( (X), \
	breadthfs_node_t*: breadthfs_next_node, \
	depthfs_node_t*: depthfs_next_node, \
	bestfs_node_t*: bestfs_next_node, \
	dijkstra_node_t*: dijkstra_next_node, \
	astar_node_t*: astar_next_node \
	)( X )
#define csearch_iterative_init( X, start, end, found ) _Generic( (X), \
	breadthfs_t*: breadthfs_iterative_init, \
	depthfs_t*: depthfs_iterative_init, \
	bestfs_t*: bestfs_iterative_init, \
	dijkstra_t*: dijkstra_iterative_init, \
	astar_t*: astar_iterative_init \
	)( X, start, end, found )
#define csearch_iterative_find( X, start, end, found ) _Generic( (X), \
	breadthfs_t*: breadthfs_iterative_find, \
	depthfs_t*: depthfs_iterative_find, \
	bestfs_t*: bestfs_iterative_find, \
	dijkstra_t*: dijkstra_iterative_find, \
	astar_t*: astar_iterative_find \
	)( X, start, end, found )
#define csearch_iterative_is_done( X, found ) _Generic( (X), \
	breadthfs_t*: breadthfs_iterative_is_done, \
	depthfs_t*: depthfs_iterative_is_done, \
	bestfs_t*: bestfs_iterative_is_done, \
	dijkstra_t*: dijkstra_iterative_is_done, \
	astar_t*: astar_iterative_is_done \
	)( X, found )
#endif



#ifdef __cplusplus
} /* extern C Linkage */
namespace csearch {
	using ::successors_t;
	using ::successors_push;
	using ::successors_pop;
	using ::successors_resize;
	using ::successors_clear;
	using ::alloc_fxn;
	using ::free_fxn;
	using ::state_hash_fxn;
	using ::compare_fxn;
	using ::heuristic_fxn;
	using ::cost_fxn;
	using ::nonnegative_cost_fxn;
	using ::heuristic_comparer_fxn;
	using ::successors_fxn;
	using ::breadthfs_t;
	using ::breadthfs_node_t;
	using ::breadthfs_create;
	using ::breadthfs_destroy;
	using ::breadthfs_set_compare_fxn;
	using ::breadthfs_set_successors_fxn;
	using ::breadthfs_find;
	using ::breadthfs_cleanup;
	using ::breadthfs_first_node;
	using ::breadthfs_state;
	using ::breadthfs_next_node;
	using ::breadthfs_iterative_init;
	using ::breadthfs_iterative_find;
	using ::breadthfs_iterative_is_done;
	using ::depthfs_t;
	using ::depthfs_node_t;
	using ::depthfs_create;
	using ::depthfs_destroy;
	using ::depthfs_set_compare_fxn;
	using ::depthfs_set_successors_fxn;
	using ::depthfs_find;
	using ::depthfs_cleanup;
	using ::depthfs_first_node;
	using ::depthfs_state;
	using ::depthfs_next_node;
	using ::depthfs_iterative_init;
	using ::depthfs_iterative_find;
	using ::depthfs_iterative_is_done;
	using ::bestfs_t;
	using ::bestfs_node_t;
	using ::bestfs_create;
	using ::bestfs_destroy;
	using ::bestfs_set_compare_fxn;
	using ::bestfs_set_heuristic_fxn;
	using ::bestfs_set_successors_fxn;
	using ::bestfs_find;
	using ::bestfs_cleanup;
	using ::bestfs_first_node;
	using ::bestfs_state;
	using ::bestfs_next_node;
	using ::bestfs_iterative_init;
	using ::bestfs_iterative_find;
	using ::bestfs_iterative_is_done;
	using ::dijkstra_t;
	using ::dijkstra_node_t;
	using ::dijkstra_create;
	using ::dijkstra_destroy;
	using ::dijkstra_set_compare_fxn;
	using ::dijkstra_set_cost_fxn;
	using ::dijkstra_set_successors_fxn;
	using ::dijkstra_find;
	using ::dijkstra_cleanup;
	using ::dijkstra_first_node;
	using ::dijkstra_state;
	using ::dijkstra_next_node;
	using ::dijkstra_iterative_init;
	using ::dijkstra_iterative_find;
	using ::dijkstra_iterative_is_done;
	using ::astar_t;
	using ::astar_node_t;
	using ::astar_create;
	using ::astar_destroy;
	using ::astar_set_compare_fxn;
	using ::astar_set_heuristic_fxn;
	using ::astar_set_cost_fxn;
	using ::astar_set_successors_fxn;
	using ::astar_find;
	using ::astar_cleanup;
	using ::astar_first_node;
	using ::astar_state;
	using ::astar_next_node;
	using ::astar_iterative_init;
	using ::astar_iterative_find;
	using ::astar_iterative_is_done;
} /* namespace csearch */
#endif 
#endif /* _CSEARCH_H_ */
