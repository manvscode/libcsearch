#include <stdlib.h>
#include <math.h>
#include "heuristics.h"

unsigned int manhattan_distance( const coordinate_t *c1, const coordinate_t *c2 )
{
	return abs( (int) (c1->x - c2->x) ) + abs( (int) (c1->y - c2->y) );
}

unsigned int euclidean_distance( const coordinate_t *c1, const coordinate_t *c2 )
{
	return (int) sqrtf( (float) ((c1->x - c2->x) * (c1->x - c2->x) + (c1->y - c2->y) * (c1->y - c2->y)) );
}
