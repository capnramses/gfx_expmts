#include "gfx.h"
#include "glcontext.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h" // NOTE: define implementation in main.c
#include <stdio.h>

#define SHADER_BINDING_VP 0       /** Vertex point attribute binding. */
#define SHADER_BINDING_VT 1       /** Vertex texture-coordinate attribute binding. */
#define SHADER_BINDING_VN 2       /** Vertex normal attribute binding. 4th channel used to store 'heightmap surface' factor of 1 or 0. */
#define SHADER_BINDING_VC 3       /** Vertex colour attribute binding. */
#define SHADER_BINDING_VPAL_IDX 4 /** Vertex palette index attribute binding. Used for array texture indices. */
#define SHADER_BINDING_VPICKING 5 /** Vertex attribute binding. Unique per-face colours to help picking algorithm. */
#define SHADER_BINDING_VEDGE 6    /** Vertex attribute binding - edge factor. */
/* NOTE(Anton) for >1 instanced buffer SHADER_BINDING_INSTANCED+i is currently used for buffer bindings but no shader binding is yet done. */
#define SHADER_BINDING_INSTANCED_A 7 /** Vertex attribute binding. */
#define SHADER_BINDING_INSTANCED_B 8 /** Vertex attribute binding. */

#define GFX_MAX_SHADER_STR 10000 /** Maximum length, in bytes, of a loaded shader string. */
#define GFX_N_SHADERS_MAX 256    /** Maximum number of registered shaders. */

/****************************************************************************
 * Externally-declared global variables.
 ****************************************************************************/

GLFWwindow* gfx_window_ptr; /** Pointer to the application's GLFW window. Externally declared in apg_glcontext.h. */
gfx_shader_t gfx_scaled_vc_shader, gfx_quad_texture_shader, gfx_default_textured_shader;
gfx_mesh_t gfx_ss_quad_mesh;
gfx_texture_t gfx_checkerboard_texture;
gfx_texture_t gfx_white_texture;

/****************************************************************************
 * Static global variables in this file.
 ****************************************************************************/

static GLFWmonitor* gfx_monitor_ptr;
static gfx_shader_t* _registered_shaders_ptrs[GFX_N_SHADERS_MAX];
static int _n_shaders;
static int g_win_width = 1024, g_win_height = 768; // to fit on laptop screen in a window

/****************************************************************************
 * SHADERS
 ****************************************************************************/

static bool _recompile_shader_with_check( GLuint shader, const char* src_str ) {
  if ( !src_str || 0 == shader ) { return false; }

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
  if ( !vert_shader_str || !frag_shader_str ) { return shader; }

  GLuint vs         = glCreateShader( GL_VERTEX_SHADER );
  GLuint fs         = glCreateShader( GL_FRAGMENT_SHADER );
  bool res_a        = _recompile_shader_with_check( vs, vert_shader_str );
  bool res_b        = _recompile_shader_with_check( fs, frag_shader_str );
  shader.program_gl = glCreateProgram();
  glAttachShader( shader.program_gl, fs );
  glAttachShader( shader.program_gl, vs );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VP, "a_vp" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VT, "a_vt" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VN, "a_vn" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VC, "a_vc" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VPAL_IDX, "a_vpal_idx" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VPICKING, "a_vpicking" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VEDGE, "a_vedge" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_INSTANCED_A, "a_instanced" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_INSTANCED_A, "a_instanced_pos" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_INSTANCED_B, "a_instanced_rgb" );
  glLinkProgram( shader.program_gl );
  glDeleteShader( vs );
  glDeleteShader( fs );

  shader.u_M                     = glGetUniformLocation( shader.program_gl, "u_M" );
  shader.u_V                     = glGetUniformLocation( shader.program_gl, "u_V" );
  shader.u_P                     = glGetUniformLocation( shader.program_gl, "u_P" );
  shader.u_cam_pos               = glGetUniformLocation( shader.program_gl, "u_cam_pos" );
  shader.u_fwd                   = glGetUniformLocation( shader.program_gl, "u_fwd" );
  shader.u_rgb                   = glGetUniformLocation( shader.program_gl, "u_rgb" );
  shader.u_output_tint_rgb       = glGetUniformLocation( shader.program_gl, "u_output_tint_rgb" );
  shader.u_underwater_wobble_fac = glGetUniformLocation( shader.program_gl, "u_underwater_wobble_fac" );
  shader.u_scale                 = glGetUniformLocation( shader.program_gl, "u_scale" );
  shader.u_pos                   = glGetUniformLocation( shader.program_gl, "u_pos" );
  shader.u_offset                = glGetUniformLocation( shader.program_gl, "u_offset" );
  shader.u_alpha                 = glGetUniformLocation( shader.program_gl, "u_alpha" );
  shader.u_texture_a             = glGetUniformLocation( shader.program_gl, "u_texture_a" );
  shader.u_texture_b             = glGetUniformLocation( shader.program_gl, "u_texture_b" );
  shader.u_texture_c             = glGetUniformLocation( shader.program_gl, "u_texture_c" );
  shader.u_scroll                = glGetUniformLocation( shader.program_gl, "u_scroll" );
  shader.u_progress_factor       = glGetUniformLocation( shader.program_gl, "u_progress_factor" );
  shader.u_time                  = glGetUniformLocation( shader.program_gl, "u_time" );
  shader.u_tint                  = glGetUniformLocation( shader.program_gl, "u_tint" );
  shader.u_texcoord_scale        = glGetUniformLocation( shader.program_gl, "u_texcoord_scale" );
  shader.u_walk_time             = glGetUniformLocation( shader.program_gl, "u_walk_time" );
  shader.u_cam_y_rot             = glGetUniformLocation( shader.program_gl, "u_cam_y_rot" );
  shader.u_chunk_id              = glGetUniformLocation( shader.program_gl, "u_chunk_id" );
  shader.u_fog_factor            = glGetUniformLocation( shader.program_gl, "u_fog_factor" );
  shader.u_upper_clip_bound      = glGetUniformLocation( shader.program_gl, "u_upper_clip_bound" );
  shader.u_lower_clip_bound      = glGetUniformLocation( shader.program_gl, "u_lower_clip_bound" );

  if ( shader.u_texture_a > -1 ) { glProgramUniform1i( shader.program_gl, shader.u_texture_a, 0 ); }
  if ( shader.u_texture_b > -1 ) { glProgramUniform1i( shader.program_gl, shader.u_texture_b, 1 ); }
  if ( shader.u_texture_c > -1 ) { glProgramUniform1i( shader.program_gl, shader.u_texture_c, 1 ); }
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
  if ( !res_a || !res_b || !linked ) { return gfx_scaled_vc_shader; }
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
  if ( !shader.program_gl ) { fprintf( stderr, "ERROR: Failed to compile shader from files `%s` and `%s`\n", vert_shader_filename, frag_shader_filename ); }
  return shader;
}

void gfx_delete_shader_program( gfx_shader_t* shader_ptr ) {
  assert( shader_ptr );
  if ( !shader_ptr ) { return; }

  if ( shader_ptr->program_gl ) { glDeleteProgram( shader_ptr->program_gl ); }
  memset( shader_ptr, 0, sizeof( gfx_shader_t ) );
}

void gfx_shader_register( gfx_shader_t* shader_ptr ) {
  assert( shader_ptr );
  if ( !shader_ptr ) { return; }

  if ( _n_shaders >= GFX_N_SHADERS_MAX ) {
    fprintf( stderr, "ERROR: Can't register shader. Too many shaders.\n" );
    return;
  }
  _registered_shaders_ptrs[_n_shaders++] = shader_ptr;
}

void gfx_shaders_reload() {
  for ( int i = 0; i < _n_shaders; i++ ) {
    if ( !_registered_shaders_ptrs[i] ) { continue; }
    char vs_filename[GFX_SHADER_PATH_MAX], fs_filename[GFX_SHADER_PATH_MAX];
    strncpy( vs_filename, _registered_shaders_ptrs[i]->vs_filename, GFX_SHADER_PATH_MAX - 1 );
    strncpy( fs_filename, _registered_shaders_ptrs[i]->fs_filename, GFX_SHADER_PATH_MAX - 1 );
    _registered_shaders_ptrs[i]->vs_filename[GFX_SHADER_PATH_MAX - 1] = _registered_shaders_ptrs[i]->fs_filename[GFX_SHADER_PATH_MAX - 1] = '\0';

    gfx_delete_shader_program( _registered_shaders_ptrs[i] );
    *_registered_shaders_ptrs[i] = gfx_create_shader_program_from_files( vs_filename, fs_filename );
  }
}

int gfx_uniform_loc( const gfx_shader_t* shader_ptr, const char* name ) {
  assert( shader_ptr && shader_ptr->program_gl );
  if ( !shader_ptr ) { return -1; }

  int l = glGetUniformLocation( shader_ptr->program_gl, name );
  return l;
}

void gfx_uniform1i( const gfx_shader_t* shader_ptr, int loc, int i ) {
  if ( !shader_ptr || 0 == shader_ptr->program_gl ) {
    fprintf( stderr, "ERROR: shader program is 0\n" );
    return;
  }
  if ( loc < 0 ) {
    fprintf( stderr, "ERROR: gfx_uniform4f() - loc is %i\n", loc );
    return;
  }

  glProgramUniform1i( shader_ptr->program_gl, loc, i );
}

void gfx_uniform1f( const gfx_shader_t* shader_ptr, int loc, float f ) {
  if ( !shader_ptr || 0 == shader_ptr->program_gl ) {
    fprintf( stderr, "ERROR: shader program is 0\n" );
    return;
  }
  if ( loc < 0 ) {
    fprintf( stderr, "ERROR: gfx_uniform4f() - loc is %i\n", loc );
    return;
  }

  glProgramUniform1f( shader_ptr->program_gl, loc, f );
}
void gfx_uniform2f( const gfx_shader_t* shader_ptr, int loc, float x, float y ) {
  if ( !shader_ptr || 0 == shader_ptr->program_gl ) {
    fprintf( stderr, "ERROR: shader program is 0\n" );
    return;
  }
  if ( loc < 0 ) {
    fprintf( stderr, "ERROR: gfx_uniform4f() - loc is %i\n", loc );
    return;
  }

  glProgramUniform2f( shader_ptr->program_gl, loc, x, y );
}
void gfx_uniform3f( const gfx_shader_t* shader_ptr, int loc, float x, float y, float z ) {
  if ( !shader_ptr || 0 == shader_ptr->program_gl ) {
    fprintf( stderr, "ERROR: shader program is 0\n" );
    return;
  }
  if ( loc < 0 ) {
    fprintf( stderr, "ERROR: gfx_uniform4f() - loc is %i\n", loc );
    return;
  }

  glProgramUniform3f( shader_ptr->program_gl, loc, x, y, z );
}
void gfx_uniform4f( const gfx_shader_t* shader_ptr, int loc, float x, float y, float z, float w ) {
  if ( !shader_ptr || 0 == shader_ptr->program_gl ) {
    fprintf( stderr, "ERROR: shader program is 0\n" );
    return;
  }
  if ( loc < 0 ) {
    fprintf( stderr, "ERROR: gfx_uniform4f() - loc is %i\n", loc );
    return;
  }

  glProgramUniform4f( shader_ptr->program_gl, loc, x, y, z, w );
}

void gfx_fullscreen( bool fullscreen ) {
  if ( !gfx_window_ptr ) { return; }
  if ( !gfx_monitor_ptr ) { gfx_monitor_ptr = glfwGetPrimaryMonitor(); }
  const GLFWvidmode* mode = glfwGetVideoMode( gfx_monitor_ptr );
  if ( fullscreen ) {
    glfwSetWindowMonitor( gfx_window_ptr, gfx_monitor_ptr, 0, 0, mode->width, mode->height, mode->refreshRate );
    printf( "monitor refresh rate is %i\n", mode->refreshRate );
    glfwSwapInterval( 1 ); // NOTE(Anton) i call vysnc here because switching to fullscreen can turn it off. could add a param for vsync to preserve user
                           // preference.
  } else {
    glfwSetWindowMonitor( gfx_window_ptr, NULL, 0, 0, 800, 600, GLFW_DONT_CARE ); // The refresh rate is ignored when no monitor is specified.
    glfwSetWindowPos( gfx_window_ptr, 0, 32 ); // make sure toolbar is not off-screen (Windows!). doesn't seem to work in params to above func.
  }
}

bool gfx_start( const char* window_title, const char* window_icon_filename, bool fullscreen ) {
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

    if ( window_icon_filename ) {
      int width = 0, height = 0, comps = 0, req_comps = 0;
      unsigned char* pixels = stbi_load( window_icon_filename, &width, &height, &comps, req_comps );
      if ( !pixels ) {
        fprintf( stderr, "ERROR: loading image from file `%s`\n", window_icon_filename );
      } else {
        GLFWimage icon_image = { .width = width, .height = height, .pixels = pixels };
        glfwSetWindowIcon( gfx_window_ptr, 1, &icon_image );
        stbi_image_free( pixels );
      }
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
  // TODO(Anton) remove 0.1 scale here and put in matrix everywhere its used and then renamed this to default shader
  { // gfx_scaled_vc_shader. IMPORTANT!!!! NOTE THE SCALE of x0.1 here!
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
    gfx_scaled_vc_shader = gfx_create_shader_program_from_strings( vertex_shader, fragment_shader );
    if ( !gfx_scaled_vc_shader.loaded ) { return false; }
  }
  { // gfx_default_textured_shader
    const char* vertex_shader =
      "#version 410 core\n"
      "in vec3 a_vp;\n"
      "in vec3 a_vt;\n"
      "uniform mat4 u_P, u_V, u_M;\n"
      "out vec2 v_st;\n"
      "void main () {\n"
      "  v_st = a_vp.xy * 0.5 + 0.5;\n"
      "  v_st.t = 1.0 - v_st.t;\n"
      "  gl_Position = u_P * u_V * u_M * vec4( a_vp, 1.0 );\n"
      "}\n";
    const char* fragment_shader =
      "#version 410 core\n"
      "in vec2 v_st;\n"
      "uniform sampler2D u_tex;\n"
      "uniform vec4 u_tint;\n"
      "out vec4 o_frag_colour;\n"
      "void main () {\n"
      "  vec4 texel = texture( u_tex, v_st );\n"
      "  texel.rgb = pow( texel.rgb, vec3( 2.2 ) );\n"
      "  o_frag_colour = texel * u_tint;\n"
      "  o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );\n"
      "}\n";
    gfx_default_textured_shader = gfx_create_shader_program_from_strings( vertex_shader, fragment_shader );
    if ( !gfx_default_textured_shader.loaded ) { return false; }
  }
  {
    const char* vertex_shader =
      "#version 410 core\n"
      "in vec2 a_vp;\n"
      "uniform vec2 u_scale, u_pos, u_texcoord_scale;\n"
      "out vec2 v_st;\n"
      "void main () {\n"
      "  v_st = a_vp.xy * 0.5 + 0.5;\n"
      "  v_st.t = 1.0 - v_st.t;\n"
      "  v_st *= u_texcoord_scale;\n"
      "  gl_Position = vec4( a_vp * u_scale + u_pos, 0.0, 1.0 );\n"
      "}\n";
    const char* fragment_shader =
      "#version 410 core\n"
      "in vec2 v_st;\n"
      "uniform sampler2D u_tex;\n"
      "uniform vec4 u_tint;\n"
      "out vec4 o_frag_colour;\n"
      "void main () {\n"
      "  vec4 texel = texture( u_tex, v_st );\n"
      "  texel.rgb = pow( texel.rgb, vec3( 2.2 ) );\n"
      "  o_frag_colour = texel * u_tint;\n"
      "  o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );\n"
      "}\n";
    gfx_quad_texture_shader = gfx_create_shader_program_from_strings( vertex_shader, fragment_shader );
    if ( !gfx_quad_texture_shader.loaded ) { return false; }
  }
//  _init_ss_quad();
//  _init_default_textures();

  glClearColor( 0.5, 0.5, 0.5, 1.0 );
  glDepthFunc( GL_LESS );
  glEnable( GL_DEPTH_TEST );
  glCullFace( GL_BACK );
  glFrontFace( GL_CCW );
  glEnable( GL_CULL_FACE );
  // TODO(Anton) for release versions gfx_fullscreen( true );
  glfwSwapInterval( 1 ); // vsync

  return true;
}

void gfx_vsync( int interval ) { glfwSwapInterval( interval ); }

void gfx_stop() {
  gfx_delete_shader_program( &gfx_default_textured_shader );
  gfx_delete_shader_program( &gfx_scaled_vc_shader );
  gfx_delete_shader_program( &gfx_quad_texture_shader );
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

void gfx_clip_planes_enable( bool enable ) {
  if ( enable ) {
    glEnable( GL_CLIP_DISTANCE0 );
    glEnable( GL_CLIP_DISTANCE1 );
  } else {
    glDisable( GL_CLIP_DISTANCE0 );
    glDisable( GL_CLIP_DISTANCE1 );
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
