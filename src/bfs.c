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
#include <assert.h>
#include <libcollections/alloc.h>
#include <libcollections/bheap.h>
#include <libcollections/hash-map.h>
#include <libcollections/tree-map.h>
#include "csearch.h"

struct bfs_node {
	struct bfs_node* parent;
	int h;
	const void* state;
};

struct bfs_algorithm {
	heuristic_fxn  heuristic;
	successors_fxn successors_of;
	bfs_node_t*    node_path;

	bheap_t        open_list; /* list of bfs_node_t* */
	hash_map_t     open_hash_map; /* (state, bfs_node_t*) */
	tree_map_t     closed_list; /* (state, bfs_node_t*) */
};

static boolean nop_fxn( void *data )
{
	return TRUE;
}
static boolean nop_keyval_fxn( void *key, void *value )
{
	return TRUE;
}

static int pointer_compare( const void* p_n1, const void* p_n2 )
{
	ptrdiff_t diff = (unsigned char*) p_n1 - (unsigned char*) p_n2;
	return (int) diff;
}

static int bfs_node_compare( const void* p_n1, const void* p_n2 )
{
	return ((bfs_node_t*)p_n1)->h > ((bfs_node_t*)p_n2)->h;
}


	
bfs_t* bfs_create( state_hash_fxn state_hasher, heuristic_fxn heuristic, successors_fxn successors_of )
{
	bfs_t* p_bfs = (bfs_t*) malloc( sizeof(bfs_t) );

	if( p_bfs )
	{
		p_bfs->heuristic     = heuristic;
		p_bfs->successors_of = successors_of;
		p_bfs->node_path     = NULL;

		bheap_create( &p_bfs->open_list, sizeof(bfs_node_t*)/*elem_size*/, 128, 
					  (heap_compare_function) bfs_node_compare, nop_fxn,
					  malloc, free );

		hash_map_create( &p_bfs->open_hash_map, HASH_MAP_SIZE_MEDIUM, 
						 state_hasher, nop_keyval_fxn, pointer_compare, 
						 malloc, free );

		tree_map_create( &p_bfs->closed_list, nop_keyval_fxn, pointer_compare, malloc, free );
	}

	return p_bfs;
}

void bfs_destroy( bfs_t** p_bfs )
{
	if( p_bfs && *p_bfs )
	{
		bfs_cleanup( *p_bfs );
		bheap_destroy( &(*p_bfs)->open_list );		
		hash_map_destroy( &(*p_bfs)->open_hash_map );		
		tree_map_destroy( &(*p_bfs)->closed_list );		
		free( *p_bfs );
		*p_bfs = NULL;
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
boolean bfs_find( bfs_t* p_bfs, const void* start, const void* end )
{
	int i;
	pvector_t successors;	
	pvector_create( &successors, 8, nop_fxn, malloc, free );

	bfs_node_t* p_node = (bfs_node_t*) malloc( sizeof(bfs_node_t) );
	p_node->parent = NULL;
	p_node->h      = p_bfs->heuristic( start, end );
	p_node->state  = start;

 	/* 1.) Set the open list and closed list to be empty. */
	bfs_cleanup( p_bfs );

 	/* 2.) Add the start node to the open list. */
	bheap_push( &p_bfs->open_list, p_node );
	hash_map_insert( &p_bfs->open_hash_map, p_node->state, p_node );

 	/* 3.) While the open list is not empty, do the following: */
	while( bheap_size(&p_bfs->open_list) > 0 )
	{
 		/* a.) Get a node from the open list, call it p_node. */
		bfs_node_t* p_node = bheap_peek( &p_bfs->open_list );
		bheap_pop( &p_bfs->open_list );
		hash_map_remove( &p_bfs->open_hash_map, p_node->state );

		/* b.) If p_node is the goal node, return true. */
		if( p_node->state == end )
		{
			p_bfs->node_path = p_node;
			return TRUE;
		}

		/* c.) Get the successor nodes of p_node. */
		p_bfs->successors_of( p_node->state, &successors ); 

		/* d.) For each successor node S: */
		for( i = 0; i < pvector_size(&successors); i++ )
		{
			bfs_node_t* s = pvector_get( &successors, i );

			/* i.) If S is in the closed list, continue. */
			void* found_node;
			if( tree_map_find( &p_bfs->closed_list, s->state, &found_node ) )
			{
				continue;
			}

			/* ii.) If S is in open list: */
			if( hash_map_find( &p_bfs->open_hash_map, s->state, &found_node ) )
			{
				bfs_node_t* p_found_node = (bfs_node_t*) found_node;
				/* If its heuristic is better, then update its 
				 * heuristic with the better value and resort 
				 * the open list.
				 */
				int h = p_bfs->heuristic( p_found_node->state, end );

				if( h < p_found_node->h )
				{
					p_found_node->h      = h;
					p_found_node->parent = p_node;

					bheap_reheapify( &p_bfs->open_list );
				}	
			}
			else /* iii.) If S is not in the open list, then add S to the open list. */
			{	
				bfs_node_t* p_new_node = (bfs_node_t*) malloc( sizeof(bfs_node_t) );
				p_new_node->parent     = p_node;
				p_new_node->h          = p_bfs->heuristic( s->state, end );
				p_new_node->state      = s->state;

				bheap_push( &p_bfs->open_list, p_new_node );
				hash_map_insert( &p_bfs->open_hash_map, p_new_node->state, p_new_node );
			}
		}

		/* e.) Add p_node to the closed list. */
		tree_map_insert( &p_bfs->closed_list, p_node->state, p_node );
	}

	return FALSE;
}

void bfs_cleanup( bfs_t* p_bfs )
{
	assert( hash_map_size(&p_bfs->open_hash_map) == bheap_size(&p_bfs->open_list) );
}

bfs_node_t* bfs_first_node( const bfs_t* p_bfs )
{
	assert( p_bfs );
	return p_bfs->node_path;
}

const void* bfs_state( const bfs_node_t* p_node )
{
	assert( p_node );
	return p_node->state;
}

bfs_node_t* bfs_next_node( const bfs_node_t* p_node )
{
	assert( p_node );
	return p_node->parent;
}
