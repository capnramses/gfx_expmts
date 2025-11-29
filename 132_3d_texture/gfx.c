#include "gfx.h"
#include "apg.h"
#include <assert.h>
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

texture_t gfx_texture_create( uint32_t w, uint32_t h, uint32_t d, uint32_t n, const uint8_t* pixels_ptr ) {
  texture_t tex         = (texture_t){ .w = w, .h = h, .d = d, .n = n };
  GLint internal_format = 4 == n ? GL_RGBA : 3 == n ? GL_RGB : 2 == n ? GL_RG : GL_RED;
  GLenum format         = 4 == n ? GL_RGBA : 3 == n ? GL_RGB : 2 == n ? GL_RG : GL_RED;
  if ( 3 == n || 1 == n ) { glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); }
  glActiveTexture( GL_TEXTURE0 );
  glGenTextures( 1, &tex.texture );
  tex.target = 0 == d ? GL_TEXTURE_2D : GL_TEXTURE_3D;
  glBindTexture( tex.target, tex.texture );
  if ( 0 == d ) {
    glTexImage2D( tex.target, 0, internal_format, tex.w, tex.h, 0, format, GL_UNSIGNED_BYTE, pixels_ptr );
  } else {
    glTexImage3D( tex.target, 0, internal_format, tex.w, tex.h, tex.d, 0, format, GL_UNSIGNED_BYTE, pixels_ptr );
  }
  glTexParameteri( tex.target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( tex.target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexParameteri( tex.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( tex.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glBindTexture( tex.target, 0 );

  return tex;
}

mesh_t gfx_mesh_cube_create( void ) {
  mesh_t m       = (mesh_t){ .n_points = 36 };
  float points[] = { -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f };
  glGenVertexArrays( 1, &m.vao );
  glGenBuffers( 1, &m.vbo );
  glBindVertexArray( m.vao );
  glBindBuffer( GL_ARRAY_BUFFER, m.vbo );
  glBufferData( GL_ARRAY_BUFFER, 3 * m.n_points * sizeof( float ), &points, GL_STATIC_DRAW );
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
  glBindVertexArray( 0 );
  return m;
}

static GLuint _shader_create_from_file( const char* path, uint32_t shader_type ) {
  apg_file_t record;
  bool ret = apg_read_entire_file( path, &record );
  assert( ret );
  GLuint shader = glCreateShader( shader_type );
  char* str_ptr = record.data_ptr;
  glShaderSource( shader, 1, (const char* const*)&str_ptr, NULL );
  glCompileShader( shader );
  return shader;
}

shader_t gfx_shader_create_from_file( const char* vs_path, const char* fs_path ) {
  shader_t sp = { 0 };
  GLuint vs   = _shader_create_from_file( vs_path, GL_VERTEX_SHADER );
  GLuint fs   = _shader_create_from_file( fs_path, GL_FRAGMENT_SHADER );
  sp.program  = glCreateProgram();
  glAttachShader( sp.program, fs );
  glAttachShader( sp.program, vs );
  glLinkProgram( sp.program );
  return sp;
}

void gfx_draw( mesh_t mesh, texture_t texture, shader_t shader ) {
  glUseProgram( shader.program );
  glBindVertexArray( mesh.vao );
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( texture.target, texture.texture );

  glDrawArrays( GL_TRIANGLES, 0, mesh.n_points );

  glBindTexture( texture.target, 0 );
  glBindVertexArray( 0 );
  glUseProgram( 0 );
}
