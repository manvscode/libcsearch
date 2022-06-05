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
#include <stdlib.h>
#include <assert.h>
#include "list.h"

void list_create( list_t *p_list, alloc_fxn_t alloc, free_fxn_t free )
{
	assert( p_list );

	p_list->head    = NULL;
	p_list->tail    = NULL;
	p_list->size    = 0;

	p_list->alloc = alloc;
	p_list->free  = free;
}

void list_destroy( list_t *p_list )
{
	list_clear( p_list );

	#ifdef _DLIST_DEBUG
	p_list->head    = NULL;
	p_list->tail    = NULL;
	p_list->size    = 0;
	#endif
}

int list_insert_front( list_t *p_list, const void *data ) /* O(1) */
{
	list_node_t *p_node;
	assert( p_list );

	p_node = p_list->alloc( sizeof(list_node_t) );
	assert( p_node );

	if( p_node != NULL )
	{
		if( p_list->head )
		{
			p_list->head->prev = p_node;
		}

		p_node->data = (void *) data;
		p_node->next = p_list->head;
		p_node->prev = NULL;

		if( p_node->next == NULL )
		{
			p_list->tail = p_node;
		}

		p_list->head = p_node;
		p_list->size++;
		return 1;
	}

	return 0;
}

int list_remove_front( list_t *p_list ) /* O(1) */
{
	list_node_t *p_node;
	int result = 1;

	assert( p_list );
	assert( list_size(p_list) >= 1 );

	p_node = p_list->head->next;

	if( p_node )
	{
		p_node->prev = NULL;

		if( p_node->next == NULL )
		{
			p_list->tail = p_node;
		}
	}
	else
	{
		p_list->tail = NULL;
	}

	p_list->free( p_list->head );

	p_list->head = p_node;
	p_list->size--;

	return result;
}

int list_insert_back( list_t *p_list, const void *data ) /* O(1) */
{
	list_node_t *p_node;
	assert( p_list );

	p_node = p_list->alloc( sizeof(list_node_t) );
	assert( p_node );

	if( p_node != NULL )
	{
		if( p_list->tail )
		{
			p_list->tail->next = p_node;
		}
		else
		{
			p_list->head = p_node;
		}

		p_node->data = (void *) data;
		p_node->next = NULL;
		p_node->prev = p_list->tail;

		p_list->tail = p_node;
		p_list->size++;
		return 1;
	}

	return 0;
}

int list_remove_back( list_t *p_list ) /* O(1) */
{
	list_node_t *p_node;
	int result = 1;

	assert( p_list );
	assert( list_size(p_list) >= 1 );

	p_node = p_list->tail->prev;

	if( p_node )
	{
		p_node->next = NULL;

		if( p_node->prev == NULL )
		{
			p_list->head = p_node;
		}
	}
	else
	{
		p_list->head = NULL;
	}

	p_list->free( p_list->tail );

	p_list->tail = p_node;
	p_list->size--;

	return result;
}

int list_insert_next( list_t *p_list, list_node_t *p_front_node, const void *data ) /* O(1) */
{
	assert( p_list );
	assert( p_front_node );

	if( p_front_node )
	{
		list_node_t *p_node = p_list->alloc( sizeof(list_node_t) );
		assert( p_node );

		if( p_node != NULL )
		{
			p_node->data = (void *) data;
			p_node->next = p_front_node->next;
			p_node->prev = p_front_node;

			if( p_front_node->next )
			{
				p_front_node->next->prev = p_node;
			}
			else
			{
				p_list->tail = p_node;
			}

			p_front_node->next = p_node;
			p_list->size++;
			return 1;
		}

		return 0;
	}

	return list_insert_front( p_list, data );
}

int list_remove_next( list_t *p_list, list_node_t *p_front_node ) /* O(1) */
{
	assert( p_list );
	assert( list_size(p_list) >= 1 );

	if( p_front_node )
	{
		int result;
		list_node_t *p_node;
		list_node_t *p_new_next;

		assert( p_front_node->next );
		result     = 1;
		p_node     = p_front_node->next;
		p_new_next = p_node->next;

		if( p_new_next )
		{
			p_new_next->prev = p_front_node;
		}
		else
		{
			p_list->tail = p_front_node;
		}

		p_list->free( p_node );

		p_front_node->next = p_new_next;
		p_list->size--;

		return result;
	}

	return list_remove_front( p_list );
}

void list_clear( list_t *p_list )
{
	while( list_head(p_list) )
	{
		list_remove_front( p_list );
	}
}

void list_alloc_set( list_t *p_list, alloc_fxn_t alloc )
{
	assert( p_list );
	assert( alloc );
	p_list->alloc = alloc;
}

void list_free_set( list_t *p_list, free_fxn_t free )
{
	assert( p_list );
	assert( free );
	p_list->free = free;
}

list_iterator_t list_begin( const list_t *p_list )
{
	assert( p_list );
	return p_list->head;
}

list_iterator_t list_rbegin( const list_t *p_list )
{
	assert( p_list );
	return p_list->tail;
}

list_iterator_t list_next( const list_iterator_t iter )
{
	assert( iter );
	return iter->next;
}

list_iterator_t list_previous( const list_iterator_t iter )
{
	assert( iter );
	return iter->prev;
}

