// Basic demo/test of BMP File Reader and display in an OpenGL app
// Anton Gerdelan
// Licence: see apg_bmp.h
// C99

/* clang-format off

HOW TO COMPILE:
---------------
Windows:
gcc -o demo .\main_opengl.c ..\common\src\GL\glew.c apg_bmp.c -lpthread -I ..\common\include\  -DGLEW_STATIC -lOpenGL32  ..\common\win64_gcc\libglfw3.a -lgdi32 -lws2_32

GNU/Linux:
gcc -o demo main_opengl.c apg_bmp.c ../common/src/GL/glew.c -I ../common/include/ -lglfw -lpthread -lGL

HOW TO RUN: Just drag the .BMP file onto demo.exe, or run with the path to the file as the first command line argument.

clang-format on */

#include "apg_bmp.h"
#include <GL/glew.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifdef USE_STB_INSTEAD

#define STBI_ONLY_BMP
#define STB_IMAGE_IMPLEMENTATION
#include "../common/include/stb/stb_image.h"

#endif

static int g_win_width = 1920, g_win_height = 1080;
static GLFWwindow* g_window;

static bool _start_gl() {
  {
    if ( !glfwInit() ) {
      fprintf( stderr, "ERROR: could not start GLFW3\n" );
      return false;
    }
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

    g_window = glfwCreateWindow( g_win_width, g_win_height, "BMP Test", NULL, NULL );
    if ( !g_window ) {
      fprintf( stderr, "ERROR: could not open window with GLFW3\n" );
      glfwTerminate();
      return false;
    }
    glfwMakeContextCurrent( g_window );
  }
  glewExperimental = GL_TRUE;
  glewInit();

  const GLubyte* renderer = glGetString( GL_RENDERER );
  const GLubyte* version  = glGetString( GL_VERSION );
  printf( "Renderer: %s\n", renderer );
  printf( "OpenGL version supported %s\n", version );

  return true;
}

struct texture_t {
  char filename[256];
  GLuint handle_gl;

  bool loaded;
  int w, h, ncomps;
  unsigned char* pixels_ptr; // only used whilst loading
};

struct texture_t load_texture( const char* filename ) {
  struct texture_t texture;
  memset( &texture, 0, sizeof( struct texture_t ) );

  // for comparison can swap-in stb_image
#ifdef USE_STB_INSTEAD
  texture.pixels_ptr = stbi_load( filename, &texture.w, &texture.h, &texture.ncomps, 0 );
#else
	bool vertically_flip = true;
  texture.pixels_ptr = apg_read_bmp( filename, &texture.w, &texture.h, &texture.ncomps, vertically_flip );
#endif
  if ( !texture.pixels_ptr ) { return texture; }
  strncpy( texture.filename, filename, 255 );

  glGenTextures( 1, &texture.handle_gl );
  {
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for irregular display sizes in RGB
    glBindTexture( GL_TEXTURE_2D, texture.handle_gl );
    if ( 4 == texture.ncomps ) {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, texture.w, texture.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture.pixels_ptr );
    } else if ( 3 == texture.ncomps ) {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, texture.w, texture.h, 0, GL_RGB, GL_UNSIGNED_BYTE, texture.pixels_ptr );
    } else if ( 1 == texture.ncomps ) {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, texture.w, texture.h, 0, GL_RED, GL_UNSIGNED_BYTE, texture.pixels_ptr );
    }
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glBindTexture( GL_TEXTURE_2D, 0 );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
  }
  free( texture.pixels_ptr );
  texture.loaded = true;

  return texture;
}

int main( int argc, char** argv ) {
  if ( argc < 2 ) {
    printf( "usage: ./demo INFILE.BMP\n" );
    return 0;
  }
  printf( "reading file `%s`\n", argv[1] );

  struct texture_t texture;

  _start_gl();

  texture = load_texture( argv[1] );
  if ( !texture.loaded ) {
    fprintf( stderr, "ERROR: could not load texture from `%s`\n", argv[1] );
    glfwTerminate();
    return 1;
  }

  printf( "texture loaded from `%s` %ix%i with %i channels. %u uncompressed bytes.\n", texture.filename, texture.w, texture.h, texture.ncomps,
    texture.w * texture.h * texture.ncomps );
  glfwSetWindowSize( g_window, texture.w, texture.h );

  const char* vertex_shader =
    "#version 410\n"
    "in vec2 a_vp;"
    "out vec2 v_st;"
    "void main () {"
    "  v_st = a_vp * 0.5 + 0.5;"
    "  v_st.t = 1.0 - v_st.t;"
    "  gl_Position = vec4( a_vp.x, a_vp.y, 0.0, 1.0 );"
    "}";
  const char* fragment_shader =
    "#version 410\n"
    "in vec2 v_st;"
    "uniform sampler2D u_tex;"
    "uniform float u_greyscale;"
    "out vec4 o_frag_colour;"
    "vec4 texel = texture( u_tex, v_st );"
    "void main () {"
    "  o_frag_colour = texture( u_tex, v_st );"
    "  if ( u_greyscale > 0.0 ) { o_frag_colour = vec4( texel.r, texel.r, texel.r, 1.0 ); }"
    "}";
  GLuint vs = glCreateShader( GL_VERTEX_SHADER );
  glShaderSource( vs, 1, &vertex_shader, NULL );
  glCompileShader( vs );
  GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
  glShaderSource( fs, 1, &fragment_shader, NULL );
  glCompileShader( fs );
  GLuint program = glCreateProgram();
  glAttachShader( program, fs );
  glAttachShader( program, vs );
  glLinkProgram( program );
  glDeleteShader( vs );
  glDeleteShader( fs );
  GLint u_greyscale_loc = glGetUniformLocation( program, "u_greyscale" );

  float square[] = { -1, 1, -1, -1, 1, -1, 1, -1, 1, 1, -1, 1 };
  GLuint vertex_buffer_gl, vertex_array_gl;
  glGenVertexArrays( 1, &vertex_array_gl );
  glGenBuffers( 1, &vertex_buffer_gl );

  glBindVertexArray( vertex_array_gl );
  glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer_gl );
  {
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * 12, square, GL_STATIC_DRAW );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, vertex_buffer_gl );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, NULL );
  }
  glBindBuffer( GL_ARRAY_BUFFER, 0 );
  glBindVertexArray( 0 );

  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glClearColor( 0.2, 0.2, 0.2, 1.0 );
  while ( !glfwWindowShouldClose( g_window ) ) {
    glfwWaitEvents(); // be kind to the CPU when not needing to update
    int width, height;
    glfwGetFramebufferSize( g_window, &width, &height );
    glViewport( 0, 0, width, height );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glEnable( GL_BLEND );
    glUseProgram( program );
    if ( 1 == texture.ncomps ) { glUniform1f( u_greyscale_loc, 1.0f ); }
    glActiveTexture( GL_TEXTURE0 );
    glBindVertexArray( vertex_array_gl );

    glBindTexture( GL_TEXTURE_2D, texture.handle_gl );
    glDrawArrays( GL_TRIANGLES, 0, 6 );

    glBindVertexArray( 0 );
    glUseProgram( 0 );
    glDisable( GL_BLEND );

    glfwSwapBuffers( g_window );
  }

  glDeleteProgram( program );
  glDeleteBuffers( 1, &vertex_buffer_gl );
  glDeleteVertexArrays( 1, &vertex_array_gl );
  glDeleteTextures( 1, &texture.handle_gl );
  glfwTerminate();
  printf( "program HALT\n" );
  return 0;
}
