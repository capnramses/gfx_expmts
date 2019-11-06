// Neural network in a compute shader
// Dr Anton Gerdelan 10 May 2017
// C99 and GLSL. requires opengl 4.5, glfw3 and glew
// gcc -std=c99 -o demo main.c -lglfw -lGL /path/to/glew/src/glew.c

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h> // for exit()
#include <stdio.h>
#include <stdbool.h>

// dimensions for our window and a pointer to it
int win_width = 800;
int win_height = 600;
GLFWwindow *window;

void init_gl();
void create_geometry();
void create_shaders();
void draw_frame();

int main() {
	printf( "Yellow World\n" );
	init_gl();
	
	bool is_running = true;
	
	while(is_running) {
		draw_frame();
    is_running = false;		
	}

	return 0;
}

// custom atexit() shutdown/cleanup function
void my_exit() { glfwTerminate(); }

void init_gl() {
if ( !glfwInit() ) {
    printf( "ERROR: could not initialise GLFW\n" );
    exit( 1 );
  }

  // suggest GLFW start OpenGL 4.1 Core Profile - play around here
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 5 );
  glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

	// create a 800x600 window and OpenGL context with GLFW
  GLFWwindow *win = glfwCreateWindow( 800, 600, "My Window", NULL, NULL );
  if ( !win ) {
    printf( "ERROR: opening window/OpenGL context with GLFW\n" );
    glfwTerminate();
    exit( 1 );
  }
  // opengl functions we call now relate to this window+context
  glfwMakeContextCurrent( win );

// from now on make sure the window is closed properly if we exit
  atexit( my_exit );

  // start GLEW to hook up OpenGL functions so we can call them
  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if ( GLEW_OK != err ) {
    printf( "ERROR: could not start GLEW\n" );
    exit( 1 );
  }
	// ask OpenGL which version is running
  const GLubyte *renderer = glGetString( GL_RENDERER );
  const GLubyte *version = glGetString( GL_VERSION );
  printf( "renderer=%s\nOpenGL version=%s\n", renderer, version );


}
void create_geometry() {
}

// current plan:
// invocation computes one layer
// each node is a workgroup member
// could probably encode this in fragment shader passes too?
// then you wouldn't even need compute shaders...
void create_shaders() {
	parse_file_into_str( "ray.comp", shader_str, 100000 );
	
	
	// create a buffer the size of the inputs
	
	
	
}

void draw_frame() {
	// input compute shader->hidden (buffer size of hiddens)

	// fence

	// hidden compute shader->output (buffer size of outputs)
}

