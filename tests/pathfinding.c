/*
 * Copyright (C) 2012 by Joseph A. Marrero and Shrewd LLC. http://www.manvscode.com/
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
#include <GL/freeglut.h>
#include <libcollections/types.h>
#include <libcollections/vector.h>
#include <libcollections/hash-functions.h>
#include <csearch.h>
#include <heuristics.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define ESC_KEY			27

extern void* pvector_get( pvector_t *p_vector, size_t index );

void initialize         ( );
void deinitialize       ( );
void draw_tiles         ( );
void draw_path          ( const pvector_t* path, GLfloat color[] );
void render             ( );
void resize             ( int width, int height );
void keyboard_keypress  ( unsigned char key, int x, int y );
void mouse              ( int button, int state, int x, int y );
void mouse_motion       ( int x, int y );
void idle               ( );
void write_text         ( void *font, const char* text, int x, int y, float r, float g, float b );
void reset              ( boolean bRandomize );
static void tile_successors4   ( const void* state, pvector_t* p_successors );
static void tile_successors8   ( const void* state, pvector_t* p_successors );
static int tile_manhattan_distance( const void *t1, const void *t2 );
static int tile_euclidean_distance( const void *t1, const void *t2 );
static unsigned int tile_cost( const void *t1, const void *t2 );

int windowWidth;
int windowHeight;
float tileWidth;
float tileHeight;

#define DEFAULT_gridWidth			23
#define DEFAULT_gridHeight			20

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

coordinate_t start;
coordinate_t end;

tile_t *tiles = NULL;
unsigned int blockList = 0, gridList = 0;

GLfloat bfsPathColor[] = { 0.0f, 6.0f, 6.0f, 0.7f };
GLfloat dijkstraPathColor[] = { 6.0f, 0.0f, 6.0f, 0.7f };
GLfloat assPathColor[] = { 6.0f, 6.0f, 0.0f, 0.7f };


unsigned int gridWidth = DEFAULT_gridWidth;
unsigned int gridHeight = DEFAULT_gridHeight;

pvector_t bestfs_path;
pvector_t dijkstra_path;
pvector_t ass_path;

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
	glutIdleFunc( idle );
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

void initialize( )
{
	pvector_create( &bestfs_path, 1, malloc, free );
	pvector_create( &dijkstra_path, 1, malloc, free );
	pvector_create( &ass_path, 1, malloc, free );

	bfs      = bestfs_create( pointer_hash, tile_manhattan_distance, tile_successors8 );
	dijkstra = dijkstra_create( pointer_hash, tile_cost, tile_successors8 );
	ass      = astar_create( );

	glDisable( GL_DEPTH_TEST );
	
	glEnable( GL_POINT_SMOOTH );
	glEnable( GL_LINE_SMOOTH );
	glHint( GL_POINT_SMOOTH_HINT, GL_FASTEST );
	glHint( GL_LINE_SMOOTH_HINT, GL_FASTEST );
	
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	

	glShadeModel( GL_SMOOTH );
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

void deinitialize( )
{
	pvector_destroy( &bestfs_path );
	pvector_destroy( &dijkstra_path );
	pvector_destroy( &ass_path );

	bestfs_destroy( &bfs );
	dijkstra_destroy( &dijkstra );
	astar_destroy( &ass );

	free( tiles );
	glDeleteLists( blockList, 2 );
}


void draw_tiles( )
{
	// draw tiles
	for( unsigned int y = 0; y < gridHeight; y++ )
	{
		for( unsigned int x = 0; x < gridWidth; x++ )
		{
			int index = y * gridWidth + x ;

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
	}
}


void draw_path( const pvector_t *path, GLfloat color[] )
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
			tile_t* p_current = pvector_get( (pvector_t*) path, i );
			glVertex2f( p_current->position.x * tileWidth + 0.5f * tileWidth, p_current->position.y * tileHeight + 0.5f * tileHeight );
		}
		glEnd( );
	glPopAttrib( );
	
}

void render( )
{
	glClear( GL_COLOR_BUFFER_BIT  );
	glLoadIdentity( );	


	draw_tiles( );

	draw_path( &bestfs_path, bfsPathColor );
	draw_path( &dijkstra_path, dijkstraPathColor );
	draw_path( &ass_path, assPathColor );

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
	

	if( height == 0 )
		height = 1;
		
	glOrtho( 0.0, width, 0.0, height, 0.0, 1.0 );
	glMatrixMode( GL_MODELVIEW );

	windowWidth = width;
	windowHeight = height;


	// resize our grid...
	grid[ 0 ][ 1 ][ 0 ] = (GLfloat) width;
	grid[ 1 ][ 0 ][ 1 ] = (GLfloat) height;
	grid[ 1 ][ 1 ][ 0 ] = (GLfloat) width;
	grid[ 1 ][ 1 ][ 1 ] = (GLfloat) height;


	glMap2f( GL_MAP2_VERTEX_3, 0.0f, 1.0f, 3, 2, 0.0f, 1.0f, 2 * 3, 2, (GLfloat *) grid );

	// set tile dimensions...
	tileWidth = (float) width / (float) gridWidth;
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
			/*
			pvector_clear( &ass_path );
			if( start.x == -1 || start.y == -1 ) return;
			if( end.x == -1 || end.y == -1 ) return;
			tile_t *p_start = &tiles[ start.y * gridWidth + start.x ];
			tile_t *p_end = &tiles[ end.y * gridWidth + end.x ];

			boolean found = ass.find( p_start, p_end );

			if( found )
			{
				ASS::Node *pPathNode = ass.getPath( );

				while( pPathNode != NULL )
				{
					ass_path.push_back( pPathNode->state );
					pvector_push( &ass_path, pPathNode->state );
					pPathNode = pPathNode->parent;
				}

			}
			
			ass.cleanup( );
			glutPostRedisplay( );
			*/
			break;
		}
		case 'B':
		case 'b':
		{ 
			if( start.x == -1 || start.y == -1 ) return;
			if( end.x == -1 || end.y == -1 ) return;

			tile_t *p_start = &tiles[ start.y * gridWidth + start.x ];
			tile_t *p_end   = &tiles[ end.y * gridWidth + end.x ];

			boolean found = bestfs_find( bfs, p_start, p_end );

			if( found )
			{
				bestfs_node_t* p_node;
				pvector_clear( &bestfs_path );

				for( p_node = bestfs_first_node( bfs );
				     p_node != NULL;
				     p_node = bestfs_next_node( p_node ) )
				{
					pvector_push( &bestfs_path, (void*) bestfs_state(p_node) );
				}
			}
			
			bestfs_cleanup( bfs );
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

			boolean found = dijkstra_find( dijkstra, p_start, p_end );

			if( found )
			{
				dijkstra_node_t* p_node;
				pvector_clear( &dijkstra_path );

				for( p_node = dijkstra_first_node( dijkstra );
				     p_node != NULL;
				     p_node = dijkstra_next_node( p_node ) )
				{
					pvector_push( &dijkstra_path, (void*) dijkstra_state(p_node) );
				}
			}
			
			dijkstra_cleanup( dijkstra );
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
			bestfs_set_successors_fxn( bfs, tile_successors4 );
			break;
		}
		case '8':
		{
			bestfs_set_successors_fxn( bfs, tile_successors8 );
			break;
		}
		case 'h':
		{
			bestfs_set_heuristic_fxn( bfs, tile_manhattan_distance );
			break;
		}
		case 'H':
		{
			bestfs_set_heuristic_fxn( bfs, tile_euclidean_distance );
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

	if( mouseButton == GLUT_MIDDLE_BUTTON && mouseButtonState == GLUT_DOWN )
	{
		unsigned int elementX =  x / tileWidth;
		unsigned int elementY = (windowHeight - y) / tileHeight;
		
		start.x = elementX;
		start.y = elementY;

		glutPostRedisplay( );
	}
	else if( mouseButton == GLUT_RIGHT_BUTTON && mouseButtonState == GLUT_DOWN )
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
	if( mouseButton == GLUT_LEFT_BUTTON && mouseButtonState == GLUT_DOWN )
	{
		unsigned int elementX =  x / tileWidth;
		unsigned int elementY = (windowHeight - y) / tileHeight;

		unsigned int index = elementY * gridWidth + elementX;
		tiles[ index ].is_walkable = FALSE;
		glutPostRedisplay( );
	}
}

void idle( )
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
	pvector_clear( &ass_path );

	for( unsigned int y = 0; y < gridHeight; y++ )
	{
		for( unsigned int x = 0; x < gridWidth; x++ )
		{
			unsigned int index = y * gridWidth + x;

			if( bRandomize )
				tiles[ index ].is_walkable = ( rand() % 1000 ) > 200 ? TRUE : FALSE;
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

	start.x = -1;
	start.y = -1;
	end.x   = -1;
	end.y   = -1;
}




void tile_successors8( const void* state, pvector_t* p_successors )
{
	const tile_t* p_tile = state;

	#if 0 //optimized version
	int s4[]   = { -1, 1 };
	size_t len = sizeof(s4) / sizeof(s4[0]);


	for( int i = 0; i < len; i++ )
	{
		for( int j = 0; j < len; j++ )
		{
			int successorY = p_tile->position.y + s4[ j ];
			int successorX = p_tile->position.x + s4[ i ];

			if( successorY < 0 ) continue;
			if( successorX < 0 ) continue;
			if( successorY >= gridHeight ) continue;
			if( successorX >= gridWidth ) continue;

			int index = successorY * gridWidth + successorX;

			if( tiles[ index ].is_walkable == FALSE ) continue;

			pvector_push( p_successors, &tiles[ index ] );
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

			if( successorY < 0 ) continue;
			if( successorX < 0 ) continue;
			if( successorY >= gridWidth ) continue;
			if( successorX >= gridHeight ) continue;

			int index = successorY * gridWidth + successorX;

			if( tiles[ index ].is_walkable == FALSE ) continue;

			pvector_push( p_successors, &tiles[ index ] );
		}
	}
	#endif
}

void tile_successors4( const void* state, pvector_t* p_successors )
{
	const tile_t* p_tile = state;

	#if 0 //optimized version
	#else // original code
	for( int i = -1; i <= 1; i += 2 )
	{
		int successorY = p_tile->position.y;
		int successorX = p_tile->position.x + i;

		if( successorY < 0 ) continue;
		if( successorX < 0 ) continue;
		if( successorY >= gridHeight ) continue;
		if( successorX >= gridWidth ) continue;

		int index = successorY * gridWidth + successorX;

		if( tiles[ index ].is_walkable == FALSE ) continue;

		pvector_push( p_successors, &tiles[ index ] );
	}

	for( int i = -1; i <= 1; i += 2 )
	{
		int successorY = p_tile->position.y + i;
		int successorX = p_tile->position.x;

		if( successorY < 0 ) continue;
		if( successorX < 0 ) continue;
		if( successorY >= gridHeight ) continue;
		if( successorX >= gridWidth ) continue;

		int index = successorY * gridWidth + successorX;

		if( tiles[ index ].is_walkable == FALSE ) continue;

		pvector_push( p_successors, &tiles[ index ] );
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

	return euclidean_distance( &p_tile1->position, &p_tile2->position );
}

unsigned int tile_cost( const void *t1, const void *t2 )
{
	const tile_t* p_tile1 = t1;
	const tile_t* p_tile2 = t2;

	return abs(p_tile1->position.x + p_tile2->position.x) +
	       abs(p_tile1->position.y + p_tile2->position.y);
}
