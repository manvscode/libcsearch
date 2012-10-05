#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <libcollections/macros.h>
#include "heuristics.h"

unsigned int manhattan_distance( const coordinate_t *c1, const coordinate_t *c2 )
{
	#if 0
	return fast_abs( c1->x - c2->x ) + fast_abs( c1->y - c2->y );
	#else
	return abs( c1->x - c2->x ) + abs( c1->y - c2->y );
	#endif
}

unsigned int euclidean_distance( const coordinate_t *c1, const coordinate_t *c2 )
{
	return (int) sqrtf( (float) ((c1->x - c2->x) * (c1->x - c2->x) + (c1->y - c2->y) * (c1->y - c2->y)) );
}
