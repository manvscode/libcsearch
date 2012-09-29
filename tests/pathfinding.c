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
#include <libgsearch/csearch.h>
#include <libgsearch/heuristic.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define ESC_KEY			27

void initialize         ( );
void deinitialize       ( );
void drawTiles          ( );
void drawPath           ( const pvector_t* path, GLfloat color[] );
void render             ( );
void resize             ( int width, int height );
void keyboard_keypress  ( unsigned char key, int x, int y );
void mouse              ( int button, int state, int x, int y );
void mouseMotion        ( int x, int y );
void idle               ( );
void writeText          ( void *font, const char* text, int x, int y, float r, float g, float b );
void reset              ( boolean bRandomize );

int windowWidth;
int windowHeight;
float tileWidth;
float tileHeight;

#define DEFAULT_gridWidth			40		
#define DEFAULT_gridHeight			40

GLfloat grid[ 2 ][ 2 ][ 3 ] = {
		{ {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },
		{ {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f} }
	};


typedef struct coordinate {
	int x;
	int y;
} coordinate_t;

typedef struct tile {
	coordinate_t position;
	boolean bWalkable;

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

GLfloat assPathColor[] = { 6.0f, 6.0f, 0.0f, 0.7f };
GLfloat bfsPathColor[] = { 0.0f, 6.0f, 6.0f, 0.7f };


unsigned int gridWidth = DEFAULT_gridWidth;
unsigned int gridHeight = DEFAULT_gridHeight;

pvector_t ass_path;
pvector_t bfs_path;

bfs_t* bfs;

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
	glutCreateWindow( "[Path Finding Demo] - http://www.ManVsCode.com/" );
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
	glutMotionFunc( mouseMotion );


	glutSetOption( GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS );

	initialize( );
	glutMainLoop( );
	deinitialize( );

	return 0;
}


void initialize( )
{
	bfs = bfs_create( state_hash_fxn state_hasher, heuristic_fxn heuristic, successors_fxn successors_of );


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
			glLineWidth( 0.35f );
			glColor3f( 0.2f, 1.0f, 1.0f );

			glMapGrid2f( gridWidth, 0.0, 1.0, gridHeight, 0.0, 1.0 );
			glEvalMesh2( GL_LINE, 0, gridWidth, 0, gridHeight );
		glPopAttrib( );
	glEndList( );

	reset( FALSE );
}

void deinitialize( )
{
	bfs_destroy( &bfs );
	free( tiles );
	glDeleteLists( blockList, 2 );
}


void drawTiles( )
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
					else if( !tiles[ index ].bWalkable )
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


void drawPath( const pvector_t *path, GLfloat color[] )
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


	drawTiles( );

	drawPath( &ass_path, assPathColor );
	drawPath( &bfs_path, bfsPathColor );

	// draw grid...
	if( gridWidth * gridWidth <= 1600 )
	{
		glCallList( gridList );
	}


	/* Write text */
	//int width = glutGet((GLenum)GLUT_WINDOW_WIDTH);
	//int height = glutGet((GLenum)GLUT_WINDOW_HEIGHT);

	writeText( GLUT_BITMAP_HELVETICA_18, "Path Finding", 2, 22, 1.0f, 1.0f, 1.0f );
	writeText( GLUT_BITMAP_8_BY_13, "Press <ESC> to quit, <A> for A*, <B> for Best-First Search, <R> to randomize, and <r> to reset.", 2, 5, 1.0f, 1.0f, 1.0f );
	
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
			tile_t *pStart = &tiles[ start.y * gridWidth + start.x ];
			tile_t *pEnd = &tiles[ end.y * gridWidth + end.x ];

			boolean bPathFound = ass.find( pStart, pEnd );

			if( bPathFound )
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
			pvector_clear( &bfs_path );
			if( start.x == -1 || start.y == -1 ) return;
			if( end.x == -1 || end.y == -1 ) return;
			tile_t *pStart = &tiles[ start.y * gridWidth + start.x ];
			tile_t *pEnd = &tiles[ end.y * gridWidth + end.x ];

			boolean bPathFound = bfs.find( pStart, pEnd );

			if( bPathFound )
			{
				BFS::Node *pPathNode = bfs.getPath( );

				while( pPathNode != NULL )
				{
					bfs_path.push_back( pPathNode->state );
					pPathNode = pPathNode->parent;
				}

			}
			
			bfs.cleanup( );
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
	}
	else if( mouseButton == GLUT_RIGHT_BUTTON && mouseButtonState == GLUT_DOWN )
	{
		unsigned int elementX =  x / tileWidth;
		unsigned int elementY = (windowHeight - y) / tileHeight;
		
		end.x = elementX;
		end.y = elementY;
	}
}

void mouseMotion( int x, int y )
{
	if( mouseButton == GLUT_LEFT_BUTTON && mouseButtonState == GLUT_DOWN )
	{
		unsigned int elementX =  x / tileWidth;
		unsigned int elementY = (windowHeight - y) / tileHeight;

		unsigned int index = elementY * gridWidth + elementX;
		tiles[ index ].bWalkable = FALSE;
		glutPostRedisplay( );
	}
}

void idle( )
{ /*glutPostRedisplay( );*/ }

void writeText( void *font, const char* text, int x, int y, float r, float g, float b )
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
	pvector_clear( &ass_path );
	pvector_clear( &bfs_path );

	for( unsigned int y = 0; y < gridHeight; y++ )
	{
		for( unsigned int x = 0; x < gridWidth; x++ )
		{
			unsigned int index = y * gridWidth + x;

			if( bRandomize )
				tiles[ index ].bWalkable = ( rand() % 1000 ) > 200 ? TRUE : FALSE;
			else
				tiles[ index ].bWalkable =  TRUE;

			tiles[ index ].X = x;
			tiles[ index ].Y = y;

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









struct TileSuccessors8 {
	std::vector<Tile *> operator()( const Tile *tile )
	{
		std::vector<Tile *> successors;

		for( int j = -1; j <= 1; j++ )
		{
			for( int i = -1; i <= 1; i++ )
			{
				if( i == 0 && j == 0 ) continue;
				int successorY = tile->Y + j;
				int successorX = tile->X + i;

				if( successorY < 0 ) continue;
				if( successorX < 0 ) continue;
				if( successorY >= gridWidth ) continue;
				if( successorX >= gridHeight ) continue;

				int index = successorY * gridWidth + successorX;

				if( tiles[ index ].bWalkable == false ) continue;

				successors.push_back( &tiles[ index ] );
			}
		}

		return successors;
	}
};


struct TileSuccessors4 {
	std::vector<Tile *> operator()( const Tile *tile )
	{
		std::vector<Tile *> successors;

		for( int i = -1; i <= 1; i += 2 )
		{
			int successorY = tile->Y;
			int successorX = tile->X + i;

			if( successorY < 0 ) continue;
			if( successorX < 0 ) continue;
			if( successorY >= gridHeight ) continue;
			if( successorX >= gridWidth ) continue;

			int index = successorY * gridWidth + successorX;

			if( tiles[ index ].bWalkable == false ) continue;

			successors.push_back( &tiles[ index ] );
		}

		for( int i = -1; i <= 1; i += 2 )
		{
			int successorY = tile->Y + i;
			int successorX = tile->X;

			if( successorY < 0 ) continue;
			if( successorX < 0 ) continue;
			if( successorY >= gridHeight ) continue;
			if( successorX >= gridWidth ) continue;

			int index = successorY * gridWidth + successorX;

			if( tiles[ index ].bWalkable == false ) continue;

			successors.push_back( &tiles[ index ] );
		}
		

		return successors;
	}
};


