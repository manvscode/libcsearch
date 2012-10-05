/*
 * Copyright (C) 2010 by Joseph A. Marrero and Shrewd LLC. http://www.manvscode.com/
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
#include <stdlib.h>
#include <assert.h>
#include <libcollections/types.h>
#include "list.h"



void list_create( list_t* p_list, alloc_function alloc, free_function free )
{
	assert( p_list );

	p_list->head    = NULL;
	p_list->size    = 0;

	p_list->alloc = alloc;
	p_list->free  = free;
}

void list_destroy( list_t* p_list ) 
{
	list_clear( p_list );

	#ifdef _SLIST_DEBUG
	p_list->head    = NULL;
	p_list->size    = 0;
	#endif
}

boolean list_insert_front( list_t* restrict p_list, const void* restrict data ) /* O(1) */ 
{
	list_node_t *p_node;
	assert( p_list );

	p_node = p_list->alloc( sizeof(list_node_t) );
	assert( p_node );

	if( p_node != NULL ) 
	{
		p_node->data = (void*)  data;
		p_node->next = p_list->head;

		p_list->head = p_node;
		p_list->size++;
		return TRUE;
	}

	return FALSE;
}

boolean list_remove_front( list_t* p_list ) /* O(1) */ 
{
	list_node_t *p_node;
	boolean result = TRUE;

	assert( p_list );
	assert( list_size(p_list) >= 1 );

	p_node = p_list->head->next;

	p_list->free( p_list->head );

	p_list->head = p_node;
	p_list->size--;

	return result;
}

boolean list_insert_next( list_t* restrict p_list, list_node_t* restrict p_front_node, const void* restrict data ) /* O(1) */ 
{
	assert( p_list );
	assert( p_front_node );

	if( p_front_node ) 
	{
		list_node_t *p_node = p_list->alloc( sizeof(list_node_t) );
		assert( p_node );

		if( p_node != NULL )
		{
			p_node->data = (void*) data;
			p_node->next = p_front_node->next;

			p_front_node->next = p_node;
			p_list->size++;
			return TRUE;
		}

		return FALSE;
	}

	return list_insert_front( p_list, data );	
}

boolean list_remove_next( list_t* restrict p_list, list_node_t* restrict p_front_node ) /* O(1) */
{
	assert( p_list );
	assert( list_size(p_list) >= 1 );

	if( p_front_node )
	{
		boolean result;
		list_node_t *p_node;
		list_node_t *p_new_next;

		assert( p_front_node->next );
		result     = TRUE;
		p_node     = p_front_node->next;
		p_new_next = p_node->next;

		p_list->free( p_node );

		p_front_node->next = p_new_next;
		p_list->size--;

		return result;
	}

	return list_remove_front( p_list );
}

void list_clear( list_t* p_list )
{
	while( list_head(p_list) )
	{
		list_remove_front( p_list );
	}
}

list_iterator_t list_begin( const list_t* p_list )
{
	assert( p_list );
	return p_list->head;
}

list_iterator_t list_next( const list_iterator_t iter )
{
	assert( iter );
	return iter->next;
}
