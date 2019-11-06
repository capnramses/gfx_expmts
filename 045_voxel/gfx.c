#include "gfx.h"

#include "../common/include/stb/stb_image.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define VP_LOCATION 0
#define VT_LOCATION 1
#define VC_LOCATION 2
#define VN_LOCATION 3
#define INSTANCE_P_WOR_LOCATION 4
#define INSTANCE_YR_LOCATION 5

GLuint g_unit_cube_vao, g_unit_cube_vbo;

/////////////////////////////////////////// extras /////////////////////////////////////////////////

static void _init_unit_cube() {
  GLfloat unit_cube_pos[3 * 3 * 6 * 2] = {
    1.0, -1.0, 1.0, -1.0, -1.0, 1.0, -1.0, -1.0, -1.0,  // bottom
    -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 1.0, 1.0,     // top
    1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 1.0,      // right
    1.0, 1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0,     // front
    -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0,   // left
    1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, // back
    1.0, -1.0, -1.0, 1.0, -1.0, 1.0, -1.0, -1.0, -1.0,  // bottom 2
    1.0, 1.0, -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, 1.0,     // top 2
    1.0, -1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0, 1.0,    // right 2
    1.0, -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, -1.0, 1.0,     // front 2
    -1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, 1.0, -1.0, // left 2
    1.0, 1.0, -1.0, 1.0, -1.0, -1.0, -1.0, 1.0, -1.0    // back 2
  };
  glGenVertexArrays( 1, &g_unit_cube_vao );
  glGenBuffers( 1, &g_unit_cube_vbo );

  glBindVertexArray( g_unit_cube_vao );
  {
    glBindBuffer( GL_ARRAY_BUFFER, g_unit_cube_vbo );
    {
      glBufferData( GL_ARRAY_BUFFER, sizeof( unit_cube_pos ), unit_cube_pos, GL_STATIC_DRAW );
      glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    }
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glEnableVertexAttribArray( 0 );
  }
  glBindVertexArray( 0 );
}

////////////////////////////////////////// context /////////////////////////////////////////////////
GLFWwindow* g_window;
int g_win_width  = 800;
int g_win_height = 600;

static void _glfw_error_callback( int error, const char* description ) { fprintf( stderr, "GLFW ERROR: code %i msg: %s\n", error, description ); }

static void _debug_gl_callback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam ) {
  char src_str[2048], type_str[2048], sev_str[2048];

  switch ( source ) {
  case 0x8246: {
    strncpy( src_str, "API", 2048 );
  } break;
  case 0x8247: {
    strncpy( src_str, "WINDOW_SYSTEM", 2048 );
  } break;
  case 0x8248: {
    strncpy( src_str, "SHADER_COMPILER", 2048 );
  } break;
  case 0x8249: {
    strncpy( src_str, "THIRD_PARTY", 2048 );
  } break;
  case 0x824A: {
    strncpy( src_str, "APPLICATION", 2048 );
  } break;
  case 0x824B: {
    strncpy( src_str, "OTHER", 2048 );
  } break;
  default: { strncpy( src_str, "undefined", 2048 ); } break;
  }

  switch ( type ) {
  case 0x824C: {
    strncpy( type_str, "ERROR", 2048 );
  } break;
  case 0x824D: {
    strncpy( type_str, "DEPRECATED_BEHAVIOR", 2048 );
  } break;
  case 0x824E: {
    strncpy( type_str, "UNDEFINED_BEHAVIOR", 2048 );
  } break;
  case 0x824F: {
    strncpy( type_str, "PORTABILITY", 2048 );
  } break;
  case 0x8250: {
    // strncpy (type_str, "PERFORMANCE", 2048);
    return; // ignore these messages
  } break;
  case 0x8251: {
    strncpy( type_str, "OTHER", 2048 );
  } break;
  case 0x8268: {
    strncpy( type_str, "MARKER", 2048 );
  } break;
  case 0x8269: {
    strncpy( type_str, "PUSH_GROUP", 2048 );
  } break;
  case 0x826A: {
    strncpy( type_str, "POP_GROUP", 2048 );
  } break;
  default: { strncpy( type_str, "undefined", 2048 ); } break;
  }

  switch ( severity ) {
  case 0x9146: {
    strncpy( sev_str, "HIGH", 2048 );
  } break;
  case 0x9147: {
    strncpy( sev_str, "MEDIUM", 2048 );
  } break;
  case 0x9148: {
    strncpy( sev_str, "LOW", 2048 );
  } break;
  case 0x826B: {
    // strncpy (sev_str, "NOTIFICATION", 2048);
    return; // ignore these messages
  } break;
  default: { strncpy( sev_str, "undefined", 2048 ); } break;
  }

  fprintf( stderr, "src: %s type: %s id: %u severity: %s len: %i msg: %s\n", src_str, type_str, id, sev_str, length, message );
}

static bool _extension_checks() {
  bool has_all_vital = true;

  // non-core but should be available on a toaster since GL 1.1
  if ( GLEW_EXT_texture_compression_s3tc ) {
    printf( "EXT_texture_compression_s3tc = yes\n" );
  } else {
    printf( "EXT_texture_compression_s3tc = no\n" );
  }

  // non-core (IP issue) but should be available on a toaster since GL 1.2
  if ( GLEW_EXT_texture_filter_anisotropic ) {
    printf( "EXT_texture_filter_anisotropic = yes\n" );
  } else {
    printf( "EXT_texture_filter_anisotropic = no\n" );
  }

  // (core since GL 3.0) 2004 works with opengl 1.5
  if ( GLEW_ARB_texture_float ) { // 94% support
    printf( "ARB_texture_float = yes\n" );
  } else {
    printf( "ARB_texture_float = no\n" );
    // has_all_vital = false; // APPLE reports it doesn't have this
  }

  // core since 3.0
  if ( GLEW_ARB_framebuffer_object ) { // glcapsviewer gives 89% support
    printf( "ARB_framebuffer_object = yes\n" );
  } else {
    printf( "ARB_framebuffer_object = no\n" );
    has_all_vital = false;
  }

  // core since 3.0
  if ( GLEW_ARB_vertex_array_object ) { // 87% support
    printf( "ARB_vertex_array_object = yes\n" );
  } else {
    printf( "ARB_vertex_array_object = no\n" );
    has_all_vital = false;
  }

  // core since 3.1
  if ( GLEW_ARB_uniform_buffer_object ) { // 80%
    printf( "ARB_uniform_buffer_object = yes\n" );
  } else {
    printf( "ARB_uniform_buffer_object = no\n" );
    has_all_vital = false;
  }

  // core since 3.3
  if ( GLEW_ARB_timer_query ) {
    printf( "ARB_timer_query = yes\n" );
    // TODO glGenQueries( 1, &g_gfx.gpu_timer_query );
  } else {
    printf( "ARB_timer_query = no\n" );
  }

  // this is core in 4.3+ but i'm using 4.1
  if ( GLEW_ARB_debug_output ) { // glcapsviewer reports 47% support
    printf( "ARB_debug_output = yes\n" );
#ifdef DEBUG_GL
    // Enabling synchronous debug output greatly simplifies the responsibilities of
    // the application for making its callback functions thread-safe, but may
    // potentially result in drastically reduced driver performance.
    glDebugMessageCallbackARB( _debug_gl_callback, NULL );
    glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
    printf( "~~~~DEBUG OUTPUT EXTENSION ENGAGED!~~~~\n" );
    printf( "~~~~DEBUG OUTPUT EXTENSION ENGAGED!~~~~\n" );
#else
    printf( "debug build not enabled\n" );
#endif
  } else {
    printf( "ARB_debug_output = no\n" );
  }

  if ( GL_NVX_gpu_memory_info ) {
    GLint dedmem = 0, totalavailmem = 0, currentavail = 0, eviction = 0, evicted = 0;
    glGetIntegerv( GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &dedmem );
    glGetIntegerv( GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalavailmem );
    glGetIntegerv( GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &currentavail );
    glGetIntegerv( GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX, &eviction );
    glGetIntegerv( GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, &evicted );
    printf(
      "NVX_gpu_memory_info = yes\n  dedicated %i kB\n  total avail %i kB\n  "
      "current avail %i kB\n  evictions %i\n  evicted %i kB\n",
      dedmem, totalavailmem, currentavail, eviction, evicted );
  } else {
    printf( "NVX_gpu_memory_info = no\n" );
  }

  /* Need to parse out that we do in fact an ATI first or this runs and produces
  enum errors
  if ( GL_ATI_meminfo ) {
    printf(  "GL_ATI_meminfo = yes\n");

    int param[4] = { 0 };
    glGetIntegerv( GL_VBO_FREE_MEMORY_ATI, param );
    printf( " GL_VBO_FREE_MEMORY_ATI:\n");
    printf( "  total dedicted gfx mem: %i kB\n", param[0]);
    printf( "   largest available block: %i kB\n", param[1]);
    printf( "  total free shared system mem: %i kB\n", param[2]);
    printf( "   largest available block: %i kB\n", param[3]);

    glGetIntegerv( GL_TEXTURE_FREE_MEMORY_ATI, param );
    printf( " GL_TEXTURE_FREE_MEMORY_ATI:\n");
    printf( "  total dedicted gfx mem: %i kB\n", param[0]);
    printf( "   largest available block: %i kB\n", param[1]);
    printf( "  total free shared system mem: %i kB\n", param[2]);
    printf( "   largest available block: %i kB\n", param[3]);

    glGetIntegerv( GL_RENDERBUFFER_FREE_MEMORY_ATI, param );
    printf( " GL_RENDERBUFFER_FREE_MEMORY_ATI:\n");
    printf( "  total dedicted gfx mem: %i kB\n", param[0]);
    printf( "   largest available block: %i kB\n", param[1]);
    printf( "  total free shared system mem: %i kB\n", param[2]);
    printf( "   largest available block: %i kB\n", param[3]);
  } else {
    printf(  "GL_ATI_meminfo = no\n");
  }
  */

  return has_all_vital;
}

void wireframe_mode() { glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); }

void polygon_mode() { glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ); }

bool start_gfx() {
  {
    glfwSetErrorCallback( _glfw_error_callback );
    if ( !glfwInit() ) {
      fprintf( stderr, "ERROR: could not start GLFW3\n" );
      return false;
    }
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

    g_window = glfwCreateWindow( g_win_width, g_win_height, "Voxel Test", NULL, NULL );
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

  if ( !_extension_checks() ) { return false; }

  _init_unit_cube();

  return true;
}

void stop_gfx() { glfwTerminate(); }

bool should_window_close() {
  assert( g_window );

  return glfwWindowShouldClose( g_window );
}

void buffer_colour( float r, float g, float b, float a ) { glClearColor( r, g, b, a ); }

void clear_colour_and_depth_buffers() {
  glViewport( 0, 0, g_win_width, g_win_height );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  {
    glDepthFunc( GL_LESS );
    glEnable( GL_DEPTH_TEST );
    glCullFace( GL_BACK );
    glFrontFace( GL_CCW );
    glEnable( GL_CULL_FACE );
    // glEnable( GL_CLIP_DISTANCE0 ); // clip plane A
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable( GL_BLEND );
  }
}

void swap_buffer_and_poll_events() {
  assert( g_window );

  glfwPollEvents();
  glfwSwapBuffers( g_window );
}

////////////////////////////////////////// geometry ////////////////////////////////////////////////

mesh_t create_mesh( const void* data, size_t sz, geom_mem_layout_t layout, unsigned int vcount, draw_mode_t mode ) {
  mesh_t mesh;
  memset( &mesh, 0, sizeof( mesh_t ) );
  mesh.vcount = vcount;

  GLenum gl_draw_mode = 0;
  switch ( mode ) {
  case STATIC_DRAW: {
    gl_draw_mode = GL_STATIC_DRAW;
  } break;
  case DYNAMIC_DRAW: {
    gl_draw_mode = GL_DYNAMIC_DRAW;
  } break;
  case STREAM_DRAW: {
    gl_draw_mode = GL_STREAM_DRAW;
  } break;
  }

  glGenVertexArrays( 1, &mesh.vao );
  glGenBuffers( 1, &mesh.vbo );

  glBindVertexArray( mesh.vao );
  glBindBuffer( GL_ARRAY_BUFFER, mesh.vbo );
  {
    glBufferData( GL_ARRAY_BUFFER, sz, data, gl_draw_mode );

    switch ( layout ) {
    case MEM_POS: {
      GLsizei vertex_stride = sizeof( float ) * 3;
      assert( vertex_stride * vcount == sz );
      glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL ); // XYZ so 3
      glEnableVertexAttribArray( VP_LOCATION );
    } break;
    case MEM_POS_ST: {
      GLsizei vertex_stride = sizeof( float ) * 5;
      assert( vertex_stride * vcount == sz );
      GLintptr vertex_position_offset = 0 * sizeof( float );
      GLintptr vertex_texcoord_offset = 3 * sizeof( float );
      glVertexAttribPointer( VP_LOCATION, 3, GL_FLOAT, GL_FALSE, vertex_stride, (GLvoid*)vertex_position_offset );
      glVertexAttribPointer( VT_LOCATION, 2, GL_FLOAT, GL_FALSE, vertex_stride, (GLvoid*)vertex_texcoord_offset );
      glEnableVertexAttribArray( VP_LOCATION );
      glEnableVertexAttribArray( VT_LOCATION );
    } break;
    case MEM_POS_ST_R: {
      GLsizei vertex_stride = sizeof( float ) * 6;
      assert( vertex_stride * vcount == sz );
      GLintptr vertex_position_offset = 0 * sizeof( float );
      GLintptr vertex_texcoord_offset = 3 * sizeof( float );
      GLintptr vertex_red_offset      = 5 * sizeof( float );
      glVertexAttribPointer( VP_LOCATION, 3, GL_FLOAT, GL_FALSE, vertex_stride, (GLvoid*)vertex_position_offset );
      glVertexAttribPointer( VT_LOCATION, 2, GL_FLOAT, GL_FALSE, vertex_stride, (GLvoid*)vertex_texcoord_offset );
      glVertexAttribPointer( VC_LOCATION, 1, GL_FLOAT, GL_FALSE, vertex_stride, (GLvoid*)vertex_red_offset ); // just R so 1
      glEnableVertexAttribArray( VP_LOCATION );
      glEnableVertexAttribArray( VT_LOCATION );
      glEnableVertexAttribArray( VC_LOCATION );
    } break;
    }
  }
  glBindBuffer( GL_ARRAY_BUFFER, 0 );
  glBindVertexArray( 0 );

  return mesh;
}

mesh_t unit_cube_mesh() {
  mesh_t mesh;
  mesh.vao    = g_unit_cube_vao;
  mesh.vbo    = g_unit_cube_vbo;
  mesh.vcount = 6 * 6 * 3;
  return mesh;
}

void delete_mesh( mesh_t* mesh ) {
  assert( mesh );

  glDeleteBuffers( 1, &mesh->vbo );
  glDeleteVertexArrays( 1, &mesh->vao );
  memset( mesh, 0, sizeof( mesh_t ) );
}

////////////////////////////////////////// shaders /////////////////////////////////////////////////

shader_t create_shader( const char* vs_str, const char* fs_str ) {
  assert( vs_str && fs_str );

  shader_t shader;

  shader.vertex_shader = glCreateShader( GL_VERTEX_SHADER );
  glShaderSource( shader.vertex_shader, 1, &vs_str, NULL );
  glCompileShader( shader.vertex_shader );
  int params = -1;
  glGetShaderiv( shader.vertex_shader, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    fprintf( stderr, "ERROR: shader index %u did not compile\n", shader.vertex_shader );
    fprintf( stderr, "  type = vertex shader\n" );
    int max_length    = 2048;
    int actual_length = 0;
    char slog[2048];
    glGetShaderInfoLog( shader.vertex_shader, max_length, &actual_length, slog );
    fprintf( stderr, "shader info log for GL index %u:\n%s\n", shader.vertex_shader, slog );
    if ( 0 != shader.vertex_shader ) {
      glDeleteShader( shader.vertex_shader );
      shader.vertex_shader = 0;
    }
    assert( false ); // return false?
  }

  shader.fragment_shader = glCreateShader( GL_FRAGMENT_SHADER );
  glShaderSource( shader.fragment_shader, 1, &fs_str, NULL );
  glCompileShader( shader.fragment_shader );
  params = -1;
  glGetShaderiv( shader.fragment_shader, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    fprintf( stderr, "ERROR: shader index %u did not compile\n", shader.fragment_shader );
    fprintf( stderr, "  type = frag shader\n" );
    int max_length    = 2048;
    int actual_length = 0;
    char slog[2048];
    glGetShaderInfoLog( shader.fragment_shader, max_length, &actual_length, slog );
    fprintf( stderr, "shader info log for GL index %u:\n%s\n", shader.fragment_shader, slog );
    if ( 0 != shader.fragment_shader ) {
      glDeleteShader( shader.fragment_shader );
      shader.fragment_shader = 0;
    }
    assert( false ); // return false?
  }

  shader.program = glCreateProgram();
  glAttachShader( shader.program, shader.vertex_shader );
  glAttachShader( shader.program, shader.fragment_shader );
  glBindAttribLocation( shader.program, VP_LOCATION, "vp" );
  glBindAttribLocation( shader.program, VT_LOCATION, "vt" );
  glBindAttribLocation( shader.program, VN_LOCATION, "vn" );
  glBindAttribLocation( shader.program, VC_LOCATION, "vc" );
  glBindAttribLocation( shader.program, INSTANCE_P_WOR_LOCATION, "instance_p_wor" );
  glBindAttribLocation( shader.program, INSTANCE_YR_LOCATION, "vyr" );
  glLinkProgram( shader.program );

  params = -1;
  glGetProgramiv( shader.program, GL_LINK_STATUS, &params );
  if ( GL_TRUE != params ) {
    fprintf( stderr, "ERROR: could not link shader program GL index %u\n", shader.program );
    int max_length    = 2048;
    int actual_length = 0;
    char plog[2048];
    glGetProgramInfoLog( shader.program, max_length, &actual_length, plog );
    fprintf( stderr, "program info log for GL index %u:\n%s", shader.program, plog );

    delete_shader( &shader );

    assert( false ); // return false?
  }

  return shader;
}

void delete_shader( shader_t* shader ) {
  assert( shader );

  glDeleteShader( shader->vertex_shader );
  glDeleteShader( shader->fragment_shader );
  glDeleteProgram( shader->program );
  memset( shader, 0, sizeof( shader_t ) );
}

int uniform_loc( shader_t shader, const char* name ) {
  assert( name );
  assert( shader.program );

  return glGetUniformLocation( shader.program, name );
}

void uniform_mat4( shader_t shader, int loc, const float* m ) {
  assert( m );

  glUseProgram( shader.program );
  glUniformMatrix4fv( loc, 1, GL_FALSE, m );
  glUseProgram( 0 );
}

void uniform_1i( shader_t shader, int loc, int i ) {
  glUseProgram( shader.program );
  glUniform1i( loc, i );
  glUseProgram( 0 );
}

////////////////////////////////////////// textures ////////////////////////////////////////////////

texture_t load_image_to_texture( const char* filename ) {
  assert( filename );

  texture_t texture;
  strcpy( texture.filename, filename );

  int width = 0, height = 0, comps = 0, req_comps = 0;
  unsigned char* pixels = stbi_load( filename, &width, &height, &comps, req_comps );
  assert( pixels ); // should just use a built-in texture here
  {
    glGenTextures( 1, &texture.handle );
    glBindTexture( GL_TEXTURE_2D, texture.handle );
    {
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );

      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels );
      glGenerateMipmap( GL_TEXTURE_2D );
      // anisotropy can go here
    }
    glBindTexture( GL_TEXTURE_2D, 0 );
  }
  stbi_image_free( pixels );

  return texture;
}

void draw_mesh( mesh_t mesh, shader_t shader ) {
  glUseProgram( shader.program );
  {
    glBindVertexArray( mesh.vao );
    glDrawArrays( GL_TRIANGLES, 0, mesh.vcount );
    glBindVertexArray( 0 );
  }
  glUseProgram( 0 );
}

void draw_mesh_textured( mesh_t mesh, shader_t shader, texture_t texture ) {
  glUseProgram( shader.program );
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, texture.handle );
  {
    glBindVertexArray( mesh.vao );
    glDrawArrays( GL_TRIANGLES, 0, mesh.vcount );
    glBindVertexArray( 0 );
  }
  glBindTexture( GL_TEXTURE_2D, 0 );
  glUseProgram( 0 );
}

void draw_mesh_texturedv( mesh_t mesh, shader_t shader, texture_t* textures, int ntextures ) {
  assert( textures );

  glUseProgram( shader.program );
  for (int i = 0; i < ntextures; i++ ) {
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( GL_TEXTURE_2D, textures[i].handle );
  }
  {
    glBindVertexArray( mesh.vao );
    glDrawArrays( GL_TRIANGLES, 0, mesh.vcount );
    glBindVertexArray( 0 );
  }
  for (int i = 0; i < ntextures; i++ ) {
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( GL_TEXTURE_2D, 0 );
  }
  glUseProgram( 0 );
}
