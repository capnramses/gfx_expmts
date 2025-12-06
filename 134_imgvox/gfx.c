#include "gfx.h"
#include "apg.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void _glfw_error_callback( int error, const char* description ) { fprintf( stderr, "GLFW ERROR: code %i msg: %s.\n", error, description ); }

gfx_t gfx_start( int w, int h, const char* title_str ) {
  gfx_t gfx = (gfx_t){ .started = false };

  printf( "Starting GLFW %s.\n", glfwGetVersionString() );

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
    glTexParameteri( tex.target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
  }
  glTexParameteri( tex.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( tex.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( tex.target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( tex.target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
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

static bool _shader_create_from_file( const char* path_str, uint32_t shader_type, uint32_t* shader_ptr ) {
  char shader_str[102400] = { 0 };
  if ( !apg_file_to_str( path_str, 102400, shader_str ) ) {
    fprintf( stderr, "ERROR: Could not read shader file `%s`.\n", path_str );
    return false;
  }
  *shader_ptr         = glCreateShader( shader_type );
  const char* str_ptr = shader_str;
  glShaderSource( *shader_ptr, 1, &str_ptr, NULL );
  glCompileShader( *shader_ptr );

  int params = -1;
  glGetShaderiv( *shader_ptr, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    int max_length = 2048, actual_length = 0;
    char slog[2048];
    glGetShaderInfoLog( *shader_ptr, max_length, &actual_length, slog );
    fprintf( stderr, "ERROR: Shader index %u did not compile.\n%s\n", *shader_ptr, slog );
    return false;
  }

  return true;
}

bool gfx_shader_create_from_file( const char* vs_path, const char* fs_path, shader_t* shader_ptr ) {
  shader_t sp       = { 0 };
  GLuint shaders[2] = { 0 };
  if ( !_shader_create_from_file( vs_path, GL_VERTEX_SHADER, &shaders[0] ) ) { return false; }
  if ( !_shader_create_from_file( fs_path, GL_FRAGMENT_SHADER, &shaders[1] ) ) { return false; }
  sp.program = glCreateProgram();
  glAttachShader( sp.program, shaders[0] );
  glAttachShader( sp.program, shaders[1] );
  glLinkProgram( sp.program );
  glDeleteShader( shaders[0] );
  glDeleteShader( shaders[1] );

  int params = -1;
  glGetProgramiv( sp.program, GL_LINK_STATUS, &params );
  if ( GL_TRUE != params ) {
    int max_length = 2048, actual_length = 0;
    char plog[2048];
    glGetProgramInfoLog( sp.program, max_length, &actual_length, plog );
    fprintf( stderr, "ERROR: Could not link shader program GL index %u.\n%s\n", sp.program, plog );
    return false;
  }

  *shader_ptr = sp;
  return true;
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
