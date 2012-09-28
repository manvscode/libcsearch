#ifndef _GSEARCH_H_
#define _GSEARCH_H_


struct bfs_algorithm;
typedef struct bfs_algorithm bfs_t;

//struct astar_algorithm;
//typedef struct astar_algorithm astar_t;

//struct dijkstra_algorithm;
//typedef struct dijkstra_algorithm dijkstra_t;


bfs_t*   bfs_create   ( void );
void     bfs_destroy  ( bfs_t** p_bfs );


#endif /* _GSEARCH_H_ */
