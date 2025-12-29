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
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 ); // Could do 4.2 Just for glsl 420 match. (Using ARB conservative depth).
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

void gfx_texture_update( uint32_t w, uint32_t h, uint32_t d, uint32_t n, bool integer, const uint8_t* pixels_ptr, texture_t* tex_ptr ) {
  assert( tex_ptr && tex_ptr->texture > 0 );
  tex_ptr->w            = w;
  tex_ptr->h            = h;
  tex_ptr->d            = d;
  tex_ptr->n            = n;
  GLint internal_format = integer ? GL_R8UI : 4 == n ? GL_RGBA : 3 == n ? GL_RGB : 2 == n ? GL_RG : GL_RED;
  GLenum format         = integer ? GL_RED_INTEGER : 4 == n ? GL_RGBA : 3 == n ? GL_RGB : 2 == n ? GL_RG : GL_RED;
  if ( 3 == n || 1 == n ) { glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); }
  tex_ptr->target = ( w > 0 && 0 == h && 0 == d ) ? GL_TEXTURE_1D : 0 == d ? GL_TEXTURE_2D : GL_TEXTURE_3D;
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( tex_ptr->target, tex_ptr->texture );
  if ( 0 == h && 0 == d ) {
    glTexImage1D( tex_ptr->target, 0, internal_format, tex_ptr->w, 0, format, GL_UNSIGNED_BYTE, pixels_ptr );
  } else if ( 0 == d ) {
    glTexImage2D( tex_ptr->target, 0, internal_format, tex_ptr->w, tex_ptr->h, 0, format, GL_UNSIGNED_BYTE, pixels_ptr );
  } else {
    glTexImage3D( tex_ptr->target, 0, internal_format, tex_ptr->w, tex_ptr->h, tex_ptr->d, 0, format, GL_UNSIGNED_BYTE, pixels_ptr );
    glTexParameteri( tex_ptr->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
  }
  glTexParameteri( tex_ptr->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( tex_ptr->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( tex_ptr->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( tex_ptr->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glBindTexture( tex_ptr->target, 0 );
}

texture_t gfx_texture_create( uint32_t w, uint32_t h, uint32_t d, uint32_t n, bool integer, const uint8_t* pixels_ptr ) {
  texture_t tex = (texture_t){ .texture = 0 };
  glGenTextures( 1, &tex.texture );
  gfx_texture_update( w, h, d, n, integer, pixels_ptr, &tex );
  return tex;
}

/** Triangulated unit cube vertices with flat shaded normals.
 *  {x,y,z,nx,ny,nz,s,t}.
 *  Use matching index array `_unit_cube_indices` for elements. */
static float _unit_cube_vertices[] = {
  1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,       // 0
  -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,      // 1
  -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,     // 2
  1.0f, -1.0f, 1.0f, 0.0f, -0.0f, 1.0f, 0.0f, 1.0f,     // 3
  1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,    // 4
  1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,     // 5
  -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,    // 6
  -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,   // 7
  -1.0f, -1.0f, -1.0f, -1.0f, -0.0f, -0.0f, 0.0f, 0.0f, // 8
  -1.0f, -1.0f, 1.0f, -1.0f, -0.0f, -0.0f, 1.0f, 0.0f,  // 9
  -1.0f, 1.0f, 1.0f, -1.0f, -0.0f, -0.0f, 1.0f, 1.0f,   // 10
  -1.0f, 1.0f, -1.0f, -1.0f, -0.0f, 0.0f, 0.0f, 1.0f,   // 11
  -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,    // 12
  1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,     // 13
  1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,    // 14
  -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,   // 15
  1.0f, 1.0f, -1.0f, 1.0f, -0.0f, 0.0f, 0.0f, 0.0f,     // 16
  1.0f, 1.0f, 1.0f, 1.0f, -0.0f, 0.0f, 1.0f, 0.0f,      // 17
  1.0f, -1.0f, 1.0f, 1.0f, -0.0f, 0.0f, 1.0f, 1.0f,     // 18
  1.0f, -1.0f, -1.0f, 1.0f, -0.0f, 0.0f, 0.0f, 1.0f,    // 19
  -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // 20
  -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,      // 21
  1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,       // 22
  1.0f, 1.0f, -1.0f, 0.0f, 1.0f, -0.0f, 0.0f, 1.0f      // 23
};

mesh_t gfx_mesh_cube_create( void ) {
  mesh_t m = (mesh_t){ .n_points = 36 };
  // { 3d point, 3d normal }
  float points[] = {
    -1.0f, 1.0f, -1.0f, 0, 0, -1,  // front face
    -1.0f, -1.0f, -1.0f, 0, 0, -1, // front face
    1.0f, -1.0f, -1.0f, 0, 0, -1,  // front face
    1.0f, -1.0f, -1.0f, 0, 0, -1,  // front face
    1.0f, 1.0f, -1.0f, 0, 0, -1,   // front face
    -1.0f, 1.0f, -1.0f, 0, 0, -1,  // front face
    -1.0f, -1.0f, 1.0f, -1, 0, 0,  // left face
    -1.0f, -1.0f, -1.0f, -1, 0, 0, // left face
    -1.0f, 1.0f, -1.0f, -1, 0, 0,  // left face
    -1.0f, 1.0f, -1.0f, -1, 0, 0,  // left face
    -1.0f, 1.0f, 1.0f, -1, 0, 0,   // left face
    -1.0f, -1.0f, 1.0f, -1, 0, 0,  // left face
    1.0f, -1.0f, -1.0f, 1, 0, 0,   // right face
    1.0f, -1.0f, 1.0f, 1, 0, 0,    // right face
    1.0f, 1.0f, 1.0f, 1, 0, 0,     // right face
    1.0f, 1.0f, 1.0f, 1, 0, 0,     // right face
    1.0f, 1.0f, -1.0f, 1, 0, 0,    // right face
    1.0f, -1.0f, -1.0f, 1, 0, 0,   // right face
    -1.0f, -1.0f, 1.0f, 0, 0, 1,   // back face
    -1.0f, 1.0f, 1.0f, 0, 0, 1,    // back face
    1.0f, 1.0f, 1.0f, 0, 0, 1,     // back face
    1.0f, 1.0f, 1.0f, 0, 0, 1,     // back face
    1.0f, -1.0f, 1.0f, 0, 0, 1,    // back face
    -1.0f, -1.0f, 1.0f, 0, 0, 1,   // back face
    -1.0f, 1.0f, -1.0f, 0, 1, 0,   // top face
    1.0f, 1.0f, -1.0f, 0, 1, 0,    // top face
    1.0f, 1.0f, 1.0f, 0, 1, 0,     // top face
    1.0f, 1.0f, 1.0f, 0, 1, 0,     // top face
    -1.0f, 1.0f, 1.0f, 0, 1, 0,    // top face
    -1.0f, 1.0f, -1.0f, 0, 1, 0,   // top face
    -1.0f, -1.0f, -1.0f, 0, -1, 0, // bottom face
    -1.0f, -1.0f, 1.0f, 0, -1, 0,  // bottom face
    1.0f, -1.0f, -1.0f, 0, -1, 0,  // bottom face
    1.0f, -1.0f, -1.0f, 0, -1, 0,  // bottom face
    -1.0f, -1.0f, 1.0f, 0, -1, 0,  // bottom face
    1.0f, -1.0f, 1.0f, 0, -1, 0    // bottom face
  };
  glGenVertexArrays( 1, &m.vao );
  glGenBuffers( 1, &m.vbo );
  glBindVertexArray( m.vao );
  glBindBuffer( GL_ARRAY_BUFFER, m.vbo );
  glBufferData( GL_ARRAY_BUFFER, 6 * m.n_points * sizeof( float ), &points, GL_STATIC_DRAW );
  glEnableVertexAttribArray( 0 );
  glEnableVertexAttribArray( 1 );
  GLintptr vertex_position_offset = 0 * sizeof( float );
  GLintptr vertex_normal_offset   = 3 * sizeof( float );
  GLsizei vertex_stride_sz        = 6 * sizeof( float );
  glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, vertex_stride_sz, (GLvoid*)vertex_position_offset );
  glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, vertex_stride_sz, (GLvoid*)vertex_normal_offset );
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
  glBindAttribLocation( sp.program, 0, "a_vp" );
  glBindAttribLocation( sp.program, 1, "a_vn" );
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

void gfx_draw( shader_t shader, mesh_t mesh, const texture_t** textures_ptr, int n_textures ) {
  glUseProgram( shader.program );
  glBindVertexArray( mesh.vao );
  for ( int i = 0; i < n_textures; i++ ) {
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( textures_ptr[i]->target, textures_ptr[i]->texture );
  }

  glDrawArrays( GL_TRIANGLES, 0, mesh.n_points );

  for ( int i = 0; i < n_textures; i++ ) {
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( textures_ptr[i]->target, 0 );
  }
  glBindVertexArray( 0 );
  glUseProgram( 0 );
}
