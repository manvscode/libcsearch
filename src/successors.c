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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "successors-private.h"
#include "csearch.h"

/*
 * successors - A growable array of states.
 * successors does not own the states.
 */
bool successors_create( successors_t* p_successors, size_t size, alloc_function alloc, free_function free )
{
	assert( p_successors );

	p_successors->array_size = size;
	p_successors->size       = 0L;
	p_successors->alloc      = alloc;
	p_successors->free       = free;
	p_successors->array      = p_successors->alloc( sizeof(void *) * successors_array_size(p_successors) );

	assert( p_successors->array );

	return p_successors->array != NULL;
}

void successors_destroy( successors_t* p_successors )
{
	assert( p_successors );

	while( !successors_is_empty(p_successors) )
	{
		successors_pop( p_successors );
	}

	p_successors->free( p_successors->array );

	#ifdef _DEBUG_VECTOR
	p_successors->array      = NULL;
	p_successors->array_size = 0L;
	p_successors->size       = 0L;
	#endif
}

bool successors_push( successors_t* restrict p_successors, const void* restrict state )
{
	assert( p_successors );

	/* grow the array if needed */
	if( successors_size(p_successors) >= successors_array_size(p_successors) )
	{
		size_t new_size          = 1.5 * p_successors->array_size + 1;
		p_successors->array_size = new_size;
		p_successors->array      = realloc( p_successors->array, sizeof(void*) * successors_array_size(p_successors) );
		assert( p_successors->array );
	}

	p_successors->array[ p_successors->size++ ] = (void*) state;

	return p_successors->array != NULL;
}

bool successors_pop( successors_t* p_successors )
{
	bool result = false;
	assert( p_successors );
	assert( successors_size(p_successors) > 0 );

	if( p_successors->size > 0 )
	{
		p_successors->size--;
		result = true;
	}

	return result;
}

bool successors_resize( successors_t* p_successors, size_t new_size )
{
	bool result = true;

	if( successors_size(p_successors) > new_size )
	{
		while( successors_size(p_successors) > new_size )
		{
			successors_pop( p_successors );
		}

		p_successors->array_size = new_size;
		p_successors->array      = realloc( p_successors->array, sizeof(void*) * successors_array_size(p_successors) );
		assert( p_successors->array );

		result = p_successors->array != NULL;
	}

	return result;
}

