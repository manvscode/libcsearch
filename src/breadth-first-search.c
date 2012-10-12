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
#include <libcollections/hash-map.h>
#include <libcollections/tree-map.h>
#include <libcollections/bench-mark.h>
#include "successors-private.h"
#include "list.h"
#include "csearch.h"

struct breadthfs_node {
	struct breadthfs_node* parent;
	const void* state; /* vertex */
};

struct breadthfs_algorithm {
	alloc_fxn alloc;
	free_fxn  free;

	compare_fxn       compare;
	successors_fxn    successors_of;
	breadthfs_node_t* node_path;

	successors_t  successors;
	list_t        open_list; /* list of breadthfs_node_t* */
	lc_hash_map_t open_hash_map; /* (state, breadthfs_node_t*) */
	lc_tree_map_t closed_list; /* (state, breadthfs_node_t*) */

	#ifdef DEBUG_BREADTH_FIRST_SEARCH
	size_t       allocations;
	lc_bench_mark_t bm;
	#endif
};


static boolean nop_keyval_fxn( void* __restrict key, void* __restrict value )
{
	return TRUE;
}

static int pointer_compare( const void* __restrict p_n1, const void* __restrict p_n2 )
{
	ptrdiff_t diff = (unsigned char*) p_n1 - (unsigned char*) p_n2;
	return (int) diff;
}

breadthfs_t* breadthfs_create( compare_fxn compare, state_hash_fxn state_hasher, successors_fxn successors_of, alloc_fxn alloc, free_fxn free )
{
	breadthfs_t* p_bfs = (breadthfs_t*) alloc( sizeof(breadthfs_t) );

	if( p_bfs )
	{
		p_bfs->alloc         = alloc;
		p_bfs->free          = free;
		p_bfs->compare       = compare;
		p_bfs->successors_of = successors_of;
		p_bfs->node_path     = NULL;
		#ifdef DEBUG_BREADTH_FIRST_SEARCH
		p_bfs->allocations   = 0;
		p_bfs->bm            = bench_mark_create( "Breadth First Search Algorithm" );
		#endif

		successors_create( &p_bfs->successors, 8, alloc, free );
		list_create( &p_bfs->open_list, alloc, free );
		hash_map_create( &p_bfs->open_hash_map, HASH_MAP_SIZE_MEDIUM,
						 state_hasher, nop_keyval_fxn, pointer_compare,
						 alloc, free );
		tree_map_create( &p_bfs->closed_list, nop_keyval_fxn, pointer_compare, alloc, free );
	}

	return p_bfs;
}

void breadthfs_destroy( breadthfs_t** p_bfs )
{
	if( p_bfs && *p_bfs )
	{
		#ifdef DEBUG_BEST_FIRST_SEARCH
		bench_mark_destroy( (*p_bfs)->bm );
		#endif

		breadthfs_cleanup( *p_bfs );
		successors_destroy( &(*p_bfs)->successors );
		list_destroy( &(*p_bfs)->open_list );
		tree_map_destroy( &(*p_bfs)->closed_list );

		free_fxn _free = (*p_bfs)->free;
		_free( *p_bfs );
		*p_bfs = NULL;
	}
}

void breadthfs_set_compare_fxn( breadthfs_t* p_bfs, compare_fxn compare )
{
	if( p_bfs )
	{
		assert( compare );
		p_bfs->compare = compare;
	}
}

void breadthfs_set_successors_fxn( breadthfs_t* p_bfs, successors_fxn successors_of )
{
	if( p_bfs )
	{
		assert( successors_of );
		p_bfs->successors_of = successors_of;
	}
}

/*
 * Breadth First Search Algorithm
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
 *            ii.) If S is in open list, continue
 *            iii.) If S is not in the open list, then add S to the open list.
 *        e.) Add N to the closed list.
 * 4.) Return false.
 */
bool breadthfs_find( breadthfs_t* __restrict p_bfs, const void* __restrict start, const void* __restrict end )
{
	breadthfs_node_t* p_node;
	int i;
	bool found = false;

	#ifdef DEBUG_BEST_FIRST_SEARCH
	bench_mark_start( p_bfs->bm );
	#endif

 	/* 1.) Set the open list and closed list to be empty. */
	breadthfs_cleanup( p_bfs );

	p_node         = (breadthfs_node_t*) p_bfs->alloc( sizeof(breadthfs_node_t) );
	p_node->parent = NULL;
	p_node->state  = start;

	#ifdef DEBUG_BREADTH_FIRST_SEARCH
	p_bfs->allocations++;
	#endif

 	/* 2.) Add the start node to the open list. */
	list_insert_back( &p_bfs->open_list, p_node );
	hash_map_insert( &p_bfs->open_hash_map, p_node->state, p_node );

 	/* 3.) While the open list is not empty, do the following: */
	while( !found && list_size(&p_bfs->open_list) > 0 )
	{
 		/* a.) Get a node from the open list, call it p_current_node. */
		breadthfs_node_t* p_current_node = list_front( &p_bfs->open_list )->data;
		list_remove_front( &p_bfs->open_list );
		hash_map_remove( &p_bfs->open_hash_map, p_current_node->state );

		/* b.) If p_current_node is the goal node, return true. */
		if( p_current_node->state == end )
		{
			p_bfs->node_path = p_current_node;
			found = true;
		}
		else
		{
			/* c.) Get the successor nodes of p_current_node. */
			p_bfs->successors_of( p_current_node->state, &p_bfs->successors );

			/* d.) For each successor node S: */
			for( i = 0; i < successors_size(&p_bfs->successors); i++ )
			{
				const void* successor_state = successors_get( &p_bfs->successors, i );

				/* i.) If S is not in the closed list, continue. */
				void* found_node;
				if( tree_map_find( &p_bfs->closed_list, successor_state, &found_node ) )
				{
					continue;
				}

				/* ii.) If S is in open list, continue */
				if( hash_map_find( &p_bfs->open_hash_map, successor_state, &found_node ) )
				{
					continue;
				}
				else /* iii.) If S is not in the open list, then add S to the open list. */
				{
					breadthfs_node_t* p_new_node = (breadthfs_node_t*) p_bfs->alloc( sizeof(breadthfs_node_t) );
					p_new_node->parent     = p_current_node;
					p_new_node->state      = successor_state;

					list_insert_back( &p_bfs->open_list, p_new_node );
					hash_map_insert( &p_bfs->open_hash_map, p_new_node->state, p_new_node );

					#ifdef DEBUG_BREADTH_FIRST_SEARCH
					p_bfs->allocations++;
					#endif
				}
			} /* for */

			successors_clear( &p_bfs->successors );
		}

		/* e.) Add p_current_node to the closed list. */
		tree_map_insert( &p_bfs->closed_list, p_current_node->state, p_current_node );
	}

	#ifdef DEBUG_BEST_FIRST_SEARCH
	bench_mark_end( p_bfs->bm );
	bench_mark_report( p_bfs->bm );
	#endif

	return found;
}

void breadthfs_cleanup( breadthfs_t* p_bfs )
{
	lc_hash_map_iterator_t open_itr;
	lc_tree_map_iterator_t closed_itr;

	assert( hash_map_size(&p_bfs->open_hash_map) == list_size(&p_bfs->open_list) );
	#ifdef DEBUG_BREADTH_FIRST_SEARCH
	size_t size2 = hash_map_size(&p_bfs->open_hash_map);
	size_t size3 = tree_map_size(&p_bfs->closed_list);
	assert( size2 + size3 == p_bfs->allocations );
	#endif

	successors_clear( &p_bfs->successors );

	p_bfs->node_path = NULL;

	hash_map_iterator( &p_bfs->open_hash_map, &open_itr );

	// free everything on the open list.
	while( hash_map_iterator_next( &open_itr ) )
	{
		bestfs_node_t* p_node = hash_map_iterator_value( &open_itr );
		p_bfs->free( p_node );
		#ifdef DEBUG_BREADTH_FIRST_SEARCH
		p_bfs->allocations--;
		#endif
	}

	// free everything on the closed list.
	for( closed_itr = tree_map_begin( &p_bfs->closed_list );
	     closed_itr != tree_map_end( );
	     closed_itr = tree_map_next( closed_itr ) )
	{
		p_bfs->free( closed_itr->value );
		#ifdef DEBUG_BREADTH_FIRST_SEARCH
		p_bfs->allocations--;
		#endif
	}

	// empty out the data structures
	list_clear( &p_bfs->open_list );
	hash_map_clear( &p_bfs->open_hash_map );
	tree_map_clear( &p_bfs->closed_list );
}

breadthfs_node_t* breadthfs_first_node( const breadthfs_t* p_bfs )
{
	assert( p_bfs );
	return p_bfs->node_path;
}

const void* breadthfs_state( const breadthfs_node_t* p_node )
{
	assert( p_node );
	return p_node->state;
}

breadthfs_node_t* breadthfs_next_node( const breadthfs_node_t* p_node )
{
	assert( p_node );
	return p_node->parent;
}

void breadthfs_iterative_init( breadthfs_t* __restrict p_bfs, const void* __restrict start, const void* __restrict end, bool* __restrict found )
{
	breadthfs_node_t* p_node;
	#ifdef DEBUG_BEST_FIRST_SEARCH
	bench_mark_start( p_bfs->bm );
	#endif
	*found = false;

 	/* 1.) Set the open list and closed list to be empty. */
	breadthfs_cleanup( p_bfs );

	p_node         = (breadthfs_node_t*) p_bfs->alloc( sizeof(breadthfs_node_t) );
	p_node->parent = NULL;
	p_node->state  = start;

	#ifdef DEBUG_BREADTH_FIRST_SEARCH
	p_bfs->allocations++;
	#endif

 	/* 2.) Add the start node to the open list. */
	list_insert_back( &p_bfs->open_list, p_node );
	hash_map_insert( &p_bfs->open_hash_map, p_node->state, p_node );
}

void breadthfs_iterative_find( breadthfs_t* __restrict p_bfs, const void* __restrict start, const void* __restrict end, bool* __restrict found )
{
 	/* 3.) While the open list is not empty, do the following: */
	if( !*found && list_size(&p_bfs->open_list) > 0 )
	{
 		/* a.) Get a node from the open list, call it p_current_node. */
		breadthfs_node_t* p_current_node = list_front( &p_bfs->open_list )->data;
		list_remove_front( &p_bfs->open_list );
		hash_map_remove( &p_bfs->open_hash_map, p_current_node->state );

		/* b.) If p_current_node is the goal node, return true. */
		if( p_current_node->state == end )
		{
			p_bfs->node_path = p_current_node;
			*found = true;
		}
		else
		{
			int i;

			/* c.) Get the successor nodes of p_current_node. */
			p_bfs->successors_of( p_current_node->state, &p_bfs->successors );

			/* d.) For each successor node S: */
			for( i = 0; i < successors_size(&p_bfs->successors); i++ )
			{
				const void* successor_state = successors_get( &p_bfs->successors, i );

				/* i.) If S is not in the closed list, continue. */
				void* found_node;
				if( tree_map_find( &p_bfs->closed_list, successor_state, &found_node ) )
				{
					return;
				}

				/* ii.) If S is in open list, continue */
				if( hash_map_find( &p_bfs->open_hash_map, successor_state, &found_node ) )
				{
					return;
				}
				else /* iii.) If S is not in the open list, then add S to the open list. */
				{
					breadthfs_node_t* p_new_node = (breadthfs_node_t*) p_bfs->alloc( sizeof(breadthfs_node_t) );
					p_new_node->parent     = p_current_node;
					p_new_node->state      = successor_state;

					list_insert_back( &p_bfs->open_list, p_new_node );
					hash_map_insert( &p_bfs->open_hash_map, p_new_node->state, p_new_node );

					#ifdef DEBUG_BREADTH_FIRST_SEARCH
					p_bfs->allocations++;
					#endif
				}
			} /* for */

			successors_clear( &p_bfs->successors );
		}

		/* e.) Add p_current_node to the closed list. */
		tree_map_insert( &p_bfs->closed_list, p_current_node->state, p_current_node );
	}

	#ifdef DEBUG_BEST_FIRST_SEARCH
	if( *found )
	{
		bench_mark_end( p_bfs->bm );
		bench_mark_report( p_bfs->bm );
	}
	#endif
}

bool breadthfs_iterative_is_done( breadthfs_t* __restrict p_bfs, bool* __restrict found )
{
	return *found || list_size(&p_bfs->open_list) == 0;
}
