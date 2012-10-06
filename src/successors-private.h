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
#ifndef _SUCCESSORS_H_
#define _SUCCESSORS_H_
#ifdef __cplusplus
extern "C" {
#endif 

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <libcollections/types.h>
#include <libcollections/alloc.h>
#include "csearch.h"

/*
 * successors - A growable array of states.
 * successors does not own the states.
 */
struct successors {
	alloc_function  alloc;
	free_function   free;
	size_t array_size;
	size_t size;

	void** array;
};

bool successors_create  ( successors_t *p_successors, size_t size, alloc_function alloc, free_function free );
void    successors_destroy ( successors_t *p_successors );

#define successors_array( p_successors )       ((p_successors)->array)
#define successors_array_size( p_successors )  ((p_successors)->array_size)
#define successors_size( p_successors )        ((p_successors)->size)
#define successors_is_empty( p_successors )    ((p_successors)->size <= 0)
#define successors_peek( p_successors )        (successors_get(p_successors, successors_size(p_successors) - 1))
#define successors_clear( p_successors )       ((p_successors)->size = 0)


static inline void* successors_get( successors_t* restrict p_successors, size_t index )
{
	return p_successors->array[ index ];
}

static inline void successors_set( successors_t* restrict p_successors, size_t index, void* restrict p_data )
{
	p_successors->array[ index ] = p_data;
}

#ifdef __cplusplus
}
#endif 
#endif /* _SUCCESSORS_H_ */
