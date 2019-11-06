#include "gl_utils.h"
#include "apg_maths.h"
#include <GL/glew.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <assert.h>
#include <string.h>

#define SHADER_BINDING_VP 0
#define SHADER_BINDING_VC 1

typedef struct shader_t {
  uint32_t program;
  int u_P, u_V, u_M;
} shader_t;

static int g_win_width = 1920, g_win_height = 1080;
static GLFWwindow* g_window;
static shader_t _default_shader, _text_shader;
static mesh_t _ss_quad_mesh;

static void _init_ss_quad() {
  float ss_quad_pos[] = { -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0, -1.0 };
  _ss_quad_mesh       = create_mesh_from_mem( ss_quad_pos, 2, NULL, 0, 4 );
}

shader_t default_shaders() {
  const char* vertex_shader =
    "#version 410\n"
    "in vec3 a_vp;\n"
    "in vec3 a_vc;\n"
    "uniform mat4 u_P, u_V, u_M;\n"
    "out vec3 vc;\n"
    "void main () {\n"
    "  vc = a_vc;\n"
    "  gl_Position = u_P * u_V * u_M * vec4( a_vp * 0.1, 1.0 );\n"
    "}\n";
  const char* fragment_shader =
    "#version 410\n"
    "in vec3 vc;\n"
    "out vec4 o_frag_colour;\n"
    "void main () {\n"
    "  o_frag_colour = vec4( vc, 1.0 );\n"
    "  o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );\n"
    "}\n";
  GLuint vs = glCreateShader( GL_VERTEX_SHADER );
  glShaderSource( vs, 1, &vertex_shader, NULL );
  glCompileShader( vs );
  GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
  glShaderSource( fs, 1, &fragment_shader, NULL );
  glCompileShader( fs );
  GLuint program = glCreateProgram();
  glAttachShader( program, fs );
  glAttachShader( program, vs );
  glBindAttribLocation( program, SHADER_BINDING_VP, "a_vp" );
  glBindAttribLocation( program, SHADER_BINDING_VC, "a_vc" );
  glLinkProgram( program );
  glDeleteShader( vs );
  glDeleteShader( fs );
  int u_M = glGetUniformLocation( program, "u_M" );
  assert( u_M >= 0 );
  int u_V = glGetUniformLocation( program, "u_V" );
  assert( u_V >= 0 );
  int u_P = glGetUniformLocation( program, "u_P" );
  assert( u_P >= 0 );
  return ( shader_t ){ .program = program, .u_P = u_P, .u_V = u_V, .u_M = u_M };
}

static shader_t _text_shaders() {
  const char* vertex_shader =
    "#version 410\n"
    "in vec2 a_vp;\n"
    "out vec2 v_st;\n"
    "void main () {\n"
    "  v_st = a_vp.xy * 0.5 + 0.5;\n"
		"  v_st.t = 1.0 - v_st.t;\n"
    "  gl_Position = vec4( a_vp * 0.1 + vec2( -0.9, 0.9 ), 0.0, 1.0 );\n"
    "}\n";
  const char* fragment_shader =
    "#version 410\n"
    "in vec2 v_st;\n"
    "uniform sampler2D u_tex;\n"
    "out vec4 o_frag_colour;\n"
    "void main () {\n"
    "  vec4 texel = texture( u_tex, v_st );\n"
    "  o_frag_colour = texel;\n"
    "  o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );\n"
    "}\n";
  GLuint vs = glCreateShader( GL_VERTEX_SHADER );
  glShaderSource( vs, 1, &vertex_shader, NULL );
  glCompileShader( vs );
  GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
  glShaderSource( fs, 1, &fragment_shader, NULL );
  glCompileShader( fs );
  GLuint program = glCreateProgram();
  glAttachShader( program, fs );
  glAttachShader( program, vs );
  glBindAttribLocation( program, SHADER_BINDING_VP, "a_vp" );
  glBindAttribLocation( program, SHADER_BINDING_VC, "a_vc" );
  glLinkProgram( program );
  glDeleteShader( vs );
  glDeleteShader( fs );
  return ( shader_t ){ .program = program };
}

bool start_gl() {
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
      int max_samples = 1;
      glGetIntegerv( GL_MAX_COLOR_TEXTURE_SAMPLES, &max_samples ); // was 32 on my 1080ti
      int nsamples = 32 < max_samples ? 32 : max_samples;
      glfwWindowHint( GLFW_SAMPLES, nsamples );
    }

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

  _default_shader = default_shaders();
  _text_shader    = _text_shaders();
  _init_ss_quad();

  glClearColor( 0.5, 0.5, 0.9, 1.0 );
  glDepthFunc( GL_LESS );
  glEnable( GL_DEPTH_TEST );
  glCullFace( GL_BACK );
  glFrontFace( GL_CCW );
  glEnable( GL_CULL_FACE );
  // glEnable( GL_CLIP_DISTANCE0 ); // clip plane A
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glDisable( GL_BLEND );

	glfwSwapInterval( 1 );

  return true;
}
void stop_gl() { glfwTerminate(); }

mesh_t create_mesh_from_mem( const float* points_buffer, int n_points_comps, const float* colours_buffer, int n_colours_comps, int n_vertices ) {
  assert( points_buffer && n_points_comps > 0 && n_vertices > 0 );

  GLuint vertex_array_gl;
  GLuint points_buffer_gl = 0, colour_buffer_gl = 0;
  glGenVertexArrays( 1, &vertex_array_gl );
  glBindVertexArray( vertex_array_gl );
  {
    glGenBuffers( 1, &points_buffer_gl );
    glBindBuffer( GL_ARRAY_BUFFER, points_buffer_gl );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_points_comps * n_vertices, points_buffer, GL_STATIC_DRAW );
    glEnableVertexAttribArray( SHADER_BINDING_VP );
    glVertexAttribPointer( SHADER_BINDING_VP, n_points_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( colours_buffer && n_colours_comps > 0 ) {
    glGenBuffers( 1, &colour_buffer_gl );
    glBindBuffer( GL_ARRAY_BUFFER, colour_buffer_gl );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_colours_comps * n_vertices, colours_buffer, GL_STATIC_DRAW );
    glEnableVertexAttribArray( SHADER_BINDING_VC );
    glVertexAttribPointer( SHADER_BINDING_VC, n_colours_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  glBindVertexArray( 0 );

  mesh_t mesh = ( mesh_t ){ .vao = vertex_array_gl, .points_vbo = points_buffer_gl, .colours_vbo = colour_buffer_gl, .n_vertices = n_vertices };
  return mesh;
}

void delete_mesh( mesh_t* mesh ) {
  assert( mesh && mesh->vao > 0 && mesh->points_vbo > 0 );

  if ( mesh->colours_vbo ) { glDeleteBuffers( 1, &mesh->colours_vbo ); }
  glDeleteBuffers( 1, &mesh->points_vbo );
  glDeleteVertexArrays( 1, &mesh->vao );
  memset( mesh, 0, sizeof( mesh_t ) );
}

texture_t create_texture_from_mem( const uint8_t* img_buffer, int w, int h, int n_channels, bool srgb, bool is_depth ) {
  texture_t texture     = ( texture_t ){ .w = w, .h = h, .n_channels = n_channels, .srgb = srgb, .is_depth = is_depth };
  GLint internal_format = GL_RGBA;
  GLenum format         = GL_RGBA;

  assert( img_buffer );

  glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
  switch ( texture.n_channels ) {
  case 4: {
    internal_format = texture.srgb ? GL_SRGB_ALPHA : GL_RGBA;
    format          = GL_RGBA;
  } break;
  case 3: {
    internal_format = texture.srgb ? GL_SRGB : GL_RGB;
    format          = GL_RGB;
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  case 1: {
    internal_format = GL_RED;
    format          = GL_RED;
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  default: {
    fprintf( stderr, "WARNING: unhandled texture channel number: %i\n", texture.n_channels );
    delete_texture( &texture );
    return texture;
  } break;
  } // endswitch
  // NOTE can use 32-bit, GL_FLOAT depth component for eg DOF
  if ( texture.is_depth ) {
    internal_format = GL_DEPTH_COMPONENT;
    format          = GL_DEPTH_COMPONENT;
  }

  glGenTextures( 1, &texture.handle_gl );
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, texture.handle_gl );
  glTexImage2D( GL_TEXTURE_2D, 0, internal_format, texture.w, texture.h, 0, format, GL_UNSIGNED_BYTE, img_buffer );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glBindTexture( GL_TEXTURE_2D, 0 );

  return texture;
}

void delete_texture( texture_t* texture ) {
  assert( texture && texture->handle_gl );
  glDeleteTextures( 1, &texture->handle_gl );
  memset( texture, 0, sizeof( texture_t ) );
}

void draw_mesh( uint32_t vao, size_t n_vertices ) {
  static double prev_time;
  double curr_time    = glfwGetTime();
  double elapsed_time = curr_time - prev_time;
  prev_time           = curr_time;

  int width, height;
  glfwGetFramebufferSize( g_window, &width, &height );
  float aspect = (float)width / (float)height;

  mat4 P  = perspective( 67.0f, aspect, 0.1f, 1000.0f );
  vec3 up = normalise_vec3( ( vec3 ){ .x = 0.5f, .y = 0.5f, .z = -0.5f } );
  mat4 V  = look_at( ( vec3 ){ .x = -5.0f, .y = 5.0f, .z = 5.0f }, ( vec3 ){ .x = 0, .y = 0, .z = 0 }, up );

  static double r = 0;
  r += elapsed_time * 50.0;
  mat4 M = rot_y_deg_mat4( r );

  glViewport( 0, 0, width, height );

  glUseProgram( _default_shader.program );
  glProgramUniformMatrix4fv( _default_shader.program, _default_shader.u_P, 1, GL_FALSE, P.m );
  glProgramUniformMatrix4fv( _default_shader.program, _default_shader.u_V, 1, GL_FALSE, V.m );
  glProgramUniformMatrix4fv( _default_shader.program, _default_shader.u_M, 1, GL_FALSE, M.m );
  glBindVertexArray( vao );

  glDrawArrays( GL_TRIANGLES, 0, n_vertices );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
}

void draw_textured_quad( texture_t texture ) {
  glEnable( GL_BLEND );
  glUseProgram( _text_shader.program );
  glBindVertexArray( _ss_quad_mesh.vao );
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, texture.handle_gl );

  glDrawArrays( GL_TRIANGLE_STRIP, 0, _ss_quad_mesh.n_vertices );

  glBindTexture( GL_TEXTURE_2D, 0 );
  glBindVertexArray( 0 );
  glUseProgram( 0 );
  glDisable( GL_BLEND );
}

void wireframe_mode() { glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); }

void polygon_mode() { glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ); }

void clear_colour_and_depth_buffers() {
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

bool should_window_close() {
  assert( g_window );
  return glfwWindowShouldClose( g_window );
}

void swap_buffer_and_poll_events() {
  assert( g_window );

  glfwPollEvents();
  glfwSwapBuffers( g_window );
}

double get_time_s() { return glfwGetTime(); }