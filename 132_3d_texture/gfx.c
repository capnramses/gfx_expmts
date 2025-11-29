#include "gfx.h"
#include <stdbool.h>
#include <stdio.h>

static void _glfw_error_callback( int error, const char* description ) { fputs( description, stderr ); }

gfx_t gfx_start( int w, int h, const char* title_str ) {
  gfx_t gfx = (gfx_t){ .started = false };

  printf( "starting GLFW %s", glfwGetVersionString() );

  glfwSetErrorCallback( _glfw_error_callback );
  if ( !glfwInit() ) {
    fprintf( stderr, "ERROR: Could not start GLFW3.\n" );
    return gfx;
  }

  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
  glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  glfwWindowHint( GLFW_SAMPLES, 16 );
  gfx.window_ptr = glfwCreateWindow( w, h, title_str, NULL, NULL );
  if ( !gfx.window_ptr ) {
    fprintf( stderr, "ERROR: Could not open window with GLFW3.\n" );
    glfwTerminate();
    return gfx;
  }
  glfwMakeContextCurrent( gfx.window_ptr );

  int version_glad = gladLoadGL( glfwGetProcAddress );
  if ( version_glad == 0 ) {
    fprintf( stderr, "ERROR: Failed to initialise OpenGL context.\n" );
    glfwTerminate();
    return gfx;
  }
  printf( "Loaded OpenGL %i.%i\n", GLAD_VERSION_MAJOR( version_glad ), GLAD_VERSION_MINOR( version_glad ) );

  printf( "Renderer: %s.\n", glGetString( GL_RENDERER ) );
  printf( "OpenGL version supported %s.\n", glGetString( GL_VERSION ) );

  gfx.started = true;
  return gfx;
}

void gfx_stop( void ) { glfwTerminate(); }

texture_t gfx_texture_create( uint32_t w, uint32_t h, uint32_t d, const uint8_t* pixels_ptr ) {
  texture_t tex = (texture_t){ .w = w, .h = h, .d = d };
    
  return tex;
}
