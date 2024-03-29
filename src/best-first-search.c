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
#include <assert.h>
#include <collections/binary-heap.h>
#include <collections/hash-map.h>
#include <collections/tree-map.h>
#include <collections/benchmark.h>
#include "successors-private.h"
#include "csearch.h"

struct bestfs_node {
	struct bestfs_node* parent;
	int h; /* heuristic */
	const void* state; /* vertex */
};

struct bestfs_algorithm {
	alloc_fxn_t alloc;
	free_fxn_t  free;

	compare_fxn_t   compare;
	heuristic_fxn_t heuristic;
	successors_fxn_t successors_of;
	bestfs_node_t*  node_path;

	successors_t   successors;
	lc_pbheap_t    open_list; /* list of bestfs_node_t* */
	lc_hash_map_t  open_hash_map; /* (state, bestfs_node_t*) */
	#ifdef USE_TREEMAP_FOR_CLOSEDLIST
	lc_tree_map_t  closed_list; /* (state, bestfs_node_t*) */
	#else
	lc_hash_map_t  closed_list; /* (state, bestfs_node_t*) */
	#endif

	#ifdef DEBUG_BEST_FIRST_SEARCH
	size_t       allocations;
	lc_bench_mark_t bm;
	#endif
};

static boolean nop_keyval_fxn( void* __restrict key, void* __restrict value )
{
	return TRUE;
}

static int bestfs_pointer_compare( const void* __restrict p_n1, const void* __restrict p_n2 )
{
	ptrdiff_t diff = (size_t* __restrict) p_n1 - (size_t* __restrict) p_n2;
	return (int) diff;
}

#define default_heuristic_compare( h1, h2 )  ((h2) - (h1))


static int bestfs_heuristic_compare( const void* __restrict p_n1, const void* __restrict p_n2 )
{
	return default_heuristic_compare( ((bestfs_node_t* __restrict)p_n1)->h, ((bestfs_node_t* __restrict)p_n2)->h );
}



bestfs_t* bestfs_create( compare_fxn_t compare, state_hash_fxn_t state_hasher, heuristic_fxn_t heuristic, successors_fxn_t successors_of, alloc_fxn_t alloc, free_fxn_t free )
{
	bestfs_t* p_best = (bestfs_t*) alloc( sizeof(bestfs_t) );

	if( p_best )
	{
		p_best->alloc         = alloc;
		p_best->free          = free;
		p_best->compare       = compare;
		p_best->heuristic     = heuristic;
		p_best->successors_of = successors_of;
		p_best->node_path     = NULL;
		#ifdef DEBUG_BEST_FIRST_SEARCH
		p_best->allocations   = 0;
		p_best->bm            = bench_mark_create( "Best First Search Algorithm" );
		#endif

		successors_create( &p_best->successors, 8, alloc, free );

		pbheap_create( &p_best->open_list, 128,
					   bestfs_heuristic_compare,
					   alloc, free );

		hash_map_create( &p_best->open_hash_map, HASH_MAP_SIZE_MEDIUM,
						 state_hasher, nop_keyval_fxn, bestfs_pointer_compare,
						 alloc, free );

		#ifdef USE_TREEMAP_FOR_CLOSEDLIST
		tree_map_create( &p_best->closed_list, nop_keyval_fxn, bestfs_pointer_compare, alloc, free );
		#else
		hash_map_create( &p_best->closed_list, HASH_MAP_SIZE_SMALL,
						 state_hasher, nop_keyval_fxn, bestfs_pointer_compare,
						 alloc, free );
		#endif
	}

	return p_best;
}

void bestfs_destroy( bestfs_t** p_best )
{
	if( p_best && *p_best )
	{
		#ifdef DEBUG_BEST_FIRST_SEARCH
		bench_mark_destroy( (*p_best)->bm );
		#endif

		bestfs_cleanup( *p_best );
		successors_destroy( &(*p_best)->successors );
		pbheap_destroy( &(*p_best)->open_list );
		hash_map_destroy( &(*p_best)->open_hash_map );
		#ifdef USE_TREEMAP_FOR_CLOSEDLIST
		tree_map_destroy( &(*p_best)->closed_list );
		#else
		hash_map_destroy( &(*p_best)->closed_list );
		#endif

		free_fxn_t _free = (*p_best)->free;
		_free( *p_best );
		*p_best = NULL;

	}
}

void bestfs_set_compare_fxn( bestfs_t* p_best, compare_fxn_t compare )
{
	if( p_best )
	{
		assert( compare );
		p_best->compare = compare;
	}
}

void bestfs_set_heuristic_fxn( bestfs_t* p_best, heuristic_fxn_t heuristic )
{
	if( p_best )
	{
		assert( heuristic );
		p_best->heuristic = heuristic;
	}
}

void bestfs_set_successors_fxn( bestfs_t* p_best, successors_fxn_t successors_of )
{
	if( p_best )
	{
		assert( successors_of );
		p_best->successors_of = successors_of;
	}
}

/*
 * Best First Search Algorithm
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
bool bestfs_find( bestfs_t* __restrict p_best, const void* __restrict start, const void* __restrict end )
{
	bestfs_node_t* p_node;
	bool found = false;
	int i;

	#ifdef DEBUG_BEST_FIRST_SEARCH
	bench_mark_start( p_best->bm );
	#endif

 	/* 1.) Set the open list and closed list to be empty. */
	bestfs_cleanup( p_best );

	p_node         = (bestfs_node_t*) p_best->alloc( sizeof(bestfs_node_t) );
	p_node->parent = NULL;
	p_node->h      = p_best->heuristic( start, end );
	p_node->state  = start;

	#ifdef DEBUG_BEST_FIRST_SEARCH
	p_best->allocations++;
	#endif

 	/* 2.) Add the start node to the open list. */
	pbheap_push( &p_best->open_list, p_node );
	hash_map_insert( &p_best->open_hash_map, p_node->state, p_node );

 	/* 3.) While the open list is not empty, do the following: */
	while( !found && pbheap_size(&p_best->open_list) > 0 )
	{
 		/* a.) Get a node from the open list, call it p_current_node. */
		bestfs_node_t* p_current_node = pbheap_peek( &p_best->open_list );
		pbheap_pop( &p_best->open_list );
		hash_map_remove( &p_best->open_hash_map, p_current_node->state );

		/* b.) If p_current_node is the goal node, return true. */
		if( p_best->compare( p_current_node->state, end ) == 0 )
		{
			p_best->node_path = p_current_node;
			found = true;
		}
		else
		{
			/* c.) Get the successor nodes of p_current_node. */
			p_best->successors_of( p_current_node->state, &p_best->successors );

			/* d.) For each successor node S: */
			for( i = 0; i < successors_size(&p_best->successors); i++ )
			{
				const void* __restrict successor_state = successors_get( &p_best->successors, i );

				/* i.) If S is in the closed list, continue. */
				void* found_node;
				#ifdef USE_TREEMAP_FOR_CLOSEDLIST
				if( tree_map_find( &p_best->closed_list, successor_state, &found_node ) )
				#else
				if( hash_map_find( &p_best->closed_list, successor_state, &found_node ) )
				#endif
				{
					continue;
				}

				/* ii.) If S is in open list: */
				if( hash_map_find( &p_best->open_hash_map, successor_state, &found_node ) )
				{
					bestfs_node_t* p_found_node = (bestfs_node_t*) found_node;
					/* If its heuristic is better, then update its
					 * heuristic with the better value and resort
					 * the open list.
					 */
					int h = p_best->heuristic( p_found_node->state, end );

					if( default_heuristic_compare( h, p_found_node->h ) < 0 ) // TODO: Expose heuristic compare
					{
						p_found_node->h      = h;
						p_found_node->parent = p_current_node;

						pbheap_reheapify( &p_best->open_list );
					}
				}
				else /* iii.) If S is not in the open list, then add S to the open list. */
				{
					bestfs_node_t* p_new_node = (bestfs_node_t*) p_best->alloc( sizeof(bestfs_node_t) );
					p_new_node->parent     = p_current_node;
					p_new_node->h          = p_best->heuristic( successor_state, end );
					p_new_node->state      = successor_state;

					pbheap_push( &p_best->open_list, p_new_node );
					hash_map_insert( &p_best->open_hash_map, p_new_node->state, p_new_node );

					#ifdef DEBUG_BEST_FIRST_SEARCH
					p_best->allocations++;
					#endif
				}
			} /* for */

			successors_clear( &p_best->successors );
		}

		/* e.) Add p_current_node to the closed list. */
		#ifdef USE_TREEMAP_FOR_CLOSEDLIST
		tree_map_insert( &p_best->closed_list, p_current_node->state, p_current_node );
		#else
		hash_map_insert( &p_best->closed_list, p_current_node->state, p_current_node );
		#endif
	}

	#ifdef DEBUG_BEST_FIRST_SEARCH
	bench_mark_end( p_best->bm );
	bench_mark_report( p_best->bm );
	#endif

	return found;
}

void bestfs_cleanup( bestfs_t* p_best )
{
	lc_hash_map_iterator_t open_itr;
	#ifdef USE_TREEMAP_FOR_CLOSEDLIST
	lc_tree_map_iterator_t closed_itr;
	#else
	lc_hash_map_iterator_t closed_itr;
	#endif
	assert( hash_map_size(&p_best->open_hash_map) == pbheap_size(&p_best->open_list) );

	successors_clear( &p_best->successors );
	pbheap_clear( &p_best->open_list );
	p_best->node_path = NULL;

	hash_map_iterator( &p_best->open_hash_map, &open_itr );
	// free everything on the open list.
	while( hash_map_iterator_next( &open_itr ) )
	{
		bestfs_node_t* p_node = hash_map_iterator_value( &open_itr );
		p_best->free( p_node );
		#ifdef DEBUG_BEST_FIRST_SEARCH
		p_best->allocations--;
		#endif
	}
	hash_map_clear( &p_best->open_hash_map );

	#ifdef USE_TREEMAP_FOR_CLOSEDLIST
	// free everything on the closed list.
	for( closed_itr = tree_map_begin( &p_best->closed_list );
	     closed_itr != tree_map_end( );
	     closed_itr = tree_map_next( closed_itr ) )
	{
		p_best->free( closed_itr->value );
		#ifdef DEBUG_BEST_FIRST_SEARCH
		p_best->allocations--;
		#endif
	}
	tree_map_clear( &p_best->closed_list );
	#else
	hash_map_iterator( &p_best->closed_list, &closed_itr );
	// free everything on the open list.
	while( hash_map_iterator_next( &closed_itr ) )
	{
		bestfs_node_t* p_node = hash_map_iterator_value( &closed_itr );
		p_best->free( p_node );
		#ifdef DEBUG_BEST_FIRST_SEARCH
		p_best->allocations--;
		#endif
	}
	hash_map_clear( &p_best->closed_list );
	#endif

}

bestfs_node_t* bestfs_first_node( const bestfs_t* p_best )
{
	assert( p_best );
	return p_best->node_path;
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

void bestfs_iterative_init( bestfs_t* __restrict p_best, const void* __restrict start, const void* __restrict end, bool* __restrict found )
{
	bestfs_node_t* p_node;
	#ifdef DEBUG_BEST_FIRST_SEARCH
	bench_mark_start( p_best->bm );
	#endif
	*found = false;

 	/* 1.) Set the open list and closed list to be empty. */
	bestfs_cleanup( p_best );

	p_node         = (bestfs_node_t*) p_best->alloc( sizeof(bestfs_node_t) );
	p_node->parent = NULL;
	p_node->h      = p_best->heuristic( start, end );
	p_node->state  = start;

	#ifdef DEBUG_BEST_FIRST_SEARCH
	p_best->allocations++;
	#endif

 	/* 2.) Add the start node to the open list. */
	pbheap_push( &p_best->open_list, p_node );
	hash_map_insert( &p_best->open_hash_map, p_node->state, p_node );
}

void bestfs_iterative_find( bestfs_t* __restrict p_best, const void* __restrict start, const void* __restrict end, bool* __restrict found )
{
 	/* 3.) While the open list is not empty, do the following: */
	if( !*found && pbheap_size(&p_best->open_list) > 0 )
	{
 		/* a.) Get a node from the open list, call it p_current_node. */
		bestfs_node_t* p_current_node = pbheap_peek( &p_best->open_list );
		pbheap_pop( &p_best->open_list );
		hash_map_remove( &p_best->open_hash_map, p_current_node->state );

		/* b.) If p_current_node is the goal node, return true. */
		if( p_best->compare( p_current_node->state, end ) == 0 )
		{
			p_best->node_path = p_current_node;
			*found = true;
		}
		else
		{
			int i;

			/* c.) Get the successor nodes of p_current_node. */
			p_best->successors_of( p_current_node->state, &p_best->successors );

			/* d.) For each successor node S: */
			for( i = 0; i < successors_size(&p_best->successors); i++ )
			{
				const void* __restrict successor_state = successors_get( &p_best->successors, i );

				/* i.) If S is in the closed list, continue. */
				void* found_node;
				#ifdef USE_TREEMAP_FOR_CLOSEDLIST
				if( tree_map_find( &p_best->closed_list, successor_state, &found_node ) )
				#else
				if( hash_map_find( &p_best->closed_list, successor_state, &found_node ) )
				#endif
				{
					return;
				}

				/* ii.) If S is in open list: */
				if( hash_map_find( &p_best->open_hash_map, successor_state, &found_node ) )
				{
					bestfs_node_t* p_found_node = (bestfs_node_t*) found_node;
					/* If its heuristic is better, then update its
					 * heuristic with the better value and resort
					 * the open list.
					 */
					int h = p_best->heuristic( p_found_node->state, end );

					if( default_heuristic_compare( h, p_found_node->h ) < 0 ) // TODO: Expose heuristic compare
					{
						p_found_node->h      = h;
						p_found_node->parent = p_current_node;

						pbheap_reheapify( &p_best->open_list );
					}
				}
				else /* iii.) If S is not in the open list, then add S to the open list. */
				{
					bestfs_node_t* p_new_node = (bestfs_node_t*) p_best->alloc( sizeof(bestfs_node_t) );
					p_new_node->parent     = p_current_node;
					p_new_node->h          = p_best->heuristic( successor_state, end );
					p_new_node->state      = successor_state;

					pbheap_push( &p_best->open_list, p_new_node );
					hash_map_insert( &p_best->open_hash_map, p_new_node->state, p_new_node );

					#ifdef DEBUG_BEST_FIRST_SEARCH
					p_best->allocations++;
					#endif
				}
			} /* for */

			successors_clear( &p_best->successors );
		}

		/* e.) Add p_current_node to the closed list. */
		#ifdef USE_TREEMAP_FOR_CLOSEDLIST
		tree_map_insert( &p_best->closed_list, p_current_node->state, p_current_node );
		#else
		hash_map_insert( &p_best->closed_list, p_current_node->state, p_current_node );
		#endif
	}

	#ifdef DEBUG_BEST_FIRST_SEARCH
	if( *found )
	{
		bench_mark_end( p_best->bm );
		bench_mark_report( p_best->bm );
	}
	#endif
}

bool bestfs_iterative_is_done( bestfs_t* __restrict p_best, bool* __restrict found )
{
	return *found || pbheap_size(&p_best->open_list) == 0;
}
