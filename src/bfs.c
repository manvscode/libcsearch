#include <stdlib.h>
#include "gsearch.h"
#include "libgsearch-config.h"

struct bfs_algorithm {
	void* state;
};

bfs_t* bfs_create( void )
{
	bfs_t* p_bfs = (bfs_t*) malloc( sizeof(bfs_t) );

	if( p_bfs )
	{
		// TODO: initialize
	}

	return p_bfs;
}

void bfs_destroy( bfs_t** p_bfs )
{
	if( p_bfs && *p_bfs )
	{
		free( *p_bfs );
		*p_bfs = NULL;
	}
}

