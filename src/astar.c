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
#include <stdbool.h>
#include <assert.h>
#include <collections/binary-heap.h>
#include <collections/hash-map.h>
#include <collections/tree-map.h>
#include <collections/benchmark.h>
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
	alloc_fxn_t alloc;
	free_fxn_t free;

	compare_fxn_t   compare;
	heuristic_fxn_t heuristic;
	cost_fxn_t      cost;
	successors_fxn_t successors_of;
	astar_node_t*   node_path;

	successors_t  successors;
	astar_node_t** open_list; /* list of astar_node_t* */
	lc_hash_map_t open_hash_map; /* (state, astar_node_t*) */
	#ifdef USE_TREEMAP_FOR_CLOSEDLIST
	lc_tree_map_t closed_list; /* (state, astar_node_t*) */
	#else
	lc_hash_map_t closed_list; /* (state, astar_node_t*) */
	#endif

	#ifdef DEBUG_ASTAR
	size_t       allocations;
	lc_bench_mark_t bm;
	#endif
};


static bool nop_keyval_fxn( void* __restrict key, void* __restrict value )
{
	return true;
}

static int astar_pointer_compare( const void* __restrict p_n1, const void* __restrict p_n2 )
{
	ptrdiff_t diff = (size_t* __restrict) p_n1 - (size_t* __restrict) p_n2;
	return (int) diff;
}

#define default_f_compare( f1, f2 )      ((f2) - (f1))

static int best_f_compare( const void* __restrict p_n1, const void* __restrict p_n2 )
{
	return default_f_compare( ((astar_node_t* __restrict)p_n1)->f, ((astar_node_t* __restrict)p_n2)->f );
}


astar_t* astar_create( compare_fxn_t compare, state_hash_fxn_t state_hasher, heuristic_fxn_t heuristic, cost_fxn_t cost, successors_fxn_t successors_of, alloc_fxn_t alloc, free_fxn_t free )
{
	astar_t* p_astar = (astar_t*) alloc( sizeof(astar_t) );

	if( p_astar )
	{
		p_astar->alloc         = alloc;
		p_astar->free          = free;
		p_astar->compare       = compare;
		p_astar->heuristic     = heuristic;
		p_astar->cost          = cost;
		p_astar->successors_of = successors_of;
		p_astar->open_list     = NULL;
		p_astar->node_path     = NULL;
		#ifdef DEBUG_ASTAR
		p_astar->allocations   = 0;
		p_astar->bm            = bench_mark_create( "A* Search Algorithm" );
		#endif

		successors_create( &p_astar->successors, 8, alloc, free );


		lc_binary_heap_create(p_astar->open_list, 128);

		lc_hash_map_create( &p_astar->open_hash_map, LC_HASH_MAP_SIZE_MEDIUM,
						 state_hasher, nop_keyval_fxn, astar_pointer_compare,
						 alloc, free );

		#ifdef USE_TREEMAP_FOR_CLOSEDLIST
		lc_tree_map_create( &p_astar->closed_list, nop_keyval_fxn, astar_pointer_compare, alloc, free );
		#else
		lc_hash_map_create( &p_astar->closed_list, LC_HASH_MAP_SIZE_MEDIUM,
						 state_hasher, nop_keyval_fxn, astar_pointer_compare,
						 alloc, free );
		#endif
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
		successors_destroy( &(*p_astar)->successors );
		lc_binary_heap_destroy( (*p_astar)->open_list );
		lc_hash_map_destroy( &(*p_astar)->open_hash_map );
		#ifdef USE_TREEMAP_FOR_CLOSEDLIST
		lc_tree_map_destroy( &(*p_astar)->closed_list );
		#else
		lc_hash_map_destroy( &(*p_astar)->closed_list );
		#endif
		free_fxn_t _free = (*p_astar)->free;
		_free( *p_astar );
		*p_astar = NULL;

	}
}

void astar_set_compare_fxn( astar_t* p_astar, compare_fxn_t compare )
{
	if( p_astar )
	{
		assert( compare );
		p_astar->compare = compare;
	}
}

void astar_set_heuristic_fxn( astar_t* p_astar, heuristic_fxn_t heuristic )
{
	if( p_astar )
	{
		assert( heuristic );
		p_astar->heuristic = heuristic;
	}
}

void astar_set_cost_fxn( astar_t* p_astar, cost_fxn_t cost )
{
	if( p_astar )
	{
		assert( cost );
		p_astar->cost = cost;
	}
}

void astar_set_successors_fxn( astar_t* p_astar, successors_fxn_t successors_of )
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
bool astar_find( astar_t* __restrict p_astar, const void* __restrict start, const void* __restrict end )
{
	#ifdef DEBUG_ASTAR
	bench_mark_start( p_astar->bm );
	#endif
	astar_node_t* p_node;
	bool found = false;
	int i;

 	/* 1.) Set the open list and closed list to be empty. */
	astar_cleanup( p_astar );

	p_node         = (astar_node_t*) p_astar->alloc( sizeof(astar_node_t) );
	p_node->parent = NULL;
	p_node->h      = p_astar->heuristic( start, end );
	p_node->g      = 0 /* no cost */;
	p_node->f      = p_node->g + p_node->h;
	p_node->state  = start;

	#ifdef DEBUG_ASTAR
	p_astar->allocations++;
	#endif

 	/* 2.) Add the start node to the open list. */
	lc_binary_heap_push( p_astar->open_list, p_node, astar_node_t*, p_astar->compare );
	lc_hash_map_insert( &p_astar->open_hash_map, p_node->state, p_node );

 	/* 3.) While the open list is not empty, do the following: */
	while( !found && lc_binary_heap_size(p_astar->open_list) > 0 )
	{
 		/* a.) Get a node from the open list, call it p_current_node. */
		astar_node_t* p_current_node = lc_binary_heap_peek( p_astar->open_list );
		lc_binary_heap_pop( p_astar->open_list, astar_node_t*, p_astar->compare );
		lc_hash_map_remove( &p_astar->open_hash_map, p_current_node->state );

		/* b.) If p_current_node is the goal node, return true. */
		if( p_astar->compare( p_current_node->state, end ) == 0 )
		{
			p_astar->node_path = p_current_node;
			found = true;
		}
		else
		{
			/* c.) Get the successor nodes of p_current_node. */
			p_astar->successors_of( p_current_node->state, &p_astar->successors );

			/* d.) For each successor node S: */
			for( i = 0; i < successors_size(&p_astar->successors); i++ )
			{
				const void* __restrict successor_state = successors_get( &p_astar->successors, i );

				/* i.) If S is in the closed list: */
				void* found_node;
				#ifdef USE_TREEMAP_FOR_CLOSEDLIST
				if( lc_tree_map_find( &p_astar->closed_list, successor_state, &found_node ) )
				#else
				if( lc_hash_map_find( &p_astar->closed_list, successor_state, &found_node ) )
				#endif
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

						#ifdef USE_TREEMAP_FOR_CLOSEDLIST
						lc_tree_map_remove( &p_astar->closed_list, successor_state );
						#else
						lc_hash_map_remove( &p_astar->closed_list, successor_state );
						#endif

						pbheap_push( &p_astar->open_list, p_found_node );
						hash_map_insert( &p_astar->open_hash_map, p_found_node->state, p_found_node );
					}
					#endif
				}

				/* ii.) If S is in open list: */
				if( lc_hash_map_find( &p_astar->open_hash_map, successor_state, &found_node ) )
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

						lc_binary_heap_reheapify( p_astar->open_list, astar_node_t*, p_astar->compare );
					}
				}
				else /* iii.) If S is not in the open list, then add S to the open list. */
				{
					int h = p_astar->heuristic( successor_state, end );
					int g = p_current_node->g + p_astar->cost( p_current_node->state, successor_state );
					int f = g + h;

					astar_node_t* p_new_node = (astar_node_t*) p_astar->alloc( sizeof(astar_node_t) );
					p_new_node->parent     = p_current_node;
					p_new_node->h          = h;
					p_new_node->g          = g;
					p_new_node->f          = f;
					p_new_node->state      = successor_state;

					lc_binary_heap_push( p_astar->open_list, p_new_node, astar_node_t*, p_astar->compare );
					lc_hash_map_insert( &p_astar->open_hash_map, p_new_node->state, p_new_node );

					#ifdef DEBUG_ASTAR
					p_astar->allocations++;
					#endif
				}
			} /* for */

			successors_clear( &p_astar->successors );
		}

		/* e.) Add p_current_node to the closed list. */
		#ifdef USE_TREEMAP_FOR_CLOSEDLIST
		lc_tree_map_insert( &p_astar->closed_list, p_current_node->state, p_current_node );
		#else
		lc_hash_map_insert( &p_astar->closed_list, p_current_node->state, p_current_node );
		#endif
	}

	#ifdef DEBUG_ASTAR
	bench_mark_end( p_astar->bm );
	bench_mark_report( p_astar->bm );
	#endif

	return found;
}

void astar_cleanup( astar_t* p_astar )
{
	lc_hash_map_iterator_t open_itr;
	#ifdef USE_TREEMAP_FOR_CLOSEDLIST
	lc_tree_map_iterator_t closed_itr;
	#else
	lc_hash_map_iterator_t closed_itr;
	#endif

	assert( lc_hash_map_size(&p_astar->open_hash_map) == lc_binary_heap_size(p_astar->open_list) );

	p_astar->node_path = NULL;
	successors_clear( &p_astar->successors );
	lc_binary_heap_clear( p_astar->open_list );

	lc_hash_map_iterator( &p_astar->open_hash_map, &open_itr );
	// free everything on the open list.
	while( lc_hash_map_iterator_next( &open_itr ) )
	{
		astar_node_t* p_node = lc_hash_map_iterator_value( &open_itr );
		p_astar->free( p_node );
		#ifdef DEBUG_ASTAR
		p_astar->allocations--;
		#endif
	}
	lc_hash_map_clear( &p_astar->open_hash_map );

	#ifdef USE_TREEMAP_FOR_CLOSEDLIST
	// free everything on the closed list.
	for( closed_itr = lc_tree_map_begin( &p_astar->closed_list );
	     closed_itr != lc_tree_map_end( );
	     closed_itr = lc_tree_map_next( closed_itr ) )
	{
		p_astar->free( closed_itr->value );
		#ifdef DEBUG_ASTAR
		p_astar->allocations--;
		#endif
	}
	lc_tree_map_clear( &p_astar->closed_list );
	#else
	lc_hash_map_iterator( &p_astar->closed_list, &closed_itr );
	// free everything on the open list.
	while( lc_hash_map_iterator_next( &closed_itr ) )
	{
		astar_node_t* p_node = lc_hash_map_iterator_value( &closed_itr );
		p_astar->free( p_node );
		#ifdef DEBUG_ASTAR
		p_astar->allocations--;
		#endif
	}
	lc_hash_map_clear( &p_astar->closed_list );
	#endif
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

void astar_iterative_init( astar_t* __restrict p_astar, const void* __restrict start, const void* __restrict end, bool* found )
{
	astar_node_t* p_node;
	#ifdef DEBUG_ASTAR
	bench_mark_start( p_astar->bm );
	#endif
	*found = false;

 	/* 1.) Set the open list and closed list to be empty. */
	astar_cleanup( p_astar );

	p_node         = (astar_node_t*) p_astar->alloc( sizeof(astar_node_t) );
	p_node->parent = NULL;
	p_node->h      = p_astar->heuristic( start, end );
	p_node->g      = 0 /* no cost */;
	p_node->f      = p_node->g + p_node->h;
	p_node->state  = start;

	#ifdef DEBUG_ASTAR
	p_astar->allocations++;
	#endif

 	/* 2.) Add the start node to the open list. */
	lc_binary_heap_push( p_astar->open_list, p_node, astar_node_t*, p_astar->compare );
	lc_hash_map_insert( &p_astar->open_hash_map, p_node->state, p_node );
}

void astar_iterative_find( astar_t* __restrict p_astar, const void* __restrict start, const void* __restrict end, bool* found )
{
 	/* 3.) While the open list is not empty, do the following: */
	if( !*found && lc_binary_heap_size(p_astar->open_list) > 0 )
	{
 		/* a.) Get a node from the open list, call it p_current_node. */
		astar_node_t* p_current_node = lc_binary_heap_peek( p_astar->open_list );
		lc_binary_heap_pop( p_astar->open_list, astar_node_t*, p_astar->compare );
		lc_hash_map_remove( &p_astar->open_hash_map, p_current_node->state );

		/* b.) If p_current_node is the goal node, return true. */
		if( p_astar->compare( p_current_node->state, end ) == 0 )
		{
			p_astar->node_path = p_current_node;
			*found = true;
		}
		else
		{
			int i;

			/* c.) Get the successor nodes of p_current_node. */
			p_astar->successors_of( p_current_node->state, &p_astar->successors );

			/* d.) For each successor node S: */
			for( i = 0; i < successors_size(&p_astar->successors); i++ )
			{
				const void* __restrict successor_state = successors_get( &p_astar->successors, i );

				/* i.) If S is in the closed list: */
				void* found_node;
				#ifdef USE_TREEMAP_FOR_CLOSEDLIST
				if( tree_map_find( &p_astar->closed_list, successor_state, &found_node ) )
				#else
				if( lc_hash_map_find( &p_astar->closed_list, successor_state, &found_node ) )
				#endif
				{
					#if 1
					return;
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

						#ifdef USE_TREEMAP_FOR_CLOSEDLIST
						tree_map_remove( &p_astar->closed_list, successor_state );
						#else
						hash_map_remove( &p_astar->closed_list, successor_state );
						#endif

						pbheap_push( &p_astar->open_list, p_found_node );
						hash_map_insert( &p_astar->open_hash_map, p_found_node->state, p_found_node );
					}
					#endif
				}

				/* ii.) If S is in open list: */
				if( lc_hash_map_find( &p_astar->open_hash_map, successor_state, &found_node ) )
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

						lc_binary_heap_reheapify( p_astar->open_list, astar_node_t*, p_astar->compare );
					}
				}
				else /* iii.) If S is not in the open list, then add S to the open list. */
				{
					int h = p_astar->heuristic( successor_state, end );
					int g = p_current_node->g + p_astar->cost( p_current_node->state, successor_state );
					int f = g + h;

					astar_node_t* p_new_node = (astar_node_t*) p_astar->alloc( sizeof(astar_node_t) );
					p_new_node->parent     = p_current_node;
					p_new_node->h          = h;
					p_new_node->g          = g;
					p_new_node->f          = f;
					p_new_node->state      = successor_state;

					lc_binary_heap_push( p_astar->open_list, p_new_node, astar_node_t*, p_astar->compare );
					lc_hash_map_insert( &p_astar->open_hash_map, p_new_node->state, p_new_node );

					#ifdef DEBUG_ASTAR
					p_astar->allocations++;
					#endif
				}
			} /* for */

			successors_clear( &p_astar->successors );
		}

		/* e.) Add p_current_node to the closed list. */
		#ifdef USE_TREEMAP_FOR_CLOSEDLIST
		lc_tree_map_insert( &p_astar->closed_list, p_current_node->state, p_current_node );
		#else
		lc_hash_map_insert( &p_astar->closed_list, p_current_node->state, p_current_node );
		#endif
	}

	#ifdef DEBUG_ASTAR
	if( *found )
	{
		bench_mark_end( p_astar->bm );
		bench_mark_report( p_astar->bm );
	}
	#endif
}

bool astar_iterative_is_done( astar_t* __restrict p_astar, bool* found )
{
	return *found || lc_binary_heap_size(p_astar->open_list) == 0;
}
