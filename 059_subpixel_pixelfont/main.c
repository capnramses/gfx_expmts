// based on -> https://www.shadertoy.com/view/ltBGWc
// current feeling
// this is poor in this non screenspace impl.
// it introduces artifacts, doesn't fix shimmer,
// adds blur (which is the whole reason to use nearest to begin with), and doesn't work at all for magnification
// taking more samples OR UPSCALING THE TEXTURE & USING BILINEAR might be a better idea if this is really required at all

#include "apg_pixfont.h"
#include <GL/glew.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  GLuint handle_gl;
  int w, h, ncomps;
};

struct texture_t load_texture_from_mem( uint8_t* image_mem, int w, int h, int n ) {
  struct texture_t texture;
  texture.w      = w;
  texture.h      = h;
  texture.ncomps = n;

  glGenTextures( 1, &texture.handle_gl );
  {
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for irregular display sizes in RGB
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, texture.handle_gl );
    if ( 4 == texture.ncomps ) {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, texture.w, texture.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_mem );
    } else if ( 3 == texture.ncomps ) {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, texture.w, texture.h, 0, GL_RGB, GL_UNSIGNED_BYTE, image_mem );
    } else if ( 1 == texture.ncomps ) {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, texture.w, texture.h, 0, GL_RED, GL_UNSIGNED_BYTE, image_mem );
    }
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    // NOTE(Anton) keep it on bilinear for pixel art. but modify the texcoords before sampling
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glBindTexture( GL_TEXTURE_2D, 0 );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
  }

  return texture;
}

int main() {
  struct texture_t texture;
  int w, h;
  if ( !_start_gl() ) { return 1; }
  {
    int n_channels              = 1;
    int thickness               = 1;
    bool outlines               = false;
    bool vflip                  = false;
    unsigned char* text_img_mem = NULL;
    memset( &texture, 0, sizeof( struct texture_t ) );
    apg_pixfont_image_size_for_str( "my_string", &w, &h, thickness, outlines );
    text_img_mem = (unsigned char*)malloc( w * h * n_channels );
    memset( text_img_mem, 0x00, w * h * n_channels );
    apg_pixfont_str_into_image( "my_string", text_img_mem, w, h, n_channels, 0xFF, 0x7F, 0x00, 0xFF, thickness, outlines, vflip );
    texture = load_texture_from_mem( text_img_mem, w, h, n_channels );
    free( text_img_mem );
  }

  const char* vertex_shader =
    "#version 410\n"
    "in vec2 a_vp;"
    "uniform float u_scale, u_time;"
    "out vec2 v_st;"
    "void main () {"
    "  v_st = a_vp * 0.5 + 0.5;"
    "  v_st.t = 1.0 - v_st.t;"
    "  gl_Position = vec4( a_vp.xy * 0.0425 , 0.0, 1.0 );"
    "  gl_Position.x += sin( u_time * 0.015 );"
    "}";
  const char* fragment_shader =
    "#version 410\n"
    "in vec2 v_st;"
    "uniform ivec2 u_tex_dims;"
    "uniform sampler2D u_tex;"
    "out vec4 o_frag_colour;"
    "void main () {"
    "  vec2 big_coords = v_st * u_tex_dims;"
    "  vec2 uv = floor(big_coords);" // emulate point sampling
    "  uv += 1.0 - clamp((1.0 - fract(big_coords)), 0.0, 1.0);" // subsample AA stuff. comment to disable
    "  vec4 texel = texture( u_tex, uv / u_tex_dims );"
    "  o_frag_colour = vec4( texel.r, texel.r, texel.r, 1.0 );"
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
  int u_scale_loc = glGetUniformLocation( program, "u_scale" );
  glProgramUniform1f( program, u_scale_loc, 0.035 );
  int u_time_loc     = glGetUniformLocation( program, "u_time" );
  int u_tex_dims_loc = glGetUniformLocation( program, "u_tex_dims" );
  glProgramUniform2i( program, u_tex_dims_loc, w, h );

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
  glEnable( GL_BLEND );
  glClearColor( 0.2, 0.2, 0.2, 1.0 );
  while ( !glfwWindowShouldClose( g_window ) ) {
    double t = glfwGetTime();
    glProgramUniform1f( program, u_time_loc, t );
    glfwPollEvents(); // be kind to the CPU when not needing to update
    int width, height;
    glfwGetFramebufferSize( g_window, &width, &height );
    glViewport( 0, 0, width, height );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glEnable( GL_BLEND );
    glUseProgram( program );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, texture.handle_gl );

    glBindVertexArray( vertex_array_gl );

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

  return 0;
}