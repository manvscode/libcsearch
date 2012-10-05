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
#include <libcollections/bench-mark.h>
#include "successors-private.h"
#include "csearch.h"

struct astar_node {
	struct astar_node* parent;
	int h; /* heuristic */
	int g; /* cost */
	int f; /* heuristic + cost */
	const void* state; /* vertex */
};

struct astar_algorithm {
	heuristic_fxn          heuristic;
	cost_fxn               cost;
	successors_fxn         successors_of;
	astar_node_t*          node_path;

	pbheap_t               open_list; /* list of astar_node_t* */
	hash_map_t             open_hash_map; /* (state, astar_node_t*) */
	tree_map_t             closed_list; /* (state, astar_node_t*) */

	#ifdef DEBUG_ASTAR
	size_t       allocations;
	bench_mark_t bm;
	#endif
};


static boolean nop_keyval_fxn( void* restrict key, void* restrict value )
{
	return TRUE;
}

static int astar_pointer_compare( const void* restrict p_n1, const void* restrict p_n2 )
{
	ptrdiff_t diff = (size_t*) p_n1 - (size_t*) p_n2;
	return (int) diff;
}

#define default_f_compare( f1, f2 )      ((f2) - (f1))

static int best_f_compare( const void* restrict p_n1, const void* restrict p_n2 )
{
	return default_f_compare( ((astar_node_t*)p_n1)->f, ((astar_node_t*)p_n2)->f );
}


astar_t* astar_create( state_hash_fxn state_hasher, heuristic_fxn heuristic, cost_fxn cost, successors_fxn successors_of )
{
	astar_t* p_astar = (astar_t*) malloc( sizeof(astar_t) );

	if( p_astar )
	{
		p_astar->heuristic     = heuristic;
		p_astar->cost          = cost;
		p_astar->successors_of = successors_of;
		p_astar->node_path     = NULL;
		#ifdef DEBUG_ASTAR
		p_astar->allocations   = 0;
		p_astar->bm            = bench_mark_create( "A* Search Algorithm" );
		#endif

		pbheap_create( &p_astar->open_list, 128, 
					  (heap_compare_function) best_f_compare, 
					  malloc, free );

		hash_map_create( &p_astar->open_hash_map, HASH_MAP_SIZE_MEDIUM, 
						 state_hasher, nop_keyval_fxn, astar_pointer_compare, 
						 malloc, free );

		tree_map_create( &p_astar->closed_list, nop_keyval_fxn, astar_pointer_compare, malloc, free );
	}

	return p_astar;
}

void astar_destroy( astar_t** p_astar )
{
	if( p_astar && *p_astar )
	{
		#ifdef DEBUG_ASTAR
		bench_mark_destroy( (*p_astar)->bm );
		#endif

		astar_cleanup( *p_astar );
		pbheap_destroy( &(*p_astar)->open_list );		
		hash_map_destroy( &(*p_astar)->open_hash_map );		
		tree_map_destroy( &(*p_astar)->closed_list );		
		free( *p_astar );
		*p_astar = NULL;

	}
}

void astar_set_heuristic_fxn( astar_t* p_astar, heuristic_fxn heuristic )
{
	if( p_astar )
	{
		assert( heuristic );
		p_astar->heuristic = heuristic;
	}
}

void astar_set_cost_fxn( astar_t* p_astar, cost_fxn cost )
{
	if( p_astar )
	{
		assert( cost );
		p_astar->cost = cost;
	}
}

void astar_set_successors_fxn( astar_t* p_astar, successors_fxn successors_of )
{
	if( p_astar )
	{
		assert( successors_of );
		p_astar->successors_of = successors_of;
	}
}

/*
 * A* Search Algorithm
 * ------------------------------------------------------------------------
 *   Input: The start node and goal nodes.
 *  Output: True if goal node is found, false if goal node cannot be found 
 *          from the start node.
 * ------------------------------------------------------------------------
 * 1.) Set the open list and closed list to be empty.
 * 2.) Add the start node to the open list.
 * 3.) While the open list is not empty, do the following:
 *    a.) Get a node from the open list, call it N.
 *    b.) If N is the goal node, return true.
 *    c.) Get the successor nodes of N.
 *    d.) For each successor node S:
 *          i.) If S is in the closed list:
 *               - If its F-value is better, then update its 
 *                 F-value with the better value and move it from the
 *                 closed list to the open list (this case handles negative
 *                 weights).
 *         ii.) If S is in open list:
 *               - If its F-value is better, then update its 
 *                 F-value with the better value and resort the open list.
 *               - Otherwise, continue (do not add S to the open list).
 *         iii.) If S is not in the open list, then add S to the open list.
 *    e.) Add N to the closed list.
 * 4.) Return false.
 */
boolean astar_find( astar_t* restrict p_astar, const void* restrict start, const void* restrict end )
{
	#ifdef DEBUG_ASTAR
	bench_mark_start( p_astar->bm );
	#endif
	boolean found = FALSE;
	int i;
	successors_t successors;	
	successors_create( &successors, 8, malloc, free );

 	/* 1.) Set the open list and closed list to be empty. */
	astar_cleanup( p_astar );

	astar_node_t* p_node = (astar_node_t*) malloc( sizeof(astar_node_t) );
	p_node->parent = NULL;
	p_node->h      = p_astar->heuristic( start, end );
	p_node->g      = 0 /* no cost */;
	p_node->f      = p_node->g + p_node->h;
	p_node->state  = start;
	
	#ifdef DEBUG_ASTAR
	p_astar->allocations++;
	#endif

 	/* 2.) Add the start node to the open list. */
	pbheap_push( &p_astar->open_list, p_node );
	hash_map_insert( &p_astar->open_hash_map, p_node->state, p_node );

 	/* 3.) While the open list is not empty, do the following: */
	while( !found && pbheap_size(&p_astar->open_list) > 0 )
	{
 		/* a.) Get a node from the open list, call it p_current_node. */
		astar_node_t* p_current_node = pbheap_peek( &p_astar->open_list );
		pbheap_pop( &p_astar->open_list );
		hash_map_remove( &p_astar->open_hash_map, p_current_node->state );
					
		/* b.) If p_current_node is the goal node, return true. */
		if( p_current_node->state == end )
		{
			p_astar->node_path = p_current_node;
			found = TRUE;
		}
		else
		{
			/* c.) Get the successor nodes of p_current_node. */
			p_astar->successors_of( p_current_node->state, &successors ); 

			/* d.) For each successor node S: */
			for( i = 0; i < successors_size(&successors); i++ )
			{
				const void* successor_state = successors_get( &successors, i );

				/* i.) If S is in the closed list: */
				void* found_node;
				if( tree_map_find( &p_astar->closed_list, successor_state, &found_node ) )
				{
					#if 1
					continue;
					#else
					astar_node_t* p_found_node = (astar_node_t*) found_node;
					 /* If its F-value is better, then update its F-value with the 
 					  * better value and move it from the closed list to the open 
 					  * list (this case handles negative weights).
					  */
					int h = p_astar->heuristic( p_found_node->state, end );
					int g = p_current_node->g + p_astar->cost( p_current_node->state, successor_state );
					int f = g + h;

					if( default_f_compare( f, p_found_node->f ) < 0 ) 
					{
						p_found_node->h      = h;
						p_found_node->g      = g;
						p_found_node->f      = f;
						p_found_node->parent = p_current_node;

						tree_map_remove( &p_astar->closed_list, successor_state );

						pbheap_push( &p_astar->open_list, p_found_node );
						hash_map_insert( &p_astar->open_hash_map, p_found_node->state, p_found_node );
					}
					#endif
				}

				/* ii.) If S is in open list: */
				if( hash_map_find( &p_astar->open_hash_map, successor_state, &found_node ) )
				{
					astar_node_t* p_found_node = (astar_node_t*) found_node;
					/* If its F-value is better, then update its 
					 * F-value with the better value and resort the open list.
					 */
					int h = p_astar->heuristic( p_found_node->state, end );
					int g = p_current_node->g + p_astar->cost( p_current_node->state, successor_state );
					int f = g + h;

					if( default_f_compare( f, p_found_node->f ) < 0 ) 
					{
						p_found_node->h      = h;
						p_found_node->g      = g;
						p_found_node->f      = f;
						p_found_node->parent = p_current_node;

						pbheap_reheapify( &p_astar->open_list );
					}	
				}
				else /* iii.) If S is not in the open list, then add S to the open list. */
				{	
					int h = p_astar->heuristic( successor_state, end );
					int g = p_current_node->g + p_astar->cost( p_current_node->state, successor_state );
					int f = g + h;

					astar_node_t* p_new_node = (astar_node_t*) malloc( sizeof(astar_node_t) );
					p_new_node->parent     = p_current_node;
					p_new_node->h          = h;
					p_new_node->g          = g;
					p_new_node->f          = f;
					p_new_node->state      = successor_state;

					pbheap_push( &p_astar->open_list, p_new_node );
					hash_map_insert( &p_astar->open_hash_map, p_new_node->state, p_new_node );
		
					#ifdef DEBUG_ASTAR
					p_astar->allocations++;
					#endif
				}
			} /* for */
			
			successors_clear( &successors );
		}

		/* e.) Add p_current_node to the closed list. */
		tree_map_insert( &p_astar->closed_list, p_current_node->state, p_current_node );
	}
	
	#ifdef DEBUG_ASTAR
	bench_mark_end( p_astar->bm );
	bench_mark_report( p_astar->bm );
	#endif

	return found;
}

void astar_cleanup( astar_t* p_astar )
{
	assert( hash_map_size(&p_astar->open_hash_map) == pbheap_size(&p_astar->open_list) );
	#ifdef DEBUG_ASTAR
	size_t size2 = hash_map_size(&p_astar->open_hash_map);	
	size_t size3 = tree_map_size(&p_astar->closed_list);	
	assert( size2 + size3 == p_astar->allocations );
	#endif
	
	p_astar->node_path = NULL;

	hash_map_iterator_t open_itr;
	tree_map_iterator_t closed_itr;
	
	hash_map_iterator( &p_astar->open_hash_map, &open_itr );

	// free everything on the open list.
	while( hash_map_iterator_next( &open_itr ) )
	{
		astar_node_t* p_node = hash_map_iterator_value( &open_itr );
		free( p_node );	
		#ifdef DEBUG_ASTAR
		p_astar->allocations--;
		#endif
	}

	// free everything on the closed list.
	for( closed_itr = tree_map_begin( &p_astar->closed_list );
	     closed_itr != tree_map_end( );
	     closed_itr = tree_map_next( closed_itr ) )
	{
		free( closed_itr->value );	
		#ifdef DEBUG_ASTAR
		p_astar->allocations--;
		#endif
	}

	// empty out the data structures
	pbheap_clear( &p_astar->open_list );
	hash_map_clear( &p_astar->open_hash_map );
	tree_map_clear( &p_astar->closed_list );
}

astar_node_t* astar_first_node( const astar_t* p_astar )
{
	assert( p_astar );
	return p_astar->node_path;
}

const void* astar_state( const astar_node_t* p_node )
{
	assert( p_node );
	return p_node->state;
}

astar_node_t* astar_next_node( const astar_node_t* p_node )
{
	assert( p_node );
	return p_node->parent;
}

