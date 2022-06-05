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
#ifdef __APPLE__
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif
#include <GL/freeglut.h>
//#include <collections/types.h>
#include <collections/hash-functions.h>
#include <collections/vector.h>
#include <csearch.h>
#include <heuristics.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

extern void* pvector_get( lc_pvector_t* p_vector, size_t index );

#define ESC_KEY			27

void initialize         ( void );
void deinitialize       ( void );
void draw_tiles         ( void );
void draw_path          ( const lc_pvector_t* restrict path, GLfloat color[] );
void render             ( void );
void resize             ( int width, int height );
void keyboard_keypress  ( unsigned char key, int x, int y );
void mouse              ( int button, int state, int x, int y );
void mouse_motion       ( int x, int y );
void idle               ( void );
void write_text         ( void *font, const char* text, int x, int y, float r, float g, float b );
void reset              ( boolean bRandomize );

static void tile_successors4           ( const void* restrict state, successors_t* restrict p_successors );
static void tile_successors8           ( const void* restrict state, successors_t* restrict p_successors );
static int tile_manhattan_distance     ( const void* restrict t1, const void* restrict t2 );
static int tile_euclidean_distance     ( const void* restrict t1, const void* restrict t2 );
static unsigned int tile_positive_cost ( const void* restrict t1, const void* restrict t2 );
static int tile_cost                   ( const void* restrict t1, const void* restrict t2 );

int windowWidth;
int windowHeight;
float tileWidth;
float tileHeight;

#define DEFAULT_GRIDWIDTH		  	300
#define DEFAULT_GRIDHEIGHT			300

GLfloat grid[ 2 ][ 2 ][ 3 ] = {
		{ {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
		{ {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f} }
	};


typedef struct tile {
	coordinate_t position;
	boolean      is_walkable;

	// stuff for BFS...
	int cost;

	// stuff for A*
	int F;
	int G;
	int H;
} tile_t;

tile_t *tiles = NULL;
unsigned int blockList = 0, gridList = 0;


unsigned int gridWidth  = DEFAULT_GRIDWIDTH;
unsigned int gridHeight = DEFAULT_GRIDHEIGHT;

coordinate_t start;
coordinate_t end;


lc_pvector_t bestfs_path;
lc_pvector_t dijkstra_path;
lc_pvector_t astar_path;

bestfs_t*   bfs;
dijkstra_t* dijkstra;
astar_t*    ass;

int main( int argc, char *argv[] )
{
	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );

	if( argc != 1 && (argc - 1) % 2 != 0 )
	{
		printf( "Usage: %s -gw <Grid Width> -gh <Grid Height>\n\n", argv[0] );
		printf( "The -gw and -gh options are optional. If they are omitted than the\n" );
		printf( "defaults are used (%d x %d).\n", gridWidth, gridHeight );
		return 1;
	}


	for( int c = 1; c < argc; c += 2 )
	{
		if( strncmp( argv[ c ], "-gw", 3 ) == 0 )
			gridWidth = strtoul( argv[ c + 1 ], NULL, 0 );
		else if( strncmp( argv[ c ], "-gh", 3 ) == 0 )
			gridHeight = strtoul( argv[ c + 1 ], NULL, 0 );
	}


#ifndef FULLSCREEN
	glutInitWindowSize( 800, 600 );
	int windowId = glutCreateWindow( "[Path Finding Demo] - http://www.ManVsCode.com/" );
#else
	glutGameModeString( "640x480" );

	if( glutGameModeGet( GLUT_GAME_MODE_POSSIBLE ) )
		glutEnterGameMode( );
	else
	{
		glutGameModeString( "640x480" );
		if( glutGameModeGet( GLUT_GAME_MODE_POSSIBLE ) )
			glutEnterGameMode( );
		else
		{
			cerr << "The requested mode is not available!" << endl;
			return -1;
		}
	}
#endif


	glutDisplayFunc( render );
	glutReshapeFunc( resize );
	glutKeyboardFunc( keyboard_keypress );
	//glutIdleFunc( idle );
	glutMouseFunc( mouse );
	glutMotionFunc( mouse_motion );


	glutSetOption( GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS );

	initialize( );
	glutMainLoop( );
#ifndef FULLSCREEN
	glutDestroyWindow( windowId );
#endif
	deinitialize( );

	return 0;
}
static int pointer_compare( const void* left, const void* right )
{
	return ((unsigned char*)left) - ((unsigned char*)right);
}

void initialize( void )
{
	lc_vector_create( &bestfs_path, 1 );
	lc_vector_create( &dijkstra_path, 1 );
	lc_vector_create( &astar_path, 1 );

	bfs      = bestfs_create   ( pointer_compare, pointer_hash, tile_manhattan_distance, tile_successors4, malloc, free );
	dijkstra = dijkstra_create ( pointer_compare, pointer_hash, tile_positive_cost, tile_successors4, malloc, free );
	ass      = astar_create    ( pointer_compare, pointer_hash, tile_manhattan_distance, tile_cost, tile_successors4, malloc, free );

	glDisable( GL_DEPTH_TEST );

	glEnable( GL_POINT_SMOOTH );
	glEnable( GL_LINE_SMOOTH );
	glHint( GL_POINT_SMOOTH_HINT, GL_FASTEST );
	glHint( GL_LINE_SMOOTH_HINT, GL_FASTEST );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );


	glShadeModel( GL_FLAT );
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );


	glPixelStorei( GL_PACK_ALIGNMENT, 4 );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );


	// TO DO: Initialization code goes here...
	glEnable( GL_MAP2_VERTEX_3 );
	glEnable( GL_LINE_STIPPLE );


	tiles = (tile_t*) malloc( sizeof(tile_t) * gridWidth * gridHeight );

	blockList = glGenLists( 2 );
	glNewList( blockList, GL_COMPILE );
		glBegin( GL_QUADS );
			glVertex2f( 0.0f, 0.0f );
			glVertex2f( 1.0f, 0.0f );
			glVertex2f( 1.0f, 1.0f );
			glVertex2f( 0.0f, 1.0f );
		glEnd( );
	glEndList( );


	gridList = blockList + 1;
	glNewList( gridList, GL_COMPILE );
		glPushAttrib( GL_CURRENT_BIT | GL_LINE_BIT );
			glEnable( GL_LINE_STIPPLE );
			glLineStipple( 1, 0xF0F0 );
			glLineWidth( 0.1f );
			glColor3f( 0.2f, 1.0f, 1.0f );

			glMapGrid2f( gridWidth, 0.0, 1.0, gridHeight, 0.0, 1.0 );
			glEvalMesh2( GL_LINE, 0, gridWidth, 0, gridHeight );
		glPopAttrib( );
	glEndList( );

	reset( FALSE );
}

void deinitialize( void )
{
	lc_vector_destroy( &bestfs_path );
	pvector_destroy( &dijkstra_path );
	pvector_destroy( &astar_path );

	csearch_destroy( &bfs );
	csearch_destroy( &dijkstra );
	csearch_destroy( &ass );

	free( tiles );
	glDeleteLists( blockList, 2 );
}


void draw_tiles( void )
{
	// draw tiles
	int windex = 0;
	int index = 0;
	for( unsigned int y = 0; y < gridHeight; y++ )
	{
		for( unsigned int x = 0; x < gridWidth; x++ )
		{
			#if 1
			index = windex + x;
			#else
			//int index = y * gridWidth + x ;
			#endif

			glPushMatrix( );
				glPushAttrib( GL_CURRENT_BIT );
					if( x == start.x && y == start.y )
						glColor3f( 0.0f, 1.0f, 0.0f );
					else if( x == end.x && y == end.y )
						glColor3f( 1.0f, 0.0f, 0.0f );
					else if( !tiles[ index ].is_walkable )
						glColor3f( 0.0f, 0.0f, 1.0f );
					else
						continue;

					glTranslatef( x * tileWidth, y * tileHeight, 0.0f );
					glScalef( tileWidth, tileHeight, 1.0f );


					glCallList( blockList );

				glPopAttrib( );
			glPopMatrix( );

		}

		windex += gridWidth;
	}
}


void draw_path( const lc_pvector_t* restrict path, GLfloat color[] )
{
	size_t i;
	if( pvector_size(path) <= 0 ) return;

	glPushAttrib( GL_CURRENT_BIT | GL_LINE_BIT );
		glColor4fv( color );

		glLineWidth( tileWidth / 5.0f );
		glDisable( GL_LINE_STIPPLE );
		glBegin( GL_LINE_STRIP );

		for( i = 0; i < pvector_size(path); i++ )
		{
			tile_t* p_current = pvector_get( (lc_pvector_t*) path, i );
			glVertex2f( p_current->position.x * tileWidth + 0.5f * tileWidth, p_current->position.y * tileHeight + 0.5f * tileHeight );
		}
		glEnd( );
	glPopAttrib( );

}

void render( void )
{
	GLfloat bfsPathColor[]      = { 0.0f, 6.0f, 6.0f, 0.7f };
	GLfloat dijkstraPathColor[] = { 6.0f, 0.0f, 6.0f, 0.7f };
	GLfloat assPathColor[]      = { 6.0f, 6.0f, 0.0f, 0.7f };

	glClear( GL_COLOR_BUFFER_BIT  );
	glLoadIdentity( );



	draw_tiles( );

	draw_path( &bestfs_path, bfsPathColor );
	draw_path( &dijkstra_path, dijkstraPathColor );
	draw_path( &astar_path, assPathColor );

	// draw grid...
	if( gridWidth * gridWidth <= 1600 )
	{
		glCallList( gridList );
	}


	/* Write text */
	//int width = glutGet((GLenum)GLUT_WINDOW_WIDTH);
	//int height = glutGet((GLenum)GLUT_WINDOW_HEIGHT);

	write_text( GLUT_BITMAP_HELVETICA_18, "Path Finding", 2, 22, 1.0f, 1.0f, 1.0f );
	write_text( GLUT_BITMAP_8_BY_13, "Press <ESC> to quit, <A> for A*, <B> for Best-First Search, <R> to randomize, and <r> to reset.", 2, 5, 1.0f, 1.0f, 1.0f );

	glutSwapBuffers( );
}

void resize( int width, int height )
{
	glViewport( 0, 0, width, height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );


#define max( x, y )              ((x) ^ (((x) ^ (y)) & -((x) < (y))))
	height = max( 1, height );

	glOrtho( 0.0, width, 0.0, height, 0.0, 1.0 );
	glMatrixMode( GL_MODELVIEW );

	windowWidth  = width;
	windowHeight = height;


	// resize our grid...
	grid[ 0 ][ 1 ][ 0 ] = (GLfloat) width;
	grid[ 1 ][ 0 ][ 1 ] = (GLfloat) height;
	grid[ 1 ][ 1 ][ 0 ] = (GLfloat) width;
	grid[ 1 ][ 1 ][ 1 ] = (GLfloat) height;


	glMap2f( GL_MAP2_VERTEX_3, 0.0f, 1.0f, 3, 2, 0.0f, 1.0f, 2 * 3, 2, (GLfloat *) grid );

	// set tile dimensions...
	tileWidth  = (float) width / (float) gridWidth;
	tileHeight = (float) height / (float) gridHeight;

}

void keyboard_keypress( unsigned char key, int x, int y )
{
	switch( key )
	{
		case ESC_KEY:
			deinitialize( );
			exit( 0 );
			break;
		case 'A':
		case 'a':
		{
			if( start.x == -1 || start.y == -1 ) return;
			if( end.x == -1 || end.y == -1 ) return;

			tile_t *p_start = &tiles[ start.y * gridWidth + start.x ];
			tile_t *p_end   = &tiles[ end.y * gridWidth + end.x ];

			boolean found = csearch_find( ass, p_start, p_end );

			if( found )
			{
				astar_node_t* p_node;
				pvector_clear( &astar_path );

				for( p_node = csearch_first_node( ass );
				     p_node != NULL;
				     p_node = csearch_next_node( p_node ) )
				{
					pvector_push( &astar_path, (void*) csearch_state(p_node) );
				}
			}

			csearch_cleanup( ass );
			glutPostRedisplay( );
			break;
		}
		case 'B':
		case 'b':
		{
			if( start.x == -1 || start.y == -1 ) return;
			if( end.x == -1 || end.y == -1 ) return;

			tile_t *p_start = &tiles[ start.y * gridWidth + start.x ];
			tile_t *p_end   = &tiles[ end.y * gridWidth + end.x ];

			boolean found = csearch_find( bfs, p_start, p_end );

			if( found )
			{
				bestfs_node_t* p_node;
				pvector_clear( &bestfs_path );

				for( p_node = csearch_first_node( bfs );
				     p_node != NULL;
				     p_node = csearch_next_node( p_node ) )
				{
					pvector_push( &bestfs_path, (void*) csearch_state(p_node) );
				}
			}

			csearch_cleanup( bfs );
			glutPostRedisplay( );
			break;
		}
		case 'D':
		case 'd':
		{
			if( start.x == -1 || start.y == -1 ) return;
			if( end.x == -1 || end.y == -1 ) return;

			tile_t *p_start = &tiles[ start.y * gridWidth + start.x ];
			tile_t *p_end   = &tiles[ end.y * gridWidth + end.x ];

			boolean found = csearch_find( dijkstra, p_start, p_end );

			if( found )
			{
				dijkstra_node_t* p_node;
				pvector_clear( &dijkstra_path );

				for( p_node = csearch_first_node( dijkstra );
				     p_node != NULL;
				     p_node = csearch_next_node( p_node ) )
				{
					pvector_push( &dijkstra_path, (void*) csearch_state(p_node) );
				}
			}

			csearch_cleanup( dijkstra );
			glutPostRedisplay( );
			break;
		}
		case 'R':
		{
			reset( TRUE );
			glutPostRedisplay( );
			break;
		}
		case 'r':
		{
			reset( FALSE );
			glutPostRedisplay( );
			break;
		}
		case '4':
		{
			csearch_set_successors_fxn( bfs, tile_successors4 );
			csearch_set_successors_fxn( dijkstra, tile_successors4 );
			csearch_set_successors_fxn( ass, tile_successors4 );
			break;
		}
		case '8':
		{
			csearch_set_successors_fxn( bfs, tile_successors8 );
			csearch_set_successors_fxn( dijkstra, tile_successors8 );
			csearch_set_successors_fxn( ass, tile_successors8 );
			break;
		}
		case 'h':
		{
			csearch_set_heuristic_fxn( bfs, tile_manhattan_distance );
			csearch_set_heuristic_fxn( ass, tile_manhattan_distance );
			break;
		}
		case 'H':
		{
			csearch_set_heuristic_fxn( bfs, tile_euclidean_distance );
			csearch_set_heuristic_fxn( ass, tile_euclidean_distance );
			break;
		}
		default:
			break;
	}

}

int mouseButton = 0;
int mouseButtonState = 0;

void mouse( int button, int state, int x, int y )
{
	mouseButton = button;
	mouseButtonState = state;

	if( mouseButton == GLUT_MIDDLE_BUTTON && mouseButtonState == GLUT_DOWN && y <= windowHeight )
	{
		unsigned int elementX =  x / tileWidth;
		unsigned int elementY = (windowHeight - y) / tileHeight;

		start.x = elementX;
		start.y = elementY;

		glutPostRedisplay( );
	}
	else if( mouseButton == GLUT_RIGHT_BUTTON && mouseButtonState == GLUT_DOWN && y <= windowHeight )
	{
		unsigned int elementX =  x / tileWidth;
		unsigned int elementY = (windowHeight - y) / tileHeight;

		end.x = elementX;
		end.y = elementY;

		glutPostRedisplay( );
	}
}

void mouse_motion( int x, int y )
{
	if( mouseButton == GLUT_LEFT_BUTTON && mouseButtonState == GLUT_DOWN && y <= windowHeight )
	{
		unsigned int elementX =  x / tileWidth;
		unsigned int elementY = (windowHeight - y) / tileHeight;

		unsigned int index = elementY * gridWidth + elementX;
		tiles[ index ].is_walkable = FALSE;
		glutPostRedisplay( );
	}
}

void idle( void )
{ /*glutPostRedisplay( );*/ }

void write_text( void *font, const char* text, int x, int y, float r, float g, float b )
{
	int width   = glutGet( (GLenum) GLUT_WINDOW_WIDTH );
	int height  = glutGet( (GLenum) GLUT_WINDOW_HEIGHT );
	size_t size = strlen( text ) + 1;

	glPushAttrib( GL_LIGHTING_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_LIGHTING );

		glMatrixMode( GL_PROJECTION );
		glPushMatrix( );
			glLoadIdentity( );
			glOrtho( 0, width, 0, height, 1.0, 10.0 );

			glMatrixMode( GL_MODELVIEW );
			glPushMatrix( );
				glLoadIdentity( );
				glColor3f( r, g, b );
				glTranslatef( 0.0f, 0.0f, -1.0f );
				glRasterPos2i( x, y );

				for( unsigned int i = 0; i < size; i++ )
					glutBitmapCharacter( font, text[ i ] );

			glPopMatrix( );
			glMatrixMode( GL_PROJECTION );
		glPopMatrix( );
		glMatrixMode( GL_MODELVIEW );
	glPopAttrib( );
}



void reset( boolean bRandomize )
{
	pvector_clear( &bestfs_path );
	pvector_clear( &dijkstra_path );
	pvector_clear( &astar_path );

	for( unsigned int y = 0; y < gridHeight; y++ )
	{
		for( unsigned int x = 0; x < gridWidth; x++ )
		{
			unsigned int index = y * gridWidth + x;

			if( bRandomize )
				tiles[ index ].is_walkable = (rand() % 1000) > 200 ;
			else
				tiles[ index ].is_walkable =  TRUE;

			tiles[ index ].position.x = x;
			tiles[ index ].position.y = y;

			tiles[ index ].cost = 0;
			tiles[ index ].F = 0;
			tiles[ index ].G = 0;
			tiles[ index ].H = 0;
		}
	}

#if 0
	start.x = -1;
	start.y = -1;
	end.x   = -1;
	end.y   = -1;
#else
	start.x = 0;
	start.y = DEFAULT_GRIDHEIGHT - 1;
	end.x   = DEFAULT_GRIDWIDTH - 1;
	end.y   = 0;
#endif
}




void tile_successors8( const void* restrict state, successors_t* restrict p_successors )
{
	const tile_t* p_tile = state;

	#if 1 //optimized version
	int s4[]   = { -1, 1 };
	size_t size = sizeof(s4) / sizeof(s4[0]);


	for( int i = 0; i < size; i++ )
	{
		for( int j = 0; j < size; j++ )
		{
			int successorY = p_tile->position.y + s4[ j ];
			int successorX = p_tile->position.x + s4[ i ];

			if( successorY >= 0 &&
			    successorX >= 0 &&
			    successorY < gridHeight &&
			    successorX < gridWidth )
			{
				int index = successorY * gridWidth + successorX;

				if( tiles[ index ].is_walkable == FALSE ) continue;

				successors_push( p_successors, &tiles[ index ] );
			}
		}
	}
	#else // original code
	for( int j = -1; j <= 1; j++ )
	{
		for( int i = -1; i <= 1; i++ )
		{
			if( i == 0 && j == 0 ) continue;
			int successorY = p_tile->position.y + j;
			int successorX = p_tile->position.x + i;

			if( successorY >= 0 &&
			    successorX >= 0 &&
			    successorY < gridHeight &&
			    successorX < gridWidth )
			{
				int index = successorY * gridWidth + successorX;

				if( tiles[ index ].is_walkable == FALSE ) continue;

				successors_push( p_successors, &tiles[ index ] );
			}
		}
	}
	#endif
}

void tile_successors4( const void* restrict state, successors_t* restrict p_successors )
{
	const tile_t* p_tile = state;

	#if 1 //optimized version
	coordinate_t deltas[] = {
		{ -1,  0 },
		{  1,  0 },
		{  0, -1 },
		{  0,  1 },
	};
	int successorY;
	int successorX;
	const size_t size = sizeof(deltas)/sizeof(deltas[0]);
	for( int i = 0; i < size; i++ )
	{
		successorY = p_tile->position.y + deltas[ i ].y;
		successorX = p_tile->position.x + deltas[ i ].x;

		if( successorX >= 0 && successorX < gridWidth &&
		    successorY >= 0 && successorY < gridHeight )
		{
			int index = successorY * gridWidth + successorX;

			if( !tiles[ index ].is_walkable ) continue;

			successors_push( p_successors, &tiles[ index ] );
		}
	}
	#else // original code
	for( int i = -1; i <= 1; i += 2 )
	{
		int successorY = p_tile->position.y;
		int successorX = p_tile->position.x + i;

		if( successorX >= 0 && successorX < gridWidth )
		{
			int index = successorY * gridWidth + successorX;

			if( !tiles[ index ].is_walkable ) continue;

			successors_push( p_successors, &tiles[ index ] );
		}
	}

	for( int i = -1; i <= 1; i += 2 )
	{
		int successorY = p_tile->position.y + i;
		int successorX = p_tile->position.x;

		if( successorY >= 0 && successorY < gridHeight )
		{
			int index = successorY * gridWidth + successorX;

			if( tiles[ index ].is_walkable == FALSE ) continue;

			successors_push( p_successors, &tiles[ index ] );
		}
	}
	#endif
}


int tile_manhattan_distance( const void *t1, const void *t2 )
{
	const tile_t* p_tile1 = t1;
	const tile_t* p_tile2 = t2;

	return manhattan_distance( &p_tile1->position, &p_tile2->position );
}

int tile_euclidean_distance( const void *t1, const void *t2 )
{
	const tile_t* p_tile1 = t1;
	const tile_t* p_tile2 = t2;

	//return euclidean_distance( &p_tile1->position, &p_tile2->position );
	return (int) sqrtf( (float) ((p_tile1->position.x - p_tile2->position.x) * (p_tile1->position.x - p_tile2->position.x) +
					 (p_tile1->position.y - p_tile2->position.y) * (p_tile1->position.y - p_tile2->position.y)) );
}



unsigned int tile_positive_cost( const void *t1, const void *t2 )
{
#if 0
	const tile_t* p_tile1 = t1;
	const tile_t* p_tile2 = t2;

	//return tile_euclidean_distance( p_tile1, p_tile2 );
	return abs(p_tile1->position.x - p_tile2->position.x) + abs(p_tile1->position.y - p_tile2->position.y);
#else
	return 1;
#endif
}

int tile_cost( const void *t1, const void *t2 )
{
	const tile_t* p_tile1 = t1;
	const tile_t* p_tile2 = t2;

#if 1
	//return tile_euclidean_distance( p_tile1, p_tile2 );
	return abs(p_tile1->position.x - p_tile2->position.x) + abs(p_tile1->position.y - p_tile2->position.y);
#else
	return 1;
#endif
}
