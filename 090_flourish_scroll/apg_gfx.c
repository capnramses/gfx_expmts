/*
"New Voxel Project"
Copyright: Anton Gerdelan <antonofnote@gmail.com>
C99
*/

#include "apg_gfx.h"
#include "apg_glcontext.h"
#define STB_IMAGE_WRITE_IMPLEMENATION
#include "stb/stb_image_write.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define SHADER_BINDING_VP 0
#define SHADER_BINDING_VT 1
#define SHADER_BINDING_VN 2 /* 4th channel used to store 'heightmap surface' factor of 1 or 0 */
#define SHADER_BINDING_VC 3
#define SHADER_BINDING_VPAL_IDX 4
#define SHADER_BINDING_VPICKING 5 /* special colours to help picking algorithm */
#define SHADER_BINDING_VEDGE 6
#define SHADER_BINDING_INSTANCED 7

#define GFX_MAX_SHADER_STR 10000
#define GFX_N_SHADERS_MAX 256

GLFWwindow* gfx_window_ptr; // extern in apg_glcontext.h
gfx_shader_t gfx_default_shader, gfx_text_shader;

static GLFWmonitor* gfx_monitor_ptr;
static gfx_shader_t* _shaders[GFX_N_SHADERS_MAX];
static int _n_shaders;
static int g_win_width = 1920, g_win_height = 1080;
static gfx_mesh_t _ss_quad_mesh;
static gfx_stats_t _stats;

static void _init_ss_quad() {
  float ss_quad_pos[] = { -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0, -1.0 };
  _ss_quad_mesh       = gfx_create_mesh_from_mem( ss_quad_pos, 2, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, 4, false );
}

static bool _recompile_shader_with_check( GLuint shader, const char* src_str ) {
  glShaderSource( shader, 1, &src_str, NULL );
  glCompileShader( shader );
  int params = -1;
  glGetShaderiv( shader, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    fprintf( stderr, "ERROR: shader index %u did not compile. src:\n%s\n", shader, src_str );
    int max_length    = 2048;
    int actual_length = 0;
    char slog[2048];
    glGetShaderInfoLog( shader, max_length, &actual_length, slog );
    fprintf( stderr, "shader info log for GL index %u:\n%s\n", shader, slog );
    if ( 0 != shader ) { glDeleteShader( shader ); }
    return false;
  }
  return true;
}

gfx_shader_t gfx_create_shader_program_from_strings( const char* vert_shader_str, const char* frag_shader_str ) {
  assert( vert_shader_str && frag_shader_str );
  gfx_shader_t shader = ( gfx_shader_t ){ .program_gl = 0 };
  GLuint vs           = glCreateShader( GL_VERTEX_SHADER );
  GLuint fs           = glCreateShader( GL_FRAGMENT_SHADER );
  bool res_a          = _recompile_shader_with_check( vs, vert_shader_str );
  bool res_b          = _recompile_shader_with_check( fs, frag_shader_str );
  shader.program_gl   = glCreateProgram();
  glAttachShader( shader.program_gl, fs );
  glAttachShader( shader.program_gl, vs );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VP, "a_vp" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VT, "a_vt" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VN, "a_vn" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VC, "a_vc" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VPAL_IDX, "a_vpal_idx" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VPICKING, "a_vpicking" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VEDGE, "a_vedge" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_INSTANCED, "a_instanced" );
  glLinkProgram( shader.program_gl );
  glDeleteShader( vs );
  glDeleteShader( fs );

  shader.u_M               = glGetUniformLocation( shader.program_gl, "u_M" );
  shader.u_V               = glGetUniformLocation( shader.program_gl, "u_V" );
  shader.u_P               = glGetUniformLocation( shader.program_gl, "u_P" );
  shader.u_fwd             = glGetUniformLocation( shader.program_gl, "u_fwd" );
  shader.u_rgb             = glGetUniformLocation( shader.program_gl, "u_rgb" );
  shader.u_output_tint_rgb = glGetUniformLocation( shader.program_gl, "u_output_tint_rgb" );
  shader.u_scale           = glGetUniformLocation( shader.program_gl, "u_scale" );
  shader.u_pos             = glGetUniformLocation( shader.program_gl, "u_pos" );
  shader.u_offset          = glGetUniformLocation( shader.program_gl, "u_offset" );
  shader.u_alpha           = glGetUniformLocation( shader.program_gl, "u_alpha" );
  shader.u_texture_a       = glGetUniformLocation( shader.program_gl, "u_texture_a" );
  shader.u_texture_b       = glGetUniformLocation( shader.program_gl, "u_texture_b" );
  shader.u_scroll          = glGetUniformLocation( shader.program_gl, "u_scroll" );
  shader.u_time            = glGetUniformLocation( shader.program_gl, "u_time" );
  shader.u_walk_time       = glGetUniformLocation( shader.program_gl, "u_walk_time" );
  shader.u_cam_y_rot       = glGetUniformLocation( shader.program_gl, "u_cam_y_rot" );
  shader.u_chunk_id        = glGetUniformLocation( shader.program_gl, "u_chunk_id" );
  shader.u_fog_factor      = glGetUniformLocation( shader.program_gl, "u_fog_factor" );

  if ( shader.u_texture_a > -1 ) { glProgramUniform1i( shader.program_gl, shader.u_texture_a, 0 ); }
  if ( shader.u_texture_b > -1 ) { glProgramUniform1i( shader.program_gl, shader.u_texture_b, 1 ); }
  if ( shader.u_output_tint_rgb > -1 ) { glProgramUniform3f( shader.program_gl, shader.u_output_tint_rgb, 1.0f, 1.0f, 1.0f ); }

  bool linked = true;
  int params  = -1;
  glGetProgramiv( shader.program_gl, GL_LINK_STATUS, &params );
  if ( GL_TRUE != params ) {
    fprintf( stderr, "ERROR: could not link shader program GL index %u\n", shader.program_gl );
    int max_length    = 2048;
    int actual_length = 0;
    char plog[2048];
    glGetProgramInfoLog( shader.program_gl, max_length, &actual_length, plog );
    fprintf( stderr, "program info log for GL index %u:\n%s", shader.program_gl, plog );
    gfx_delete_shader_program( &shader );
    linked = false;
  }
  // fall back to default shader
  if ( !res_a || !res_b || !linked ) { return gfx_default_shader; }
  shader.loaded = true;
  return shader;
}

gfx_shader_t gfx_create_shader_program_from_files( const char* vert_shader_filename, const char* frag_shader_filename ) {
  gfx_shader_t shader = ( gfx_shader_t ){ .program_gl = 0 };

  char vert_shader_str[GFX_MAX_SHADER_STR], frag_shader_str[GFX_MAX_SHADER_STR];
  {
    FILE* f_ptr = fopen( vert_shader_filename, "rb" );
    if ( !f_ptr ) {
      fprintf( stderr, "ERROR: opening shader file `%s`\n", vert_shader_filename );
      return shader; // TODO return fallback shader as per dwarf3D code
    }
    size_t count = fread( vert_shader_str, 1, GFX_MAX_SHADER_STR - 1, f_ptr );
    assert( count < GFX_MAX_SHADER_STR - 1 ); // file was too long
    vert_shader_str[count] = '\0';
    fclose( f_ptr );
  }
  {
    FILE* f_ptr = fopen( frag_shader_filename, "rb" );
    if ( !f_ptr ) {
      fprintf( stderr, "ERROR: opening shader file `%s`\n", frag_shader_filename );
      return shader; // TODO return fallback shader as per dwarf3D code
    }
    size_t count = fread( frag_shader_str, 1, GFX_MAX_SHADER_STR - 1, f_ptr );
    assert( count < GFX_MAX_SHADER_STR - 1 ); // file was too long
    frag_shader_str[count] = '\0';
    fclose( f_ptr );
  }

  shader = gfx_create_shader_program_from_strings( vert_shader_str, frag_shader_str );
  strncpy( shader.vs_filename, vert_shader_filename, GFX_SHADER_PATH_MAX - 1 );
  strncpy( shader.fs_filename, frag_shader_filename, GFX_SHADER_PATH_MAX - 1 );
  shader.vs_filename[GFX_SHADER_PATH_MAX - 1] = shader.fs_filename[GFX_SHADER_PATH_MAX - 1] = '\0';
  if ( !shader.program_gl ) { fprintf( stderr, "ERROR: failed to compile shader from files `%s` and `%s`\n", vert_shader_filename, frag_shader_filename ); }
  return shader;
}

void gfx_delete_shader_program( gfx_shader_t* shader ) {
  assert( shader );
  if ( shader->program_gl ) { glDeleteProgram( shader->program_gl ); }
  memset( shader, 0, sizeof( gfx_shader_t ) );
}

void gfx_shader_register( gfx_shader_t* shader ) {
  assert( shader );

  if ( _n_shaders >= GFX_N_SHADERS_MAX ) {
    fprintf( stderr, "ERROR: can't register shader. Too many shaders\n" );
    return;
  }

  _shaders[_n_shaders++] = shader;
}

void gfx_shaders_reload() {
  for ( int i = 0; i < _n_shaders; i++ ) {
    if ( !_shaders[i] ) { continue; }
    char vs_filename[GFX_SHADER_PATH_MAX], fs_filename[GFX_SHADER_PATH_MAX];
    strncpy( vs_filename, _shaders[i]->vs_filename, GFX_SHADER_PATH_MAX - 1 );
    strncpy( fs_filename, _shaders[i]->fs_filename, GFX_SHADER_PATH_MAX - 1 );
    _shaders[i]->vs_filename[GFX_SHADER_PATH_MAX - 1] = _shaders[i]->fs_filename[GFX_SHADER_PATH_MAX - 1] = '\0';

    gfx_delete_shader_program( _shaders[i] );
    *_shaders[i] = gfx_create_shader_program_from_files( vs_filename, fs_filename );
  }
}

int gfx_uniform_loc( gfx_shader_t shader, const char* name ) {
  assert( shader.program_gl );
  int l = glGetUniformLocation( shader.program_gl, name );
  return l;
}

void gfx_uniform1i( gfx_shader_t shader, int loc, int i ) {
  assert( shader.program_gl );
  glProgramUniform1i( shader.program_gl, loc, i );
}

void gfx_uniform1f( gfx_shader_t shader, int loc, float f ) {
  assert( shader.program_gl );
  glProgramUniform1f( shader.program_gl, loc, f );
}
void gfx_uniform2f( gfx_shader_t shader, int loc, float x, float y ) {
  assert( shader.program_gl );
  glProgramUniform2f( shader.program_gl, loc, x, y );
}
void gfx_uniform3f( gfx_shader_t shader, int loc, float x, float y, float z ) {
  assert( shader.program_gl );
  glProgramUniform3f( shader.program_gl, loc, x, y, z );
}
void gfx_uniform4f( gfx_shader_t shader, int loc, float x, float y, float z, float w ) {
  assert( shader.program_gl );
  glProgramUniform4f( shader.program_gl, loc, x, y, z, w );
}

void gfx_fullscreen( bool fullscreen ) {
  if ( !gfx_window_ptr ) { return; }
  if ( !gfx_monitor_ptr ) { gfx_monitor_ptr = glfwGetPrimaryMonitor(); }
  const GLFWvidmode* mode = glfwGetVideoMode( gfx_monitor_ptr );
  if ( fullscreen ) {
    glfwSetWindowMonitor( gfx_window_ptr, gfx_monitor_ptr, 0, 0, mode->width, mode->height, mode->refreshRate );
  } else {
    glfwSetWindowMonitor( gfx_window_ptr, NULL, 0, 0, 800, 600, GLFW_DONT_CARE ); // The refresh rate is ignored when no monitor is specified.
    glfwSetWindowPos( gfx_window_ptr, 0, 32 ); // make sure toolbar is not off-screen (Windows!). doesn't seem to work in params to above func.
  }
}

// 0 normal, 1 hidden, 2 disabled
void gfx_cursor_mode( gfx_cursor_mode_t mode ) {
  assert( mode >= 0 && mode < 3 );
  glfwSetInputMode( gfx_window_ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode );
}

gfx_cursor_mode_t gfx_get_cursor_mode() {
  int mode = glfwGetInputMode( gfx_window_ptr, GLFW_CURSOR ) - GLFW_CURSOR_NORMAL;
  assert( mode >= 0 && mode < 3 );
  return (gfx_cursor_mode_t)mode;
}

bool gfx_start( const char* window_title, bool fullscreen ) {
  {
    if ( !glfwInit() ) {
      fprintf( stderr, "ERROR: could not start GLFW3\n" );
      return false;
    }
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    { // MSAA hint
      // NOTE(Anton) fetching max samples didn't work on my linux nvidia driver before window creation, which kind of defeats the purpose of asking
      int nsamples = 16;
      glfwWindowHint( GLFW_SAMPLES, nsamples );
      printf( "MSAA = x%i\n", nsamples );
    }
    if ( fullscreen ) {
      gfx_monitor_ptr         = glfwGetPrimaryMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode( gfx_monitor_ptr );
      glfwWindowHint( GLFW_RED_BITS, mode->redBits );
      glfwWindowHint( GLFW_GREEN_BITS, mode->greenBits );
      glfwWindowHint( GLFW_BLUE_BITS, mode->blueBits );
      glfwWindowHint( GLFW_REFRESH_RATE, mode->refreshRate );
      g_win_width  = mode->width;
      g_win_height = mode->height;
    }

    gfx_window_ptr = glfwCreateWindow( g_win_width, g_win_height, window_title, gfx_monitor_ptr, NULL );
    if ( !gfx_window_ptr ) {
      fprintf( stderr, "ERROR: could not open window with GLFW3\n" );
      glfwTerminate();
      return false;
    }
    glfwMakeContextCurrent( gfx_window_ptr );
  }
#ifdef APG_USE_GLEW
  glewExperimental = GL_TRUE;
  glewInit();
#else
  if ( !gladLoadGL() ) {
    fprintf( stderr, "FATAL ERROR: Could not load OpenGL header using Glad.\n" );
    return false;
  }
  printf( "OpenGL %d.%d\n", GLVersion.major, GLVersion.minor );
#endif

  const GLubyte* renderer = glGetString( GL_RENDERER );
  const GLubyte* version  = glGetString( GL_VERSION );
  printf( "Renderer: %s\n", renderer );
  printf( "OpenGL version supported %s\n", version );
  { // check extensions and size limits here
    int layers = 0;
    glGetIntegerv( GL_MAX_ARRAY_TEXTURE_LAYERS, &layers );
    printf( "  GL_MAX_ARRAY_TEXTURE_LAYERS %i\n", layers );
  }
  {
    // IMPORTANT!!!! NOTE THE SCALE of x0.1 here!
    const char* vertex_shader =
      "#version 410 core\n"
      "in vec3 a_vp;\n"
      "in vec3 a_vc;\n"
      "uniform mat4 u_P, u_V, u_M;\n"
      "out vec3 vc;\n"
      "void main () {\n"
      "  vc = a_vc;\n"
      "  gl_Position = u_P * u_V * u_M * vec4( a_vp * 0.1, 1.0 );\n"
      "}\n";
    const char* fragment_shader =
      "#version 410 core\n"
      "in vec3 vc;\n"
      "uniform float u_alpha;\n"
      "out vec4 o_frag_colour;\n"
      "void main () {\n"
      "  o_frag_colour = vec4( vc, u_alpha );\n"
      "  o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );\n"
      "}\n";
    gfx_default_shader = gfx_create_shader_program_from_strings( vertex_shader, fragment_shader );
    if ( !gfx_default_shader.loaded ) { return false; }
  }
  {
    const char* vertex_shader =
      "#version 410 core\n"
      "in vec2 a_vp;\n"
      "uniform vec2 u_scale, u_pos;\n"
      "out vec2 v_st;\n"
      "void main () {\n"
      "  v_st = a_vp.xy * 0.5 + 0.5;\n"
      "  v_st.t = 1.0 - v_st.t;\n"
      "  gl_Position = vec4( a_vp * u_scale + u_pos, 0.0, 1.0 );\n"
      "}\n";
    const char* fragment_shader =
      "#version 410 core\n"
      "in vec2 v_st;\n"
      "uniform sampler2D u_tex;\n"
      "out vec4 o_frag_colour;\n"
      "void main () {\n"
      "  vec4 texel = texture( u_tex, v_st );\n"
      "  o_frag_colour = texel;\n"
      "  o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );\n"
      "}\n";
    gfx_text_shader = gfx_create_shader_program_from_strings( vertex_shader, fragment_shader );
    if ( !gfx_text_shader.loaded ) { return false; }
  }
  _init_ss_quad();

  glClearColor( 0.5, 0.5, 0.5, 1.0 );
  glDepthFunc( GL_LESS );
  glEnable( GL_DEPTH_TEST );
  glCullFace( GL_BACK );
  glFrontFace( GL_CCW );
  glEnable( GL_CULL_FACE );

  glfwSwapInterval( 0 ); // vsync

  return true;
}
void gfx_stop() {
  gfx_delete_shader_program( &gfx_default_shader );
  gfx_delete_shader_program( &gfx_text_shader );
  glfwTerminate();
}

void gfx_depth_writing( bool enable ) {
  if ( enable ) {
    glDepthMask( GL_TRUE );
  } else {
    glDepthMask( GL_FALSE );
  }
}

void gfx_depth_testing( bool enable ) {
  if ( enable ) {
    glDepthFunc( GL_LESS );
    glEnable( GL_DEPTH_TEST );
  } else {
    glDisable( GL_DEPTH_TEST );
  }
}

void gfx_alpha_testing( bool enable ) {
  if ( enable ) {
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_BLEND );
  } else {
    glDisable( GL_BLEND );
  }
}
void gfx_backface_culling( bool enable ) {
  if ( enable ) {
    glCullFace( GL_BACK );
    glFrontFace( GL_CCW );
    glEnable( GL_CULL_FACE );
  } else {
    glDisable( GL_CULL_FACE );
  }
}

void gfx_clip_planes( bool enable ) {
  if ( enable ) {
    glEnable( GL_CLIP_DISTANCE0 ); // clip plane A
    glEnable( GL_CLIP_DISTANCE1 ); // clip plane B
  } else {
    glDisable( GL_CLIP_DISTANCE0 );
    glDisable( GL_CLIP_DISTANCE1 );
  }
}

const char* gfx_renderer_str( void ) { return (const char*)glGetString( GL_RENDERER ); }

void gfx_update_mesh_from_mem( gfx_mesh_t* mesh, const float* points_buffer, int n_points_comps, const uint32_t* pal_idx_buffer, int n_pal_idx_comps,
  const float* picking_buffer, int n_picking_comps, const float* texcoords_buffer, int n_texcoord_comps, const float* normals_buffer, int n_normal_comps,
  const float* vcolours_buffer, int n_vcolour_comps, const float* edges_buffer, int n_edge_comps, int n_vertices, bool dynamic ) {
  assert( points_buffer && n_points_comps > 0 );
  if ( !points_buffer || n_points_comps <= 0 ) { return; }
  assert( mesh && mesh->vao && mesh->points_vbo );
  if ( !mesh || !mesh->vao || !mesh->points_vbo ) { return; }

  GLenum usage     = !dynamic ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
  mesh->n_vertices = n_vertices;
  mesh->dynamic    = dynamic;

  glBindVertexArray( mesh->vao );
  {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->points_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_points_comps * n_vertices, points_buffer, usage );
    glEnableVertexAttribArray( SHADER_BINDING_VP );
    glVertexAttribPointer( SHADER_BINDING_VP, n_points_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( pal_idx_buffer && n_pal_idx_comps > 0 && mesh->palidx_vbo ) {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->palidx_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( uint32_t ) * n_pal_idx_comps * n_vertices, pal_idx_buffer, usage );
    glEnableVertexAttribArray( SHADER_BINDING_VPAL_IDX );
    glVertexAttribIPointer( SHADER_BINDING_VPAL_IDX, 1, GL_UNSIGNED_INT, 0, NULL ); // NOTE(Anton) ...IPointer... variant
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( picking_buffer && n_picking_comps > 0 && mesh->picking_vbo ) {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->picking_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_picking_comps * n_vertices, picking_buffer, usage );
    glEnableVertexAttribArray( SHADER_BINDING_VPICKING );
    glVertexAttribPointer( SHADER_BINDING_VPICKING, n_picking_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( texcoords_buffer && n_texcoord_comps > 0 && mesh->texcoords_vbo ) {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->texcoords_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_texcoord_comps * n_vertices, texcoords_buffer, usage );
    glEnableVertexAttribArray( SHADER_BINDING_VT );
    glVertexAttribPointer( SHADER_BINDING_VT, n_texcoord_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( normals_buffer && n_normal_comps > 0 && mesh->normals_vbo ) {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->normals_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_normal_comps * n_vertices, normals_buffer, usage );
    glEnableVertexAttribArray( SHADER_BINDING_VN );
    glVertexAttribPointer( SHADER_BINDING_VN, n_normal_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( vcolours_buffer && n_vcolour_comps > 0 && mesh->vcolours_vbo ) {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->vcolours_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_vcolour_comps * n_vertices, vcolours_buffer, usage );
    glEnableVertexAttribArray( SHADER_BINDING_VC );
    glVertexAttribPointer( SHADER_BINDING_VC, n_vcolour_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( edges_buffer && n_edge_comps > 0 && mesh->vedges_vbo ) {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->vedges_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_edge_comps * n_vertices, edges_buffer, usage );
    glEnableVertexAttribArray( SHADER_BINDING_VEDGE );
    glVertexAttribPointer( SHADER_BINDING_VEDGE, n_edge_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  glBindVertexArray( 0 );
}

void gfx_mesh_gen_instanced_buffer( gfx_mesh_t* mesh ) {
  if ( !mesh ) { return; }
  glGenBuffers( 1, &mesh->instanced_vbo );
}

/* PARAMS - divisor. Set to 0 to use as regular attribute. Set to 1 to increment 1 element per instanced draw. 2 to increment every 2 instanced draws, etc. */
void gfx_mesh_update_instanced_buffer( gfx_mesh_t* mesh, const float* buffer, int n_comps, int n_elements, int divisor ) {
  if ( !mesh || !mesh->vao || !mesh->instanced_vbo ) { return; }

  glBindVertexArray( mesh->vao );
  {
    glEnableVertexAttribArray( SHADER_BINDING_INSTANCED );
    glBindBuffer( GL_ARRAY_BUFFER, mesh->instanced_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_comps * n_elements, buffer, GL_STATIC_DRAW );
    GLsizei stride = n_comps * sizeof( float ); // NOTE(Anton) do we need this for instanced?
    glVertexAttribPointer( SHADER_BINDING_INSTANCED, n_comps, GL_FLOAT, GL_FALSE, stride, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glVertexAttribDivisor( SHADER_BINDING_INSTANCED, divisor );
  }
  glBindVertexArray( 0 );
}

gfx_mesh_t gfx_create_mesh_from_mem( const float* points_buffer, int n_points_comps, const uint32_t* pal_idx_buffer, int n_pal_idx_comps,
  const float* picking_buffer, int n_picking_comps, const float* texcoords_buffer, int n_texcoord_comps, const float* normals_buffer, int n_normal_comps,
  const float* vcolours_buffer, int n_vcolour_comps, const float* edges_buffer, int n_edge_comps, int n_vertices, bool dynamic ) {
  assert( points_buffer && n_points_comps > 0 );

  gfx_mesh_t mesh = ( gfx_mesh_t ){ .n_vertices = n_vertices, .dynamic = dynamic };

  glGenVertexArrays( 1, &mesh.vao );
  glGenBuffers( 1, &mesh.points_vbo );
  if ( pal_idx_buffer && n_pal_idx_comps > 0 ) { glGenBuffers( 1, &mesh.palidx_vbo ); }
  if ( picking_buffer && n_picking_comps > 0 ) { glGenBuffers( 1, &mesh.picking_vbo ); }
  if ( texcoords_buffer && n_texcoord_comps > 0 ) { glGenBuffers( 1, &mesh.texcoords_vbo ); }
  if ( normals_buffer && n_normal_comps > 0 ) { glGenBuffers( 1, &mesh.normals_vbo ); }
  if ( vcolours_buffer && n_vcolour_comps > 0 ) { glGenBuffers( 1, &mesh.vcolours_vbo ); }
  if ( edges_buffer && n_edge_comps > 0 ) { glGenBuffers( 1, &mesh.vedges_vbo ); }

  gfx_update_mesh_from_mem( &mesh, points_buffer, n_points_comps, pal_idx_buffer, n_pal_idx_comps, picking_buffer, n_picking_comps, texcoords_buffer,
    n_texcoord_comps, normals_buffer, n_normal_comps, vcolours_buffer, n_vcolour_comps, edges_buffer, n_edge_comps, n_vertices, dynamic );

  return mesh;
}

void gfx_delete_mesh( gfx_mesh_t* mesh ) {
  assert( mesh );

  if ( mesh->instanced_vbo ) { glDeleteBuffers( 1, &mesh->instanced_vbo ); }
  if ( mesh->vcolours_vbo ) { glDeleteBuffers( 1, &mesh->vcolours_vbo ); }
  if ( mesh->normals_vbo ) { glDeleteBuffers( 1, &mesh->normals_vbo ); }
  if ( mesh->texcoords_vbo ) { glDeleteBuffers( 1, &mesh->texcoords_vbo ); }
  if ( mesh->picking_vbo ) { glDeleteBuffers( 1, &mesh->picking_vbo ); }
  if ( mesh->palidx_vbo ) { glDeleteBuffers( 1, &mesh->palidx_vbo ); }
  if ( mesh->vedges_vbo ) { glDeleteBuffers( 1, &mesh->vedges_vbo ); }
  if ( mesh->points_vbo ) { glDeleteBuffers( 1, &mesh->points_vbo ); }
  if ( mesh->vao ) { glDeleteVertexArrays( 1, &mesh->vao ); }
  memset( mesh, 0, sizeof( gfx_mesh_t ) );
}

void gfx_update_texture_sub_image( gfx_texture_t* texture, const uint8_t* img_buffer, int x_offs, int y_offs, int w, int h ) {
  assert( texture && texture->handle_gl ); // NOTE: it is valid for pixels to be NULL

  GLenum format = GL_RGBA;
  glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
  switch ( texture->n_channels ) {
  case 4: {
    format = GL_RGBA;
  } break;
  case 3: {
    format = texture->properties.is_bgr ? GL_BGR : GL_RGB;
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  case 1: {
    format = GL_RED;
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  default: {
    fprintf( stderr, "WARNING: unhandled texture channel number: %i\n", texture->n_channels );
    gfx_delete_texture( texture );
    return;
    // TODO(Anton) - when have default texture: *texture = g_default_texture
    return;
  } break;
  } // endswitch
  // NOTE can use 32-bit, GL_FLOAT depth component for eg DOF
  if ( texture->properties.is_depth ) { format = GL_DEPTH_COMPONENT; }

  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, texture->handle_gl );
  glTexSubImage2D( GL_TEXTURE_2D, 0, x_offs, y_offs, w, h, format, GL_UNSIGNED_BYTE, img_buffer );
  glBindTexture( GL_TEXTURE_2D, 0 );
}

void gfx_update_texture( gfx_texture_t* texture, const uint8_t* img_buffer, int w, int h, int n_channels ) {
  assert( texture && texture->handle_gl );                         // NOTE: it is valid for pixels to be NULL
  assert( 4 == n_channels || 3 == n_channels || 1 == n_channels ); // 2 not used yet so not impl
  texture->w          = w;
  texture->h          = h;
  texture->n_channels = n_channels;

  GLint internal_format = GL_RGBA;
  GLenum format         = GL_RGBA;
  glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
  switch ( texture->n_channels ) {
  case 4: {
    internal_format = texture->properties.is_srgb ? GL_SRGB_ALPHA : GL_RGBA;
    format          = texture->properties.is_bgr ? GL_BGRA : GL_RGBA;
  } break;
  case 3: {
    internal_format = texture->properties.is_srgb ? GL_SRGB : GL_RGB;
    format          = texture->properties.is_bgr ? GL_BGR : GL_RGB;
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  case 1: {
    internal_format = GL_RED;
    format          = GL_RED;
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  default: {
    fprintf( stderr, "WARNING: unhandled texture channel number: %i\n", texture->n_channels );
    gfx_delete_texture( texture );
    // TODO(Anton) - when have default texture: *texture = g_default_texture
    return;
  } break;
  } // endswitch
  // NOTE can use 32-bit, GL_FLOAT depth component for eg DOF
  if ( texture->properties.is_depth ) {
    internal_format = GL_DEPTH_COMPONENT;
    format          = GL_DEPTH_COMPONENT;
  }

  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, texture->handle_gl );
  glTexImage2D( GL_TEXTURE_2D, 0, internal_format, texture->w, texture->h, 0, format, GL_UNSIGNED_BYTE, img_buffer );
  if ( texture->properties.repeats ) {
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
  } else {
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  }
  if ( texture->properties.bilinear ) {
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  } else {
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  }
  if ( texture->properties.has_mips ) {
    glGenerateMipmap( GL_TEXTURE_2D );
    if ( texture->properties.bilinear ) {
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    } else {
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
    }
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f );
  } else if ( texture->properties.bilinear ) {
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  } else {
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  }

  glBindTexture( GL_TEXTURE_2D, 0 );
}

gfx_texture_t gfx_create_texture_from_mem( const uint8_t* img_buffer, int w, int h, int n_channels, gfx_texture_properties_t properties ) {
  assert( 4 == n_channels || 3 == n_channels || 1 == n_channels ); // 2 not used yet so not impl
  gfx_texture_t texture = ( gfx_texture_t ){ .properties = properties };
  glGenTextures( 1, &texture.handle_gl );
  gfx_update_texture( &texture, img_buffer, w, h, n_channels );
  return texture;
}

void gfx_delete_texture( gfx_texture_t* texture ) {
  assert( texture && texture->handle_gl );
  glDeleteTextures( 1, &texture->handle_gl );
  memset( texture, 0, sizeof( gfx_texture_t ) );
}

void gfx_draw_mesh( gfx_shader_t shader, mat4 P, mat4 V, mat4 M, uint32_t vao, size_t n_vertices, gfx_texture_t* textures, int n_textures ) {
  if ( 0 == vao || 0 == n_vertices ) { return; }

  for ( int i = 0; i < n_textures; i++ ) {
    GLenum tex_type = !textures[i].properties.is_array ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( tex_type, textures[i].handle_gl );
  }

  glUseProgram( shader.program_gl );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_P, 1, GL_FALSE, P.m );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_V, 1, GL_FALSE, V.m );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_M, 1, GL_FALSE, M.m );
  glBindVertexArray( vao );

  glDrawArrays( GL_TRIANGLES, 0, n_vertices );
  _stats.draw_calls++;
  _stats.drawn_verts += n_vertices;

  glBindVertexArray( 0 );
  glUseProgram( 0 );
  for ( int i = 0; i < n_textures; i++ ) {
    GLenum tex_type = !textures[i].properties.is_array ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( tex_type, 0 );
  }
}

void gfx_draw_mesh_instanced( gfx_shader_t shader, mat4 P, mat4 V, mat4 M, uint32_t vao, size_t n_vertices, int n_instances, gfx_texture_t* textures, int n_textures ) {
  if ( 0 == vao || 0 == n_vertices || 0 == n_instances ) { return; }

  for ( int i = 0; i < n_textures; i++ ) {
    GLenum tex_type = !textures[i].properties.is_array ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( tex_type, textures[i].handle_gl );
  }

  glUseProgram( shader.program_gl );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_P, 1, GL_FALSE, P.m );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_V, 1, GL_FALSE, V.m );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_M, 1, GL_FALSE, M.m );
  glBindVertexArray( vao );

  glDrawArraysInstanced( GL_TRIANGLES, 0, n_vertices, n_instances );
  _stats.draw_calls++;
  _stats.drawn_verts += n_vertices;

  glBindVertexArray( 0 );
  glUseProgram( 0 );
  for ( int i = 0; i < n_textures; i++ ) {
    GLenum tex_type = !textures[i].properties.is_array ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( tex_type, 0 );
  }
}

void gfx_draw_textured_quad( gfx_texture_t texture, vec2 scale, vec2 pos ) {
  GLenum tex_type = texture.properties.is_array ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
  glEnable( GL_BLEND );
  glUseProgram( gfx_text_shader.program_gl );
  glProgramUniform2f( gfx_text_shader.program_gl, gfx_text_shader.u_scale, scale.x, scale.y );
  glProgramUniform2f( gfx_text_shader.program_gl, gfx_text_shader.u_pos, pos.x, pos.y );
  glBindVertexArray( _ss_quad_mesh.vao );
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( tex_type, texture.handle_gl );

  glDrawArrays( GL_TRIANGLE_STRIP, 0, _ss_quad_mesh.n_vertices );
  _stats.draw_calls++;
  _stats.drawn_verts += _ss_quad_mesh.n_vertices;

  glBindTexture( tex_type, 0 );
  glBindVertexArray( 0 );
  glUseProgram( 0 );
  glDisable( GL_BLEND );
}

void gfx_wireframe_mode() { glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); }

void gfx_polygon_mode() { glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ); }

void gfx_clear_colour_and_depth_buffers( float r, float g, float b, float a ) {
  glClearColor( r, g, b, a );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void gfx_clear_depth_buffer() { glClear( GL_DEPTH_BUFFER_BIT ); }
void gfx_clear_colour_buffer() { glClear( GL_COLOR_BUFFER_BIT ); }

bool gfx_should_window_close() {
  assert( gfx_window_ptr );
  return glfwWindowShouldClose( gfx_window_ptr );
}

void gfx_viewport( int x, int y, int w, int h ) { glViewport( x, y, w, h ); }

void gfx_swap_buffer() {
  assert( gfx_window_ptr );

  glfwSwapBuffers( gfx_window_ptr );
}
void gfx_poll_events() {
  assert( gfx_window_ptr );

  glfwPollEvents();
}

double gfx_get_time_s() { return glfwGetTime(); }

void gfx_framebuffer_dims( int* width, int* height ) {
  assert( width && height && gfx_window_ptr );
  glfwGetFramebufferSize( gfx_window_ptr, width, height );
}
void gfx_window_dims( int* width, int* height ) {
  assert( width && height && gfx_window_ptr );
  glfwGetWindowSize( gfx_window_ptr, width, height );
}

void gfx_rebuild_framebuffer( gfx_framebuffer_t* fb, int w, int h ) {
  assert( fb );
  if ( w <= 0 || h <= 0 ) { return; } // can happen during alt+tab
  assert( fb->handle_gl > 0 );
  assert( fb->output_texture.handle_gl > 0 );
  assert( fb->depth_texture.handle_gl > 0 );

  fb->built = false; // invalidate
  fb->w     = w;
  fb->h     = h;

  glBindFramebuffer( GL_FRAMEBUFFER, fb->handle_gl );
  {
    gfx_update_texture( &fb->output_texture, NULL, fb->w, fb->h, fb->output_texture.n_channels );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->output_texture.handle_gl, 0 );

    gfx_update_texture( &fb->depth_texture, NULL, fb->w, fb->h, fb->depth_texture.n_channels );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb->depth_texture.handle_gl, 0 );

    GLenum draw_buf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers( 1, &draw_buf );

    {
      GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
      if ( GL_FRAMEBUFFER_COMPLETE != status ) {
        fprintf( stderr, "ERROR: incomplete framebuffer\n" );
        if ( GL_FRAMEBUFFER_UNDEFINED == status ) {
          fprintf( stderr, "GL_FRAMEBUFFER_UNDEFINED\n" );
        } else if ( GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT == status ) {
          fprintf( stderr, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n" );
        } else if ( GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT == status ) {
          fprintf( stderr, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n" );
        } else if ( GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER == status ) {
          fprintf( stderr, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n" );
        } else if ( GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER == status ) {
          fprintf( stderr, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n" );
        } else if ( GL_FRAMEBUFFER_UNSUPPORTED == status ) {
          fprintf( stderr, "GL_FRAMEBUFFER_UNSUPPORTED\n" );
        } else if ( GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE == status ) {
          fprintf( stderr, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\n" );
        } else if ( GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS == status ) {
          fprintf( stderr, "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS\n" );
        } else {
          fprintf( stderr, "glCheckFramebufferStatus unspecified error\n" );
        }
        return;
      }
    } // endblock checks
  }
  glBindFramebuffer( GL_FRAMEBUFFER, 0 );
  fb->built = true; // validate
}

// NOTE(Anton) maybe later need some srgb settings in this function
gfx_framebuffer_t gfx_create_framebuffer( int w, int h ) {
  gfx_framebuffer_t fb;

  glGenFramebuffers( 1, &fb.handle_gl );
  fb.output_texture = ( gfx_texture_t ){ .w = w, .h = h, .n_channels = 4 };
  glGenTextures( 1, &fb.output_texture.handle_gl );
  fb.depth_texture = ( gfx_texture_t ){ .w = w, .h = h, .n_channels = 4, .properties.is_depth = true };
  glGenTextures( 1, &fb.depth_texture.handle_gl );

  gfx_rebuild_framebuffer( &fb, w, h );
  return fb;
}

void gfx_bind_framebuffer( const gfx_framebuffer_t* fb ) {
  if ( !fb ) {
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    return;
  }
  glBindFramebuffer( GL_FRAMEBUFFER, fb->handle_gl );
}

void gfx_read_pixels( int x, int y, int w, int h, int n_channels, uint8_t* data ) {
  GLenum format = GL_RGB;
  if ( 4 == n_channels ) { format = GL_RGBA; }

  glPixelStorei( GL_PACK_ALIGNMENT, 1 );   // for irregular display sizes in RGB
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // affects glReadPixels and subsequent texture calls alignment format

  glReadBuffer( GL_COLOR_ATTACHMENT0 );
  glReadPixels( x, y, w, h, format, GL_UNSIGNED_BYTE, data ); // note can be eg float
}

gfx_stats_t gfx_frame_stats() {
  gfx_stats_t s = _stats;
  memset( &_stats, 0, sizeof( gfx_stats_t ) );
  return s;
}

// TODO(Anton) use a scratch buffer to avoid malloc spike
void gfx_screenshot() {
  int w = 0, h = 0;
  gfx_framebuffer_dims( &w, &h );
  uint8_t* img_ptr = malloc( w * h * 3 );

  glPixelStorei( GL_PACK_ALIGNMENT, 1 );   // for irregular display sizes in RGB
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // affects glReadPixels and subsequent texture calls alignment format

  glReadBuffer( GL_COLOR_ATTACHMENT0 );
  glReadPixels( 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img_ptr ); // note can be eg float

  char filename[1024];
  long int t = time( NULL );
  sprintf( filename, "screens/screenshot_%ld.png", t );

  uint8_t* last_row = img_ptr + ( w * 3 * ( h - 1 ) );
  if ( !stbi_write_png( filename, w, h, 3, last_row, -3 * w ) ) { fprintf( stderr, "WARNING could not save screenshot `%s`\n", filename ); }

  free( img_ptr );
}
