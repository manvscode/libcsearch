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
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <libcollections/alloc.h>
#include <libcollections/bheap.h>
#include <libcollections/hash-map.h>
#include <libcollections/tree-map.h>
#include <libcollections/vector.h>
#include "csearch.h"

struct bestfs_node {
	struct bestfs_node* parent;
	int h;
	const void* state;
};

struct bestfs_algorithm {
	heuristic_fxn          heuristic;
	heuristic_comparer_fxn heuristic_compare;
	successors_fxn         successors_of;
	bestfs_node_t*            node_path;

	pbheap_t               open_list; /* list of bestfs_node_t* */
	hash_map_t             open_hash_map; /* (state, bestfs_node_t*) */
	tree_map_t             closed_list; /* (state, bestfs_node_t*) */

	#ifdef _BFS_DEBUG
	size_t allocations;
	#endif
};

static boolean nop_keyval_fxn( void *key, void *value )
{
	return TRUE;
}

static int pointer_compare( const void* p_n1, const void* p_n2 )
{
	ptrdiff_t diff = (unsigned char*) p_n1 - (unsigned char*) p_n2;
	return (int) diff;
}

#define default_heuristic_compare( h1, h2 )  ((h2) - (h1))


static int bestfs_heuristic_compare( const void* p_n1, const void* p_n2 )
{
	return default_heuristic_compare( ((bestfs_node_t*)p_n1)->h, ((bestfs_node_t*)p_n2)->h );
}


	
bestfs_t* bestfs_create( state_hash_fxn state_hasher, heuristic_fxn heuristic, successors_fxn successors_of )
{
	bestfs_t* p_bfs = (bestfs_t*) malloc( sizeof(bestfs_t) );

	if( p_bfs )
	{
		p_bfs->heuristic     = heuristic;
		p_bfs->successors_of = successors_of;
		p_bfs->node_path     = NULL;
		#ifdef _BFS_DEBUG
		p_bfs->allocations   = 0;
		#endif

		pbheap_create( &p_bfs->open_list, 128, 
					  (heap_compare_function) bestfs_heuristic_compare, 
					  malloc, free );

		hash_map_create( &p_bfs->open_hash_map, HASH_MAP_SIZE_MEDIUM, 
						 state_hasher, nop_keyval_fxn, pointer_compare, 
						 malloc, free );

		tree_map_create( &p_bfs->closed_list, nop_keyval_fxn, pointer_compare, malloc, free );
	}

	return p_bfs;
}

void bestfs_destroy( bestfs_t** p_bfs )
{
	if( p_bfs && *p_bfs )
	{
		bestfs_cleanup( *p_bfs );
		pbheap_destroy( &(*p_bfs)->open_list );		
		hash_map_destroy( &(*p_bfs)->open_hash_map );		
		tree_map_destroy( &(*p_bfs)->closed_list );		
		free( *p_bfs );
		*p_bfs = NULL;
	}
}

void bestfs_set_heuristic_fxn( bestfs_t* p_bfs, heuristic_fxn heuristic )
{
	if( p_bfs )
	{
		p_bfs->heuristic = heuristic;
	}
}

void bestfs_set_successors_fxn( bestfs_t* p_bfs, successors_fxn successors_of )
{
	if( p_bfs )
	{
		p_bfs->successors_of = successors_of;
	}
}

/*
 * BFS Algorithm
 * ------------------------------------------------------------------------
 *   Input: The start node and goal nodes.
 *  Output: True if goal node is found, false if goal node cannot be found 
 *          from the start node.
 * ------------------------------------------------------------------------
 * 1.) Set the open list and closed list to be empty.
 * 2.) Add the start node to the open list.
 * 3.) While the open list is not empty, do the following:
 *        a.) Get a node from the open list, call it N.
 *        b.) If N is the goal node, return true.
 *        c.) Get the successor nodes of N.
 *        d.) For each successor node S:
 *            i.) If S is in the closed list, continue.
 *            ii.) If S is in open list:
 *                  - If its heuristic is better, then update its 
 *                    heuristic with the better value and resort 
 *                    the open list.
 *            iii.) If S is not in the open list, then add S to the open list.
 *        e.) Add N to the closed list.
 * 4.) Return false.
 */
boolean bestfs_find( bestfs_t* p_bfs, const void* start, const void* end )
{
	boolean found = FALSE;
	int i;
	pvector_t successors;	
	pvector_create( &successors, 8, malloc, free );

 	/* 1.) Set the open list and closed list to be empty. */
	bestfs_cleanup( p_bfs );

	bestfs_node_t* p_node = (bestfs_node_t*) malloc( sizeof(bestfs_node_t) );
	p_node->parent = NULL;
	p_node->h      = p_bfs->heuristic( start, end );
	p_node->state  = start;
	
	#ifdef _BFS_DEBUG
	p_bfs->allocations++;
	#endif

 	/* 2.) Add the start node to the open list. */
	pbheap_push( &p_bfs->open_list, p_node );
	hash_map_insert( &p_bfs->open_hash_map, p_node->state, p_node );

 	/* 3.) While the open list is not empty, do the following: */
	while( !found && pbheap_size(&p_bfs->open_list) > 0 )
	{
 		/* a.) Get a node from the open list, call it p_current_node. */
		bestfs_node_t* p_current_node = pbheap_peek( &p_bfs->open_list );
		pbheap_pop( &p_bfs->open_list );
		hash_map_remove( &p_bfs->open_hash_map, p_current_node->state );
					
		/* b.) If p_current_node is the goal node, return true. */
		if( p_current_node->state == end )
		{
			p_bfs->node_path = p_current_node;
			found = TRUE;
		}
		else
		{
			/* c.) Get the successor nodes of p_current_node. */
			p_bfs->successors_of( p_current_node->state, &successors ); 

			/* d.) For each successor node S: */
			for( i = 0; i < pvector_size(&successors); i++ )
			{
				const void* successor_state = pvector_get( &successors, i );

				/* i.) If S is not in the closed list, continue. */
				void* found_node;
				if( tree_map_find( &p_bfs->closed_list, successor_state, &found_node ) )
				{
					continue;
				}

				/* ii.) If S is in open list: */
				if( hash_map_find( &p_bfs->open_hash_map, successor_state, &found_node ) )
				{
					bestfs_node_t* p_found_node = (bestfs_node_t*) found_node;
					/* If its heuristic is better, then update its 
					 * heuristic with the better value and resort 
					 * the open list.
					 */
					int h = p_bfs->heuristic( p_found_node->state, end );

					if( default_heuristic_compare( h, p_found_node->h ) < 0 ) // TODO: Expose heuristic compare
					{
						p_found_node->h      = h;
						p_found_node->parent = p_current_node;

						pbheap_reheapify( &p_bfs->open_list );
					}	
				}
				else /* iii.) If S is not in the open list, then add S to the open list. */
				{	
					bestfs_node_t* p_new_node = (bestfs_node_t*) malloc( sizeof(bestfs_node_t) );
					p_new_node->parent     = p_current_node;
					p_new_node->h          = p_bfs->heuristic( successor_state, end );
					p_new_node->state      = successor_state;

					pbheap_push( &p_bfs->open_list, p_new_node );
					hash_map_insert( &p_bfs->open_hash_map, p_new_node->state, p_new_node );
		
					#ifdef _BFS_DEBUG
					p_bfs->allocations++;
					#endif
				}
			}
		}

		/* e.) Add p_current_node to the closed list. */
		tree_map_insert( &p_bfs->closed_list, p_current_node->state, p_current_node );
	}

	return found;
}

void bestfs_cleanup( bestfs_t* p_bfs )
{
	assert( hash_map_size(&p_bfs->open_hash_map) == pbheap_size(&p_bfs->open_list) );
	#ifdef _BFS_DEBUG
	assert( size2 + size3 == p_bfs->allocations );
	#endif
	
	p_bfs->node_path = NULL;

	hash_map_iterator_t open_itr;
	tree_map_iterator_t closed_itr;
	
	hash_map_iterator( &p_bfs->open_hash_map, &open_itr );

	// free everything on the open list.
	while( hash_map_iterator_next( &open_itr ) )
	{
		bestfs_node_t* p_node = hash_map_iterator_value( &open_itr );
		free( p_node );	
		#ifdef _BFS_DEBUG
		p_bfs->allocations--;
		#endif
	}

	// free everything on the closed list.
	for( closed_itr = tree_map_begin( &p_bfs->closed_list );
	     closed_itr != tree_map_end( );
	     closed_itr = tree_map_next( closed_itr ) )
	{
		free( closed_itr->value );	
		#ifdef _BFS_DEBUG
		p_bfs->allocations--;
		#endif
	}

	// empty out the data structures
	pbheap_clear( &p_bfs->open_list );
	hash_map_clear( &p_bfs->open_hash_map );
	tree_map_clear( &p_bfs->closed_list );
}

bestfs_node_t* bestfs_first_node( const bestfs_t* p_bfs )
{
	assert( p_bfs );
	return p_bfs->node_path;
}

const void* bestfs_state( const bestfs_node_t* p_node )
{
	assert( p_node );
	return p_node->state;
}

bestfs_node_t* bestfs_next_node( const bestfs_node_t* p_node )
{
	assert( p_node );
	return p_node->parent;
}
