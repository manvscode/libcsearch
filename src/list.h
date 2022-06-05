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
#ifndef _LIST_H_
#define _LIST_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "csearch.h"

typedef int (*list_element_function)( void *element );

typedef struct list_node {
	void* data;
	struct list_node* next;
	struct list_node* prev;
} list_node_t;

typedef struct list {
	list_node_t* head;
	list_node_t* tail;
	size_t size;

	alloc_fxn_t  alloc;
	free_fxn_t   free;
} list_t;

typedef list_node_t* list_iterator_t;

void list_create        ( list_t *p_list, alloc_fxn_t alloc, free_fxn_t free );
void list_destroy       ( list_t *p_list );
int  list_insert_front  ( list_t *p_list, const void *data ); /* O(1) */
int  list_remove_front  ( list_t *p_list ); /* O(1) */
int  list_insert_back   ( list_t *p_list, const void *data ); /* O(1) */
int  list_remove_back   ( list_t *p_list ); /* O(1) */
int  list_insert_next   ( list_t *p_list, list_node_t *p_front_node, const void *data ); /* O(1) */
int  list_remove_next   ( list_t *p_list, list_node_t *p_front_node ); /* O(1) */ 
void list_clear         ( list_t *p_list ); /* O(N) */

void list_alloc_set     ( list_t *p_list, alloc_fxn_t alloc );
void list_free_set      ( list_t *p_list, free_fxn_t free );

list_iterator_t list_begin    ( const list_t *p_list );
list_iterator_t list_rbegin   ( const list_t *p_list );
#define         list_end( )   ((list_iterator_t)NULL)
list_iterator_t list_next     ( const list_iterator_t iter );
list_iterator_t list_previous ( const list_iterator_t iter );


#define list_head(p_list)       ((p_list)->head)
#define list_front(p_list)      ((p_list)->head)
#define list_tail(p_list)       ((p_list)->tail)
#define list_back(p_list)       ((p_list)->tail)
#define list_size(p_list)       ((p_list)->size)
#define list_is_empty(p_list)   ((p_list)->size <= 0)

#ifdef __cplusplus
}
#endif 
#endif /* _LIST_H_ */
