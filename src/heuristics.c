#include <stdlib.h>
#include <math.h>
#include "heuristics.h"

unsigned long manhattan_distance( const coordinate_t *c1, const coordinate_t *c2 )
{
	return (unsigned long) abs( (long) (c1->x - c2->x) ) + abs( (long) (c1->y - c2->y) );
}

unsigned long euclidean_distance( const coordinate_t *c1, const coordinate_t *c2 )
{
	return (unsigned long) sqrt( (float) ((c1->x - c2->x) * (c1->x - c2->x) + (c1->y - c2->y) * (c1->y - c2->y)) );
}
