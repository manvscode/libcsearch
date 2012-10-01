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
#include <libcollections/dlist.h>
#include <libcollections/tree-map.h>
#include "csearch.h"

struct breadthfs_node {
	struct breadthfs_node* parent;
	int h;
	const void* state;
};

struct breadthfs_algorithm {
	compare_fxn        compare;
	successors_fxn     successors_of;
	breadthfs_node_t*  node_path;

	dlist_t                open_list; /* list of bfs_node_t* */
	tree_map_t             closed_list; /* (state, bfs_node_t*) */

	#ifdef _BFS_DEBUG
	size_t allocations;
	#endif
};

static boolean nop_fxn( void *data )
{
	return TRUE;
}

breadthfs_t* breadthdfs_create( compare_fxn compare, successors_fxn successors_of )
{
	breadthfs_t* p_bfs = (breadthfs_t*) malloc( sizeof(breadthfs_t) );

	if( p_bfs )
	{
		p_bfs->compare     = compare;
		p_bfs->successors_of = successors_of;
		p_bfs->node_path     = NULL;
		#ifdef _BFS_DEBUG
		p_bfs->allocations   = 0;
		#endif

		dlist_create( &p_bfs->open_list, nop_fxn, malloc, free );
		tree_map_create( &p_bfs->closed_list, nop_keyval_fxn, pointer_compare, malloc, free );
	}

	return p_bfs;
}

void breadthdfs_destroy( breadthfs_t** p_bfs )
{
	if( p_bfs && *p_bfs )
	{
		breadthdfs_cleanup( *p_bfs );
		dlist_destroy( &(*p_bfs)->open_list );		
		tree_map_destroy( &(*p_bfs)->closed_list );		
		free( *p_bfs );
		*p_bfs = NULL;
	}
}

void breadthdfs_set_compare_fxn( breadthfs_t* p_bfs, compare_fxn compare )
{
	if( p_bfs )
	{
		p_bfs->compare = compare;
	}
}

void breadthdfs_set_successors_fxn( breadthfs_t* p_bfs, successors_fxn successors_of )
{
	if( p_bfs )
	{
		p_bfs->successors_of = successors_of;
	}
}

boolean breadthdfs_find( breadthfs_t* p_bfs, const void* start, const void* end )
{
	// TODO
	return FALSE;
}

void breadthdfs_cleanup( breadthfs_t* p_bfs )
{
	// TODO
}

breadthfs_node_t* breadthdfs_first_node( const breadthfs_t* p_bfs )
{
	assert( p_bfs );
	return p_bfs->node_path;
}

const void* breadthdfs_state( const breadthfs_node_t* p_node )
{
	assert( p_node );
	return p_node->state;
}

breadthfs_node_t* breadthdfs_next_node( const breadthfs_node_t* p_node )
{
	assert( p_node );
	return p_node->parent;
}

