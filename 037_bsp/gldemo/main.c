//
// BSP thing as in main.c but this one has an opengl renderer
// Anton Gerdelan 26 Oct 2016 <gerdela@scss.tcd.ie>
//
// compile on linux 64-bit:
// gcc -o demo main.c gl_utils.c GL/glew.c lib/linux64/libglfw3.a -lGL -std=c99
// -lX11 -lXxf86vm -lm -lpthread -ldl -lXrandr -lXcursor -lXinerama
//
// run with ./demo
// * press space bar to toggle number of walls to draw/traverse
//

/* further notes:
great explanation of doom's renderer here:
http://fabiensanglard.net/doomIphone/doomClassicRenderer.php
* great video under visplanes

-doom actually renders near-to-far (except transparent walls and 'things')
 in columns of pixels (scanline algorithm)
 when a column is filled -> done. no more drawing there. free occlusion cull.
-visplanes split map into large chunks
-bsp sorts walls in order, and only closest 256 are drawn.
-frustum culling eliminates out-of-scene walls
-floats rarely used as FPUs not common in CPUs of 1993
-lookup table converts angles of x,y things from player to screen space pos
*/

#include "includes.h"
#include "gl_utils.h"
#include "linmath.h"

#define WALL_COUNT 6
int draw_count = 1;
int g_drawn_so_far = 0;

GLuint wall_vao, wall_vbo;
GLuint globoshader;
GLint colour_loc;

bool draw_wall( float start_x, float start_y, float end_x, float end_y,
								int unique_id ) {
	if ( g_drawn_so_far >= draw_count ) {
		return false;
	}
	g_drawn_so_far++;

	// 5,0-4
	// |\|
	// 1_2,3

	float points[18];
	points[0] = start_x;
	points[1] = 5.0f;
	points[2] = start_y;

	points[3] = start_x;
	points[4] = 0.0f;
	points[5] = start_y;

	points[6] = end_x;
	points[7] = 0.0f;
	points[8] = end_y;

	points[9] = end_x;
	points[10] = 0.0f;
	points[11] = end_y;

	points[12] = end_x;
	points[13] = 5.0f;
	points[14] = end_y;

	points[15] = start_x;
	points[16] = 5.0f;
	points[17] = start_y;

	switch ( unique_id ) {
	case 0:
		glUniform3f( colour_loc, 0.8, 0, 0 );
		break;
	case 1:
		glUniform3f( colour_loc, 0.8, 0, 0.5 );
		break;
	case 2:
		glUniform3f( colour_loc, 0.0, 0.8, 0 );
		break;
	case 3:
		glUniform3f( colour_loc, 0.0, 0, 0.8 );
		break;
	case 4:
		glUniform3f( colour_loc, 0.8, 0.8, 0 );
		break;
	case 5:
		glUniform3f( colour_loc, 0.0, 0.8, 0.8 );
		break;
	default:
		glUniform3f( colour_loc, 1, 1, 1 );
	}

	glBindBuffer( GL_ARRAY_BUFFER, wall_vbo );
	glBufferData( GL_ARRAY_BUFFER, 18 * sizeof( float ), points, GL_STATIC_DRAW );
	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
	glDrawArrays( GL_TRIANGLES, 0, 6 );

	return true;
}

int main() {

	// build tree here
	// create initial list of indices to walls (could also have copied the
	// walls around directly)
	int *walls_list = (int *)malloc( sizeof( int ) * g_num_walls );
	for ( int i = 0; i < g_num_walls; i++ ) {
		walls_list[i] = i;
	}

	// build the BSP tree from the list of walls
	BSP_Node *root = create_bsp( walls_list, g_num_walls );

	printf( "BSP tree created with %i nodes\n", g_nodes_in_tree );

#if 0
// test the drawing code
	printf("\n----Drawing from V:-----\n");
	traverse_BSP_tree(root, 5.0f, 5.0f);

	printf("\n----Drawing from W:-----\n");
	traverse_BSP_tree(root, 5.0f, -15.0f);

	printf("\n----Drawing from Z:-----\n");
	traverse_BSP_tree(root, -5.0f, -5.0f);
#endif

	// TODO build geometry for each wall? here

	init_gl();

	glGenVertexArrays( 1, &wall_vao );
	glBindVertexArray( wall_vao );

	glGenBuffers( 1, &wall_vbo );

	float aspect = (float)g_gfx.fb_width / (float)g_gfx.fb_height;
	mat4 V = look_at( ( vec3 ){.x = -1, .y = 30, .z = 30 },
										( vec3 ){.x = -1, .y = 0, .z = 0 },
										( vec3 ){.x = 0, .y = 0.7, .z = -0.7 } );
	mat4 P = perspective( 69.0, aspect, 0.1, 100.0 );

// globoshader here
#ifdef APPLE
	const char *vertex_shader = "#version 400\n"
															"in vec3 vp;"
															"uniform mat4 P, V;"
															"void main() {"
															"  gl_Position = P * V * vec4(vp, 1.0);"
															"}";
	const char *fragment_shader = "#version 400\n"
																"uniform vec3 colour;"
																"out vec4 frag_colour;"
																"void main() {"
																"  frag_colour = vec4(colour, 1.0);"
																"}";
#endif
// these are OpenGL 2.1 shaders so that they run on my crappy demo laptop
#ifdef CRAPPY_LAPTOP
	const char *vertex_shader = "#version 120\n"
															"attribute vec3 vp;"
															"uniform mat4 P, V;"
															"void main() {"
															"  gl_Position = P * V * vec4(vp, 1.0);"
															"}";
	const char *fragment_shader = "#version 120\n"
																"uniform vec3 colour;"
																"void main() {"
																"  gl_FragColor = vec4(colour, 1.0);"
																"}";
#endif

	GLuint vs = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( vs, 1, &vertex_shader, NULL );
	glCompileShader( vs );
	GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( fs, 1, &fragment_shader, NULL );
	glCompileShader( fs );
	globoshader = glCreateProgram();
	glAttachShader( globoshader, fs );
	glAttachShader( globoshader, vs );
	glLinkProgram( globoshader );

	GLint P_loc = glGetUniformLocation( globoshader, "P" );
	GLint V_loc = glGetUniformLocation( globoshader, "V" );
	colour_loc = glGetUniformLocation( globoshader, "colour" );

	glUseProgram( globoshader );
	glUniformMatrix4fv( P_loc, 1, GL_FALSE, P.m );
	glUniformMatrix4fv( V_loc, 1, GL_FALSE, V.m );

	glClearColor( 0.2, 0.2, 0.2, 1.0 );

	while ( !glfwWindowShouldClose( g_gfx.window ) ) {
		glfwPollEvents();
		if ( glfwGetKey( g_gfx.window, GLFW_KEY_ESCAPE ) ) {
			glfwSetWindowShouldClose( g_gfx.window, 1 );
		}

		// toggle through drawing partial scene
		static bool space_locked = false;
		if ( glfwGetKey( g_gfx.window, GLFW_KEY_SPACE ) ) {
			if ( !space_locked ) {
				draw_count = ( draw_count + 1 ) % WALL_COUNT;
				printf( "draw count = %i/%i\n", draw_count, WALL_COUNT );
				space_locked = true;
			}
		} else {
			space_locked = false;
		}

		{
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

			// reset flag of how many items drawn so far, and then draw the tree
			g_drawn_so_far = 0;
			traverse_BSP_tree( root, 5.0f, 5.0f );

			glfwSwapBuffers( g_gfx.window );
		}
	}

	return 0;
}
