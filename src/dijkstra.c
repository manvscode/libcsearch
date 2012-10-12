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
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <libcollections/alloc.h>
#include <libcollections/bheap.h>
#include <libcollections/hash-map.h>
#include <libcollections/tree-map.h>
#include <libcollections/bench-mark.h>
#include "successors-private.h"
#include "csearch.h"

struct dijkstra_node {
	struct dijkstra_node* parent;
	int c; /* cost */
	const void* state; /* vertex */
};

struct dijkstra_algorithm {
	alloc_fxn alloc;
	free_fxn  free;

	compare_fxn          compare;
	nonnegative_cost_fxn cost;
	successors_fxn       successors_of;
	dijkstra_node_t*     node_path;

	successors_t successors;
	lc_pbheap_t     open_list; /* list of dijkstra_node_t* */
	lc_hash_map_t   open_hash_map; /* (state, dijkstra_node_t*) */
	#ifdef USE_TREEMAP_FOR_CLOSEDLIST
	lc_tree_map_t   closed_list; /* (state, dijkstra_node_t*) */
	#else
	lc_hash_map_t   closed_list; /* (state, dijkstra_node_t*) */
	#endif

	#ifdef DEBUG_DIJKSTRA
	size_t       allocations;
	lc_bench_mark_t bm;
	#endif
};

static boolean nop_keyval_fxn( void* __restrict key, void* __restrict value )
{
	return TRUE;
}

static int dijkstra_pointer_compare( const void* __restrict p_n1, const void* __restrict p_n2 )
{
	ptrdiff_t diff = (size_t*) p_n1 - (size_t*) p_n2;
	return (int) diff;
}

#define default_cost_compare( c1, c2 )      ((c2) - (c1))

static int best_cost_compare( const void* __restrict p_n1, const void* __restrict p_n2 )
{
	return default_cost_compare(((dijkstra_node_t*)p_n1)->c,  ((dijkstra_node_t*)p_n2)->c);
}

dijkstra_t* dijkstra_create( compare_fxn compare, state_hash_fxn state_hasher, nonnegative_cost_fxn cost, successors_fxn successors_of, alloc_fxn alloc, free_fxn free )
{
	dijkstra_t* p_dijkstra = (dijkstra_t*) alloc( sizeof(dijkstra_t) );

	if( p_dijkstra )
	{
		p_dijkstra->alloc         = alloc;
		p_dijkstra->free          = free;
		p_dijkstra->compare       = compare;
		p_dijkstra->cost          = cost;
		p_dijkstra->successors_of = successors_of;
		p_dijkstra->node_path     = NULL;
		#ifdef DEBUG_DIJKSTRA
		p_dijkstra->allocations   = 0;
		p_dijkstra->bm            = bench_mark_create( "Dijkstra's Search Algorithm" );
		#endif

		successors_create( &p_dijkstra->successors, 8, alloc, free );

		pbheap_create( &p_dijkstra->open_list, 128,
					  (heap_compare_function) best_cost_compare,
					  alloc, free );

		hash_map_create( &p_dijkstra->open_hash_map, HASH_MAP_SIZE_MEDIUM,
						 state_hasher, nop_keyval_fxn, dijkstra_pointer_compare,
						 alloc, free );

		#ifdef USE_TREEMAP_FOR_CLOSEDLIST
		tree_map_create( &p_dijkstra->closed_list, nop_keyval_fxn, dijkstra_pointer_compare, alloc, free );
		#else
		hash_map_create( &p_dijkstra->closed_list, HASH_MAP_SIZE_MEDIUM,
						 state_hasher, nop_keyval_fxn, dijkstra_pointer_compare,
						 alloc, free );
		#endif
	}

	return p_dijkstra;
}

void dijkstra_destroy( dijkstra_t** p_dijkstra )
{
	if( p_dijkstra && *p_dijkstra )
	{
		#ifdef DEBUG_BEST_FIRST_SEARCH
		bench_mark_destroy( (*p_dijkstra)->bm );
		#endif

		dijkstra_cleanup( *p_dijkstra );
		successors_destroy( &(*p_dijkstra)->successors );
		pbheap_destroy( &(*p_dijkstra)->open_list );
		hash_map_destroy( &(*p_dijkstra)->open_hash_map );
		#ifdef USE_TREEMAP_FOR_CLOSEDLIST
		tree_map_destroy( &(*p_dijkstra)->closed_list );
		#else
		hash_map_destroy( &(*p_dijkstra)->closed_list );
		#endif

		free_fxn _free = (*p_dijkstra)->free;
		_free( *p_dijkstra );
		*p_dijkstra = NULL;
	}
}

void dijkstra_set_compare_fxn( dijkstra_t* p_dijkstra, compare_fxn compare )
{
	if( p_dijkstra )
	{
		assert( compare );
		p_dijkstra->compare = compare;
	}
}

void dijkstra_set_cost_fxn( dijkstra_t* p_dijkstra, nonnegative_cost_fxn cost )
{
	if( p_dijkstra )
	{
		assert( cost );
		p_dijkstra->cost = cost;
	}
}

void dijkstra_set_successors_fxn( dijkstra_t* p_dijkstra, successors_fxn successors_of )
{
	if( p_dijkstra )
	{
		assert( successors_of );
		p_dijkstra->successors_of = successors_of;
	}
}

/*
 * Dijkstra's Algorithm
 * ------------------------------------------------------------------------
 *   Input: The start node and goal nodes.
 *  Output: True if goal node is found, false if goal node cannot be found
 *          from the start node; Shortest path between start to goal.
 * ------------------------------------------------------------------------
 * 1.) Set the start node to have 0 cost. Set the open list and closed
 *     list to be empty.
 * 2.) Add the start node to the open list.
 * 3.) While the open list is not empty, do the following:
 *    a.) Get a node from the open list, call it N.
 *    b.) If N is the goal node, return true.
 *    c.) Get the successor nodes of N.
 *    d.) For each successor node S:
 *    	  	 i.) If S is in the closed list, continue.
 *    		ii.) If S is in the open list, update its cost with the cost of
 *    		     N plus the cost to go from N to S, if it is better.  Make
 *    		     sure to resort the open list.
 *    	   iii.) If S is not in the open list, add it with the cost of N plus
 *    	         the cost to go from N to S.
      e.) Add N to the closed list.
 * 4.) Return false.
 */
bool dijkstra_find( dijkstra_t* __restrict p_dijkstra, const void* __restrict start, const void* __restrict end )
{
	dijkstra_node_t* p_node;
	bool found = false;
	int i;

	#ifdef DEBUG_BEST_FIRST_SEARCH
	bench_mark_start( p_dijkstra->bm );
	#endif
	assert( p_dijkstra );

 	/* 1.) Set the open list and closed list to be empty. */
	dijkstra_cleanup( p_dijkstra );

	p_node         = (dijkstra_node_t*) p_dijkstra->alloc( sizeof(dijkstra_node_t) );
	p_node->parent = NULL;
	p_node->c      = 0;
	p_node->state  = start;

	#ifdef DEBUG_DIJKSTRA
	p_dijkstra->allocations++;
	#endif

 	/* 2.) Add the start node to the open list. */
	pbheap_push( &p_dijkstra->open_list, p_node );
	hash_map_insert( &p_dijkstra->open_hash_map, p_node->state, p_node );

 	/* 3.) While the open list is not empty, do the following: */
	while( !found && pbheap_size(&p_dijkstra->open_list) > 0 )
	{
 		/* a.) Get a node from the open list, call it N. */
		dijkstra_node_t* p_current_node = pbheap_peek( &p_dijkstra->open_list );
		pbheap_pop( &p_dijkstra->open_list );
		hash_map_remove( &p_dijkstra->open_hash_map, p_current_node->state );

		/* b.) If N is the goal node, return true. */
		if( p_dijkstra->compare( p_current_node->state, end ) == 0 )
		{
			/* NOTE: This final node will not have the final cost
 			 * in the dijkstra_node_t object.
 			 */
			p_dijkstra->node_path = p_current_node;
			found = true;
		}
		else
		{
			/* c.) Get the successor nodes of N. */
			p_dijkstra->successors_of( p_current_node->state, &p_dijkstra->successors );

			/* d.) For each successor node S: */
			for( i = 0; i < successors_size(&p_dijkstra->successors); i++ )
			{
				const void* __restrict successor_state = successors_get( &p_dijkstra->successors, i );

				/* i.) If S is in the closed list, continue. */
				void* found_node;
				#ifdef USE_TREEMAP_FOR_CLOSEDLIST
				if( tree_map_find( &p_dijkstra->closed_list, successor_state, &found_node ) )
				#else
				if( hash_map_find( &p_dijkstra->closed_list, successor_state, &found_node ) )
				#endif
				{
					/* NOTE: The closed list is used to prevent re-examining
 					 * nodes that already have the minimal cost computed in
 					 * them. Without this, nodes would be added to the open
 					 * list without ever having a hope of being more optimal
 					 * than already computed. Although this is not needed,
 					 * it may improve performance.  Profiling will be needed
 					 * to determine this.
					 */
					continue;
				}

				/* ii.) If S is in the open list, update its cost with the cost of
				 *      N plus the cost to go from N to S, if it is better.  Make
				 *      sure to resort the open list.
				 */
				if( hash_map_find( &p_dijkstra->open_hash_map, successor_state, &found_node ) )
				{
					dijkstra_node_t* p_found_node = (dijkstra_node_t*) found_node;

					int c = p_dijkstra->cost( p_current_node->state, successor_state );

					if( default_cost_compare( c, p_found_node->c ) < 0 )
					{
						p_found_node->c      = c;
						p_found_node->parent = p_current_node;

						pbheap_reheapify( &p_dijkstra->open_list );
					}
				}
				else
				{
					/* iii.) If S is not in the open list, add it with the cost of N
					 *       plus the cost to go from N to S.
					 */
					dijkstra_node_t* p_new_node = (dijkstra_node_t*) p_dijkstra->alloc( sizeof(dijkstra_node_t) );
					p_new_node->parent     = p_current_node;
					p_new_node->c          = p_current_node->c + p_dijkstra->cost( p_current_node->state, successor_state );
					p_new_node->state      = successor_state;

					pbheap_push( &p_dijkstra->open_list, p_new_node );
					hash_map_insert( &p_dijkstra->open_hash_map, p_new_node->state, p_new_node );

					#ifdef DEBUG_DIJKSTRA
					p_dijkstra->allocations++;
					#endif
				}
			} /* for */

			successors_clear( &p_dijkstra->successors );
		}

		/* e.) Add p_current_node to the closed list. */
		#ifdef USE_TREEMAP_FOR_CLOSEDLIST
		tree_map_insert( &p_dijkstra->closed_list, p_current_node->state, p_current_node );
		#else
		hash_map_insert( &p_dijkstra->closed_list, p_current_node->state, p_current_node );
		#endif
	}

	#ifdef DEBUG_BEST_FIRST_SEARCH
	bench_mark_end( p_dijkstra->bm );
	bench_mark_report( p_dijkstra->bm );
	#endif

	return found;
}

void dijkstra_cleanup( dijkstra_t* p_dijkstra )
{
	lc_hash_map_iterator_t open_itr;
	#ifdef USE_TREEMAP_FOR_CLOSEDLIST
	lc_tree_map_iterator_t closed_itr;
	#else
	lc_hash_map_iterator_t closed_itr;
	#endif

	assert( p_dijkstra );
	assert( hash_map_size(&p_dijkstra->open_hash_map) == pbheap_size(&p_dijkstra->open_list) );

	p_dijkstra->node_path = NULL;
	successors_clear( &p_dijkstra->successors );
	pbheap_clear( &p_dijkstra->open_list );

	hash_map_iterator( &p_dijkstra->open_hash_map, &open_itr );
	// free everything on the open list.
	while( hash_map_iterator_next( &open_itr ) )
	{
		dijkstra_node_t* p_node = hash_map_iterator_value( &open_itr );
		p_dijkstra->free( p_node );
		#ifdef DEBUG_DIJKSTRA
		p_dijkstra->allocations--;
		#endif
	}
	hash_map_clear( &p_dijkstra->open_hash_map );

	#ifdef USE_TREEMAP_FOR_CLOSEDLIST
	// free everything on the closed list.
	for( closed_itr = tree_map_begin( &p_dijkstra->closed_list );
	     closed_itr != tree_map_end( );
	     closed_itr = tree_map_next( closed_itr ) )
	{
		p_dijkstra->free( closed_itr->value );
		#ifdef DEBUG_DIJKSTRA
		p_dijkstra->allocations--;
		#endif
	}
	tree_map_clear( &p_dijkstra->closed_list );
	#else
	hash_map_iterator( &p_dijkstra->closed_list, &closed_itr );
	// free everything on the open list.
	while( hash_map_iterator_next( &closed_itr ) )
	{
		dijkstra_node_t* p_node = hash_map_iterator_value( &closed_itr );
		p_dijkstra->free( p_node );
		#ifdef DEBUG_DIJKSTRA
		p_dijkstra->allocations--;
		#endif
	}
	hash_map_clear( &p_dijkstra->closed_list );
	#endif

}

dijkstra_node_t* dijkstra_first_node( const dijkstra_t* p_dijkstra )
{
	assert( p_dijkstra );
	return p_dijkstra->node_path;
}

const void* dijkstra_state( const dijkstra_node_t* p_node )
{
	assert( p_node );
	return p_node->state;
}

dijkstra_node_t* dijkstra_next_node( const dijkstra_node_t* p_node )
{
	assert( p_node );
	return p_node->parent;
}

void dijkstra_iterative_init( dijkstra_t* __restrict p_dijkstra, const void* __restrict start, const void* __restrict end, bool* found )
{
	dijkstra_node_t* p_node;
	#ifdef DEBUG_BEST_FIRST_SEARCH
	bench_mark_start( p_dijkstra->bm );
	#endif
	assert( p_dijkstra );
	*found = false;

 	/* 1.) Set the open list and closed list to be empty. */
	dijkstra_cleanup( p_dijkstra );

	p_node         = (dijkstra_node_t*) p_dijkstra->alloc( sizeof(dijkstra_node_t) );
	p_node->parent = NULL;
	p_node->c      = 0;
	p_node->state  = start;

	#ifdef DEBUG_DIJKSTRA
	p_dijkstra->allocations++;
	#endif

 	/* 2.) Add the start node to the open list. */
	pbheap_push( &p_dijkstra->open_list, p_node );
	hash_map_insert( &p_dijkstra->open_hash_map, p_node->state, p_node );
}

void dijkstra_iterative_find( dijkstra_t* __restrict p_dijkstra, const void* __restrict start, const void* __restrict end, bool* found )
{
 	/* 3.) While the open list is not empty, do the following: */
	if( !*found && pbheap_size(&p_dijkstra->open_list) > 0 )
	{
 		/* a.) Get a node from the open list, call it N. */
		dijkstra_node_t* p_current_node = pbheap_peek( &p_dijkstra->open_list );
		pbheap_pop( &p_dijkstra->open_list );
		hash_map_remove( &p_dijkstra->open_hash_map, p_current_node->state );

		/* b.) If N is the goal node, return true. */
		if( p_dijkstra->compare( p_current_node->state, end ) == 0 )
		{
			/* NOTE: This final node will not have the final cost
 			 * in the dijkstra_node_t object.
 			 */
			p_dijkstra->node_path = p_current_node;
			*found = true;
		}
		else
		{
			int i;

			/* c.) Get the successor nodes of N. */
			p_dijkstra->successors_of( p_current_node->state, &p_dijkstra->successors );

			/* d.) For each successor node S: */
			for( i = 0; i < successors_size(&p_dijkstra->successors); i++ )
			{
				const void* __restrict successor_state = successors_get( &p_dijkstra->successors, i );

				/* i.) If S is in the closed list, continue. */
				void* found_node;
				#ifdef USE_TREEMAP_FOR_CLOSEDLIST
				if( tree_map_find( &p_dijkstra->closed_list, successor_state, &found_node ) )
				#else
				if( hash_map_find( &p_dijkstra->closed_list, successor_state, &found_node ) )
				#endif
				{
					/* NOTE: The closed list is used to prevent re-examining
 					 * nodes that already have the minimal cost computed in
 					 * them. Without this, nodes would be added to the open
 					 * list without ever having a hope of being more optimal
 					 * than already computed. Although this is not needed,
 					 * it may improve performance.  Profiling will be needed
 					 * to determine this.
					 */
					return;
				}

				/* ii.) If S is in the open list, update its cost with the cost of
				 *      N plus the cost to go from N to S, if it is better.  Make
				 *      sure to resort the open list.
				 */
				if( hash_map_find( &p_dijkstra->open_hash_map, successor_state, &found_node ) )
				{
					dijkstra_node_t* p_found_node = (dijkstra_node_t*) found_node;

					int c = p_dijkstra->cost( p_current_node->state, successor_state );

					if( default_cost_compare( c, p_found_node->c ) < 0 )
					{
						p_found_node->c      = c;
						p_found_node->parent = p_current_node;

						pbheap_reheapify( &p_dijkstra->open_list );
					}
				}
				else
				{
					/* iii.) If S is not in the open list, add it with the cost of N
					 *       plus the cost to go from N to S.
					 */
					dijkstra_node_t* p_new_node = (dijkstra_node_t*) p_dijkstra->alloc( sizeof(dijkstra_node_t) );
					p_new_node->parent     = p_current_node;
					p_new_node->c          = p_current_node->c + p_dijkstra->cost( p_current_node->state, successor_state );
					p_new_node->state      = successor_state;

					pbheap_push( &p_dijkstra->open_list, p_new_node );
					hash_map_insert( &p_dijkstra->open_hash_map, p_new_node->state, p_new_node );

					#ifdef DEBUG_DIJKSTRA
					p_dijkstra->allocations++;
					#endif
				}
			} /* for */

			successors_clear( &p_dijkstra->successors );
		}

		/* e.) Add p_current_node to the closed list. */
		#ifdef USE_TREEMAP_FOR_CLOSEDLIST
		tree_map_insert( &p_dijkstra->closed_list, p_current_node->state, p_current_node );
		#else
		hash_map_insert( &p_dijkstra->closed_list, p_current_node->state, p_current_node );
		#endif
	}

	#ifdef DEBUG_BEST_FIRST_SEARCH
	if( *found )
	{
		bench_mark_end( p_dijkstra->bm );
		bench_mark_report( p_dijkstra->bm );
	}
	#endif
}

bool dijkstra_iterative_is_done( dijkstra_t* __restrict p_dijkstra, bool* found )
{
	return *found || pbheap_size(&p_dijkstra->open_list) == 0;
}
