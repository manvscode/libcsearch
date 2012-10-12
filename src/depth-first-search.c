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

struct depthfs_node {
	struct depthfs_node* parent;
	const void* state; /* vertex */
};

struct depthfs_algorithm {
	alloc_fxn alloc;
	free_fxn  free;

	compare_fxn     compare;
	successors_fxn  successors_of;
	depthfs_node_t* node_path;

	successors_t successors;
	list_t       open_list; /* list of depthfs_node_t* */
	lc_hash_map_t   open_hash_map; /* (state, depthfs_node_t*) */
	lc_tree_map_t   closed_list; /* (state, depthfs_node_t*) */

	#ifdef DEBUG_DEPTH_FIRST_SEARCH
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

depthfs_t* depthfs_create( compare_fxn compare, state_hash_fxn state_hasher, successors_fxn successors_of, alloc_fxn alloc, free_fxn free )
{
	depthfs_t* p_dfs = (depthfs_t*) alloc( sizeof(depthfs_t) );

	if( p_dfs )
	{
		p_dfs->alloc         = alloc;
		p_dfs->free          = free;
		p_dfs->compare       = compare;
		p_dfs->successors_of = successors_of;
		p_dfs->node_path     = NULL;
		#ifdef DEBUG_DEPTH_FIRST_SEARCH
		p_dfs->allocations   = 0;
		p_dfs->bm            = bench_mark_create( "Depth First Search Algorithm" );
		#endif

		successors_create( &p_dfs->successors, 8, alloc, free );
		list_create( &p_dfs->open_list, alloc, free );
		hash_map_create( &p_dfs->open_hash_map, HASH_MAP_SIZE_MEDIUM,
						 state_hasher, nop_keyval_fxn, pointer_compare,
						 alloc, free );
		tree_map_create( &p_dfs->closed_list, nop_keyval_fxn, pointer_compare, alloc, free );
	}

	return p_dfs;
}

void depthfs_destroy( depthfs_t** p_dfs )
{
	if( p_dfs && *p_dfs )
	{
		#ifdef DEBUG_DEPTH_FIRST_SEARCH
		bench_mark_destroy( (*p_dfs)->bm );
		#endif

		depthfs_cleanup( *p_dfs );
		successors_destroy( &(*p_dfs)->successors );
		list_destroy( &(*p_dfs)->open_list );
		tree_map_destroy( &(*p_dfs)->closed_list );

		free_fxn _free = (*p_dfs)->free;
		_free( *p_dfs );
		*p_dfs = NULL;
	}
}

void depthfs_set_compare_fxn( depthfs_t* p_dfs, compare_fxn compare )
{
	if( p_dfs )
	{
		assert( compare );
		p_dfs->compare = compare;
	}
}

void depthfs_set_successors_fxn( depthfs_t* p_dfs, successors_fxn successors_of )
{
	if( p_dfs )
	{
		assert( successors_of );
		p_dfs->successors_of = successors_of;
	}
}

bool depthfs_find( depthfs_t* __restrict p_dfs, const void* __restrict start, const void* __restrict end )
{
	depthfs_node_t* p_node;
	#ifdef DEBUG_DEPTH_FIRST_SEARCH
	bench_mark_start( p_dfs->bm );
	#endif
	int i;
	bool found = false;

 	/* 1.) Set the open list and closed list to be empty. */
	depthfs_cleanup( p_dfs );

	p_node         = (depthfs_node_t*) p_dfs->alloc( sizeof(depthfs_node_t) );
	p_node->parent = NULL;
	p_node->state  = start;

	#ifdef DEBUG_DEPTH_FIRST_SEARCH
	p_dfs->allocations++;
	#endif

 	/* 2.) Add the start node to the open list. */
	list_insert_front( &p_dfs->open_list, p_node );
	hash_map_insert( &p_dfs->open_hash_map, p_node->state, p_node );

 	/* 3.) While the open list is not empty, do the following: */
	while( !found && list_size(&p_dfs->open_list) > 0 )
	{
 		/* a.) Get a node from the open list, call it p_current_node. */
		depthfs_node_t* p_current_node = list_front( &p_dfs->open_list )->data;
		list_remove_front( &p_dfs->open_list );
		hash_map_remove( &p_dfs->open_hash_map, p_current_node->state );

		/* b.) If p_current_node is the goal node, return true. */
		if( p_current_node->state == end )
		{
			p_dfs->node_path = p_current_node;
			found = true;
		}
		else
		{
			/* c.) Get the successor nodes of p_current_node. */
			p_dfs->successors_of( p_current_node->state, &p_dfs->successors );

			/* d.) For each successor node S: */
			for( i = 0; i < successors_size(&p_dfs->successors); i++ )
			{
				const void* successor_state = successors_get( &p_dfs->successors, i );

				/* i.) If S is not in the closed list, continue. */
				void* found_node;
				if( tree_map_find( &p_dfs->closed_list, successor_state, &found_node ) )
				{
					continue;
				}

				/* ii.) If S is in open list, continue */
				if( hash_map_find( &p_dfs->open_hash_map, successor_state, &found_node ) )
				{
					continue;
				}
				else /* iii.) If S is not in the open list, then add S to the open list. */
				{
					depthfs_node_t* p_new_node = (depthfs_node_t*) p_dfs->alloc( sizeof(depthfs_node_t) );
					p_new_node->parent     = p_current_node;
					p_new_node->state      = successor_state;

					list_insert_front( &p_dfs->open_list, p_new_node );
					hash_map_insert( &p_dfs->open_hash_map, p_new_node->state, p_new_node );

					#ifdef DEBUG_DEPTH_FIRST_SEARCH
					p_dfs->allocations++;
					#endif
				}
			} /* for */

			successors_clear( &p_dfs->successors );
		}

		/* e.) Add p_current_node to the closed list. */
		tree_map_insert( &p_dfs->closed_list, p_current_node->state, p_current_node );
	}

	#ifdef DEBUG_DEPTH_FIRST_SEARCH
	bench_mark_end( p_dfs->bm );
	bench_mark_report( p_dfs->bm );
	#endif

	return found;
}

void depthfs_cleanup( depthfs_t* p_dfs )
{
	lc_hash_map_iterator_t open_itr;
	lc_tree_map_iterator_t closed_itr;

	assert( hash_map_size(&p_dfs->open_hash_map) == list_size(&p_dfs->open_list) );
	#ifdef DEBUG_DEPTH_FIRST_SEARCH
	size_t size2 = hash_map_size(&p_dfs->open_hash_map);
	size_t size3 = tree_map_size(&p_dfs->closed_list);
	assert( size2 + size3 == p_dfs->allocations );
	#endif

	successors_clear( &p_dfs->successors );

	p_dfs->node_path = NULL;

	hash_map_iterator( &p_dfs->open_hash_map, &open_itr );

	// free everything on the open list.
	while( hash_map_iterator_next( &open_itr ) )
	{
		bestfs_node_t* p_node = hash_map_iterator_value( &open_itr );
		p_dfs->free( p_node );
		#ifdef DEBUG_DEPTH_FIRST_SEARCH
		p_dfs->allocations--;
		#endif
	}

	// free everything on the closed list.
	for( closed_itr = tree_map_begin( &p_dfs->closed_list );
	     closed_itr != tree_map_end( );
	     closed_itr = tree_map_next( closed_itr ) )
	{
		p_dfs->free( closed_itr->value );
		#ifdef DEBUG_DEPTH_FIRST_SEARCH
		p_dfs->allocations--;
		#endif
	}

	// empty out the data structures
	list_clear( &p_dfs->open_list );
	hash_map_clear( &p_dfs->open_hash_map );
	tree_map_clear( &p_dfs->closed_list );
}

depthfs_node_t* depthfs_first_node( const depthfs_t* p_dfs )
{
	assert( p_dfs );
	return p_dfs->node_path;
}

const void* depthfs_state( const depthfs_node_t* p_node )
{
	assert( p_node );
	return p_node->state;
}

depthfs_node_t* depthfs_next_node( const depthfs_node_t* p_node )
{
	assert( p_node );
	return p_node->parent;
}

void depthfs_iterative_init( depthfs_t* __restrict p_dfs, const void* __restrict start, const void* __restrict end, bool* found )
{
	depthfs_node_t* p_node;
	#ifdef DEBUG_DEPTH_FIRST_SEARCH
	bench_mark_start( p_dfs->bm );
	#endif
	*found = false;

 	/* 1.) Set the open list and closed list to be empty. */
	depthfs_cleanup( p_dfs );

	p_node         = (depthfs_node_t*) p_dfs->alloc( sizeof(depthfs_node_t) );
	p_node->parent = NULL;
	p_node->state  = start;

	#ifdef DEBUG_DEPTH_FIRST_SEARCH
	p_dfs->allocations++;
	#endif

 	/* 2.) Add the start node to the open list. */
	list_insert_front( &p_dfs->open_list, p_node );
	hash_map_insert( &p_dfs->open_hash_map, p_node->state, p_node );
}

void depthfs_iterative_find( depthfs_t* __restrict p_dfs, const void* __restrict start, const void* __restrict end, bool* found )
{
 	/* 3.) While the open list is not empty, do the following: */
	if( !*found && list_size(&p_dfs->open_list) > 0 )
	{
 		/* a.) Get a node from the open list, call it p_current_node. */
		depthfs_node_t* p_current_node = list_front( &p_dfs->open_list )->data;
		list_remove_front( &p_dfs->open_list );
		hash_map_remove( &p_dfs->open_hash_map, p_current_node->state );

		/* b.) If p_current_node is the goal node, return true. */
		if( p_current_node->state == end )
		{
			p_dfs->node_path = p_current_node;
			*found = true;
		}
		else
		{
			int i;

			/* c.) Get the successor nodes of p_current_node. */
			p_dfs->successors_of( p_current_node->state, &p_dfs->successors );

			/* d.) For each successor node S: */
			for( i = 0; i < successors_size(&p_dfs->successors); i++ )
			{
				const void* successor_state = successors_get( &p_dfs->successors, i );

				/* i.) If S is not in the closed list, continue. */
				void* found_node;
				if( tree_map_find( &p_dfs->closed_list, successor_state, &found_node ) )
				{
					return;
				}

				/* ii.) If S is in open list, continue */
				if( hash_map_find( &p_dfs->open_hash_map, successor_state, &found_node ) )
				{
					return;
				}
				else /* iii.) If S is not in the open list, then add S to the open list. */
				{
					depthfs_node_t* p_new_node = (depthfs_node_t*) p_dfs->alloc( sizeof(depthfs_node_t) );
					p_new_node->parent     = p_current_node;
					p_new_node->state      = successor_state;

					list_insert_front( &p_dfs->open_list, p_new_node );
					hash_map_insert( &p_dfs->open_hash_map, p_new_node->state, p_new_node );

					#ifdef DEBUG_DEPTH_FIRST_SEARCH
					p_dfs->allocations++;
					#endif
				}
			} /* for */

			successors_clear( &p_dfs->successors );
		}

		/* e.) Add p_current_node to the closed list. */
		tree_map_insert( &p_dfs->closed_list, p_current_node->state, p_current_node );
	}

	#ifdef DEBUG_DEPTH_FIRST_SEARCH
	if( *found )
	{
		bench_mark_end( p_dfs->bm );
		bench_mark_report( p_dfs->bm );
	}
	#endif
}

bool depthfs_iterative_is_done( depthfs_t* __restrict p_dfs, bool* found )
{
	return *found || list_size(&p_dfs->open_list) == 0;
}
