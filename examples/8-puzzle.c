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
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <csearch.h>
#include <heuristics.h>
#include <collections/vector.h>
#include <collections/hash-functions.h>

#define BOARD_WIDTH   3
#define BOARD_HEIGHT  3


const int GOAL_STATE[] = {
	1, 2, 3,
	4, 5, 6,
	7, 8, 0
};

/*
 * A collection of all of the game board
 * states that was a possibility.
 */
int** states;

static void randomize_board    ( int board[], size_t width, size_t height, bool only_solvable );
static int* create_state       ( int *board, size_t size, size_t index, size_t move_index );
static void get_possible_moves ( const void* restrict state, successors_t* restrict p_successors );
static void draw_board         ( int step, const int *board );
static int  heuristic          ( const void* restrict state1, const void* restrict state2 );
static int  cost               ( const void* restrict state1, const void* restrict state2 );
static int  board_compare      ( const void* restrict state1, const void* restrict state2 );


int main( int argc, char *argv[] )
{
	srand( time(NULL) );
	lc_vector_create( states, 100 );
	astar_t* p_astar = astar_create( board_compare, lc_pointer_hash, heuristic, cost, get_possible_moves, malloc, free );

	/* Produce a solvable random board */
	int* initial_state = (int*) malloc( sizeof(int) * BOARD_WIDTH * BOARD_HEIGHT );
	randomize_board( initial_state, BOARD_WIDTH, BOARD_HEIGHT, true );
	lc_vector_push( states, initial_state );


	if( astar_find( p_astar, GOAL_STATE, initial_state ) )
	{
		int step = 0;
		for( astar_node_t* p_node = astar_first_node( p_astar );
			 p_node != NULL;
			 p_node = astar_next_node( p_node ) )
		{
			const int* board = astar_state( p_node );

			draw_board( step, board );
			printf("\n");

			step++;
		}

		astar_cleanup( p_astar );
	}
	else
	{
		printf( "No solution found for:\n\n" );
		draw_board( 0, initial_state );
		/* No solution found. */
	}

	astar_destroy( &p_astar );

	/* Release memory of all the possible states. */
	while( lc_vector_size( states ) > 0 )
	{
		int* state = lc_vector_last( states );

		free( state );
		lc_vector_pop( states );
	}

	lc_vector_destroy( states );
	return 0;
}

/*
 * Generate a random game board.
 */
void randomize_board( int board[], size_t width, size_t height, bool only_solvable )
{
	if( !only_solvable )
	{
		/* This may or may not produce a board with a solution. */
		size_t len = width * height;
		int *numbers = (int*) malloc( sizeof(int) * len );

		for( int i = 0; i < len; i++ )
		{
			numbers[ i ] = i;
		}

		for( int i = len - 1; i >= 0; i-- )
		{
			size_t random_index = rand() % (i + 1);
			int chosen = numbers[ random_index ];

			numbers[ random_index ] = numbers[ i ];
			board[ i ] = chosen;
		}

		free( numbers );
	}
	else
	{
		/* Start with the goal */
		int current_board[ BOARD_WIDTH * BOARD_HEIGHT ];
		memcpy( current_board, GOAL_STATE, sizeof(int) * BOARD_WIDTH * BOARD_HEIGHT );

		int potential_moves[ 4 ][ 2 ] = {
			{ -1,  0 },
			{  1,  0 },
			{  0, -1 },
			{  0,  1 },
		};

		/* Attempt to make 14 moves to scramble the goal state.
		 * This new state will be the start state.
		 */
		int moves = 14;
		while( moves-- > 0 )
		{
			for( int y = 0; y < height; y++ )
			{
				for( int x = 0; x < width; x++ )
				{
					size_t index = width * y + x;

					/* loop until you find the empty space to make moves */
					if( current_board[ index ] == 0 )
					{
						bool moved = false;

						while( !moved )
						{
							size_t m = rand() % 4;
							int move_x = x + potential_moves[ m ][ 0 ];
							int move_y = y + potential_moves[ m ][ 1 ];

							if( move_x >= 0 && move_x < width &&
									move_y >= 0 && move_y < height )
							{
								/* make the move */
								size_t move_index = width * move_y + move_x;

								int tmp = current_board[ index ];
								current_board[ index ] = current_board[ move_index ];
								current_board[ move_index ] = tmp;

								moved = true;
							}
						}
					}
				}
			}
		}

		/* copy randomized board to the game board */
		size_t len = width * height;
		for( int i = 0; i < len; i++ )
		{
			board[ i ] = current_board[ i ];
		}
	}
}

/*
 * Create a new game board state.
 */
int* create_state( int *board, size_t size, size_t index, size_t move_index )
{
	int* new_board = (int*) malloc( sizeof(int) * size );

	if( new_board )
	{
		memcpy( new_board, board, sizeof(int) * size );

		int tmp = new_board[ index ];
		new_board[ index ] = new_board[ move_index ];
		new_board[ move_index ] = tmp;

		lc_vector_push( states, new_board );
	}

	return new_board;
}

/*
 * Given a game board state, what are all of the possible
 * game boards that can result from all of the potential
 * moves.
 */
void get_possible_moves( const void* restrict state, successors_t* restrict p_successors )
{
	int* current_board = (int*) state;

	int potential_moves[ 4 ][ 2 ] = {
		{ -1,  0 },
		{  1,  0 },
		{  0, -1 },
		{  0,  1 },
	};

	int emptyX;
	int emptyY;
	size_t index;
	bool found_empty = false;
	for( int y = 0; !found_empty && y < BOARD_HEIGHT; y++ )
	{
		for( int x = 0; !found_empty && x < BOARD_WIDTH; x++ )
		{
			index = BOARD_WIDTH * y + x;

			/* loop until you find the empty space to make moves */
			if( current_board[ index ] == 0 )
			{
				found_empty = true;
				emptyX = x;
				emptyY = y;
			}
		}
	}

	if( found_empty )
	{
		for( int m = 0; m < 4; m++ )
		{
			int move_x = emptyX + potential_moves[ m ][ 0 ];
			int move_y = emptyY + potential_moves[ m ][ 1 ];

			if( move_x >= 0 && move_x < BOARD_WIDTH &&
				move_y >= 0 && move_y < BOARD_HEIGHT )
			{
				/* make the move */
				size_t move_index = BOARD_WIDTH * move_y + move_x;

				int* new_state = create_state( current_board, BOARD_WIDTH * BOARD_HEIGHT, index, move_index );

				successors_push( p_successors, new_state );
			}
		}
	}
}

/*
 * Draw a game board state. Step 0 implies no moves have
 * occurred and is our initial state.
 */
void draw_board( int step, const int *board )
{
	for( int y = 0; y < BOARD_HEIGHT; y++ )
	{
		if( y == 1 )
		{
			if( step == 0 )
			{
				printf( " %10s     ",  "Initial" );
			}
			else
			{
				printf( " %10s %-3d ",  "Step", step );
			}
		}
		else
		{
			printf( "                " );
		}

		for( int x = 0; x < BOARD_WIDTH; x++ )
		{
			size_t index = BOARD_WIDTH * y + x;
			int num      = board[ index ];

			if( num )
			{
				printf( "|%d", num );
			}
			else
			{
				printf( "| " );
			}
		}

		printf( "|\n" );
	}
}

/*
 * The sum of the manhattan distance of each number between
 * game boards.
 */
int heuristic( const void* restrict state1, const void* restrict state2 )
{
	int* board1 = (int*) state1;
	int* board2 = (int*) state2;
	int sum = 0;

	for( int y1 = 0; y1 < BOARD_HEIGHT; y1++ )
	{
		for( int x1 = 0; x1 < BOARD_WIDTH; x1++ )
		{
			size_t index1 = BOARD_WIDTH * y1 + x1;

			for( int y2 = 0; y2 < BOARD_HEIGHT; y2++ )
			{
				for( int x2 = 0; x2 < BOARD_WIDTH; x2++ )
				{
					size_t index2 = BOARD_WIDTH * y2 + x2;

					if( board1[ index1 ] == board2[ index2 ] )
					{
						sum += abs( x2 - x1 ) + abs( y2 - y1 );
					}
				}
			}
		}
	}

	return sum;
}

/*
 * The cost of making a move is 1.
 */
int cost( const void* restrict state1, const void* restrict state2 )
{
	#if 0
	int* board1 = (int*) state1;
	int* board2 = (int*) state2;
	#endif

	return 1;
}

/*
 * Two game board states are equal if every number is in the same
 * position in both.
 */
int board_compare( const void* restrict state1, const void* restrict state2 )
{
	return memcmp( state1, state2, sizeof(int) * BOARD_WIDTH * BOARD_HEIGHT );
}
