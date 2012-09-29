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
#ifndef _GSEARCH_H_
#define _GSEARCH_H_
#ifdef __cplusplus
extern "C" {
#endif 

#include <libcollections/vector.h>

typedef void*   (*alloc_fxn)       ( size_t size );
typedef void    (*free_fxn)        ( void *data );
typedef size_t  (*state_hash_fxn)  ( const void* state );
typedef int     (*heuristic_fxn)   ( const void* state1, const void* state2 );
typedef void    (*successors_fxn)  ( const void* state, /*out*/ vector_t* p_successors );


/*
 *  Best First Search Algorithm
 *
 *  Best-first search is a method of combinatorial search where 
 *  a heuristic function is used to guide the search toward the 
 *  goal. The heuristic function takes two nodes as input and 
 *  evaluates how likely that node will lead toward the goal.
 */
struct bfs_algorithm;
typedef struct bfs_algorithm bfs_t;

struct bfs_node;
typedef struct bfs_node bfs_node_t;


bfs_t*      bfs_create     ( state_hash_fxn state_hasher, heuristic_fxn heuristic, successors_fxn successors_of );
void        bfs_destroy    ( bfs_t** p_bfs );
boolean     bfs_find       ( bfs_t* p_bfs, const void* start, const void* end );
void        bfs_cleanup    ( bfs_t* p_bfs );
bfs_node_t* bfs_first_node ( const bfs_t* p_bfs );
void*       bfs_state      ( const bfs_node_t* p_node );
bfs_node_t* bfs_next_node  ( const bfs_node_t* p_node );

/*
 *  Dijkstra's Algorithm
 */
struct dijkstra_algorithm;
typedef struct dijkstra_algorithm dijkstra_t;

dijkstra_t* dijkstra_create( void );
void        dijkstra_destroy( dijkstra_t** p_dijkstra );

/*
 *  A* Search Algorithm
 */
struct astar_algorithm;
typedef struct astar_algorithm astar_t;

astar_t*    astar_create  ( void );
void        astar_destroy ( astar_t** p_astar );

#ifdef __cplusplus
}
#endif 
#endif /* _GSEARCH_H_ */
