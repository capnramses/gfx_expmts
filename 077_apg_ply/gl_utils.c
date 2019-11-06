#include "gl_utils.h"
#include "apg_maths.h"
#include "glcontext.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../common/include/stb/stb_image_write.h" // for screenshots
#include <assert.h>
#include <string.h>
#include <time.h>

#define SHADER_BINDING_VP 0
#define SHADER_BINDING_VN 1 /* 4th channel used to store 'heightmap surface' factor of 1 or 0 */
#define SHADER_BINDING_VC 2
#define SHADER_BINDING_VPAL_IDX 2
#define SHADER_BINDING_VPICKING 3 /* special colours to help picking algorithm */

static int g_win_width = 1920, g_win_height = 1080;
GLFWwindow* g_window;
shader_t g_default_shader, g_text_shader;
static mesh_t _ss_quad_mesh;

static void _init_ss_quad() {
  float ss_quad_pos[] = { -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0, -1.0 };
  _ss_quad_mesh       = create_mesh_from_mem( ss_quad_pos, 2, NULL, 0, NULL, 0, 4 );
}

static bool _recompile_shader_with_check( GLuint shader, const char* src_str ) {
  glShaderSource( shader, 1, &src_str, NULL );
  glCompileShader( shader );
  int params = -1;
  glGetShaderiv( shader, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    fprintf( stderr, "ERROR: shader index %u did not compile\nsrc:\n%s", shader, src_str );
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

shader_t create_shader_program_from_strings( const char* vert_shader_str, const char* frag_shader_str ) {
  assert( vert_shader_str && frag_shader_str );
  shader_t shader   = ( shader_t ){ .program_gl = 0 };
  GLuint vs         = glCreateShader( GL_VERTEX_SHADER );
  GLuint fs         = glCreateShader( GL_FRAGMENT_SHADER );
  bool res_a        = _recompile_shader_with_check( vs, vert_shader_str );
  bool res_b        = _recompile_shader_with_check( fs, frag_shader_str );
  shader.program_gl = glCreateProgram();
  glAttachShader( shader.program_gl, fs );
  glAttachShader( shader.program_gl, vs );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VP, "a_vp" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VN, "a_vn" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VC, "a_vc" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VPAL_IDX, "a_vpal_idx" );
  glBindAttribLocation( shader.program_gl, SHADER_BINDING_VPICKING, "a_vpicking" );
  glLinkProgram( shader.program_gl );
  glDeleteShader( vs );
  glDeleteShader( fs );
  shader.u_M        = glGetUniformLocation( shader.program_gl, "u_M" );
  shader.u_V        = glGetUniformLocation( shader.program_gl, "u_V" );
  shader.u_P        = glGetUniformLocation( shader.program_gl, "u_P" );
  shader.u_fwd      = glGetUniformLocation( shader.program_gl, "u_fwd" );
  shader.u_scale    = glGetUniformLocation( shader.program_gl, "u_scale" );
  shader.u_pos      = glGetUniformLocation( shader.program_gl, "u_pos" );
  shader.u_alpha    = glGetUniformLocation( shader.program_gl, "u_alpha" );
  shader.u_chunk_id = glGetUniformLocation( shader.program_gl, "u_chunk_id" );

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
    delete_shader_program( &shader );
    linked = false;
  }
  // fall back to default shader
  if ( !res_a || !res_b || !linked ) { return g_default_shader; }
  shader.loaded = true;
  return shader;
}

void delete_shader_program( shader_t* shader ) {
  assert( shader );
  if ( shader->program_gl ) { glDeleteProgram( shader->program_gl ); }
  memset( shader, 0, sizeof( shader_t ) );
}
int uniform_loc( shader_t shader, const char* name ) {
  assert( shader.program_gl );
  int l = glGetUniformLocation( shader.program_gl, name );
  return l;
}

void uniform1f( shader_t shader, int loc, float f ) {
  assert( shader.program_gl );
  glProgramUniform1f( shader.program_gl, loc, f );
}
void uniform2f( shader_t shader, int loc, float x, float y ) {
  assert( shader.program_gl );
  glProgramUniform2f( shader.program_gl, loc, x, y );
}
void uniform3f( shader_t shader, int loc, float x, float y, float z ) {
  assert( shader.program_gl );
  glProgramUniform3f( shader.program_gl, loc, x, y, z );
}
void uniform4f( shader_t shader, int loc, float x, float y, float z, float w ) {
  assert( shader.program_gl );
  glProgramUniform4f( shader.program_gl, loc, x, y, z, w );
}

bool start_gl( const char* window_title ) {
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
      int nsamples = 32;
      glfwWindowHint( GLFW_SAMPLES, 32 );
      printf( "MSAA = x%i\n", nsamples );
    }

    g_window = glfwCreateWindow( g_win_width, g_win_height, window_title, NULL, NULL );
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
  {
    // IMPORTANT!!!! NOTE THE SCALE of x0.1 here!
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
      "uniform float u_alpha;\n"
      "out vec4 o_frag_colour;\n"
      "void main () {\n"
      "  o_frag_colour = vec4( vc, u_alpha );\n"
      "  o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );\n"
      "}\n";
    g_default_shader = create_shader_program_from_strings( vertex_shader, fragment_shader );
    if ( !g_default_shader.loaded ) { return false; }
  }
  {
    const char* vertex_shader =
      "#version 410\n"
      "in vec2 a_vp;\n"
      "uniform vec2 u_scale, u_pos;\n"
      "out vec2 v_st;\n"
      "void main () {\n"
      "  v_st = a_vp.xy * 0.5 + 0.5;\n"
      "  v_st.t = 1.0 - v_st.t;\n"
      "  gl_Position = vec4( a_vp * u_scale + u_pos, 0.0, 1.0 );\n"
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
    g_text_shader = create_shader_program_from_strings( vertex_shader, fragment_shader );
    if ( !g_text_shader.loaded ) { return false; }
  }
  _init_ss_quad();

  glClearColor( 0.5, 0.5, 0.5, 1.0 );
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
void stop_gl() {
  delete_shader_program( &g_default_shader );
  delete_shader_program( &g_text_shader );
  glfwTerminate();
}

mesh_t create_mesh_from_mem( const float* points_buffer, int n_points_comps, const float* normals_buffer, int n_normals_comps, const float* colours_buffer,
  int n_colours_comps, int n_vertices ) {
  assert( points_buffer && n_points_comps > 0 && n_vertices > 0 );

  GLuint vertex_array_gl;
  GLuint points_buffer_gl = 0, colour_buffer_gl = 0, picking_buffer_gl = 0, normals_buffer_gl = 0;
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
  if ( normals_buffer && n_normals_comps > 0 ) {
    glGenBuffers( 1, &normals_buffer_gl );
    glBindBuffer( GL_ARRAY_BUFFER, normals_buffer_gl );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_normals_comps * n_vertices, normals_buffer, GL_STATIC_DRAW );
    glEnableVertexAttribArray( SHADER_BINDING_VN );
    glVertexAttribPointer( SHADER_BINDING_VN, n_normals_comps, GL_FLOAT, GL_FALSE, 0, NULL );
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

  mesh_t mesh = ( mesh_t ){
    .vao = vertex_array_gl, .points_vbo = points_buffer_gl, .colours_vbo = colour_buffer_gl, .picking_vbo = picking_buffer_gl, .normals_vbo = normals_buffer_gl, .n_vertices = n_vertices
  };
  return mesh;
}

void delete_mesh( mesh_t* mesh ) {
  assert( mesh && mesh->vao > 0 && mesh->points_vbo > 0 );

  if ( mesh->colours_vbo ) { glDeleteBuffers( 1, &mesh->colours_vbo ); }
  glDeleteBuffers( 1, &mesh->points_vbo );
  glDeleteVertexArrays( 1, &mesh->vao );
  memset( mesh, 0, sizeof( mesh_t ) );
}

void update_texture( texture_t* texture, const unsigned char* pixels, bool bgr ) {
  assert( texture && texture->handle_gl );                                                    // NOTE: it is valid for pixels to be NULL
  assert( 4 == texture->n_channels || 3 == texture->n_channels || 1 == texture->n_channels ); // 2 not used yet so not impl

  GLint internal_format = GL_RGBA;
  GLenum format         = GL_RGBA;
  glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
  switch ( texture->n_channels ) {
  case 4: {
    internal_format = texture->srgb ? GL_SRGB_ALPHA : GL_RGBA;
    format          = GL_RGBA;
  } break;
  case 3: {
    internal_format = texture->srgb ? GL_SRGB : GL_RGB;
    format          = bgr ? GL_BGR : GL_RGB;
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  case 1: {
    internal_format = GL_RED;
    format          = GL_RED;
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for small 1-channel npot images and framebuffer reading
  } break;
  default: {
    fprintf( stderr, "WARNING: unhandled texture channel number: %i\n", texture->n_channels );
    delete_texture( texture );
    // TODO(Anton) - when have default texture: *texture = g_default_texture
    return;
  } break;
  } // endswitch
  // NOTE can use 32-bit, GL_FLOAT depth component for eg DOF
  if ( texture->is_depth ) {
    internal_format = GL_DEPTH_COMPONENT;
    format          = GL_DEPTH_COMPONENT;
  }

  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, texture->handle_gl );
  glTexImage2D( GL_TEXTURE_2D, 0, internal_format, texture->w, texture->h, 0, format, GL_UNSIGNED_BYTE, pixels );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glBindTexture( GL_TEXTURE_2D, 0 );
}

texture_t create_texture_from_mem( const uint8_t* img_buffer, int w, int h, int n_channels, bool srgb, bool is_depth, bool bgr ) {
  assert( 4 == n_channels || 3 == n_channels || 1 == n_channels ); // 2 not used yet so not impl
  texture_t texture = ( texture_t ){ .w = w, .h = h, .n_channels = n_channels, .srgb = srgb, .is_depth = is_depth };
  glGenTextures( 1, &texture.handle_gl );
  update_texture( &texture, img_buffer, bgr );
  return texture;
}

void delete_texture( texture_t* texture ) {
  assert( texture && texture->handle_gl );
  glDeleteTextures( 1, &texture->handle_gl );
  memset( texture, 0, sizeof( texture_t ) );
}

void draw_mesh( shader_t shader, mat4 P, mat4 V, mat4 M, uint32_t vao, size_t n_vertices, texture_t* textures, int n_textures ) {
  for ( int i = 0; i < n_textures; i++ ) {
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( GL_TEXTURE_2D, textures[i].handle_gl );
  }

  glUseProgram( shader.program_gl );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_P, 1, GL_FALSE, P.m );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_V, 1, GL_FALSE, V.m );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_M, 1, GL_FALSE, M.m );
  glBindVertexArray( vao );

  glDrawArrays( GL_TRIANGLES, 0, n_vertices );

  glBindVertexArray( 0 );
  glUseProgram( 0 );
  for ( int i = 0; i < n_textures; i++ ) {
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( GL_TEXTURE_2D, 0 );
  }
}

void draw_textured_quad( texture_t texture, vec2 scale, vec2 pos ) {
  glEnable( GL_BLEND );
  glUseProgram( g_text_shader.program_gl );
  glProgramUniform2f( g_text_shader.program_gl, g_text_shader.u_scale, scale.x, scale.y );
  glProgramUniform2f( g_text_shader.program_gl, g_text_shader.u_pos, pos.x, pos.y );
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

void clear_colour_and_depth_buffers( float r, float g, float b, float a ) {
  glClearColor( r, g, b, a );
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

void clear_depth_buffer() { glClear( GL_DEPTH_BUFFER_BIT ); }
void clear_colour_buffer() { glClear( GL_COLOR_BUFFER_BIT ); }

bool should_window_close() {
  assert( g_window );
  return glfwWindowShouldClose( g_window );
}

void viewport( int x, int y, int w, int h ) { glViewport( x, y, w, h ); }

void swap_buffer() {
  assert( g_window );

  glfwSwapBuffers( g_window );
}
void poll_events() {
  assert( g_window );

  glfwPollEvents();
}

double get_time_s() { return glfwGetTime(); }

void framebuffer_dims( int* width, int* height ) {
  assert( width && height && g_window );
  glfwGetFramebufferSize( g_window, width, height );
}
void window_dims( int* width, int* height ) {
  assert( width && height && g_window );
  glfwGetWindowSize( g_window, width, height );
}

void rebuild_framebuffer( framebuffer_t* fb, int w, int h ) {
  assert( fb );
  assert( w > 0 && h > 0 );
  assert( fb->handle_gl > 0 );
  assert( fb->output_texture.handle_gl > 0 );
  assert( fb->depth_texture.handle_gl > 0 );

  fb->built = false; // invalidate
  fb->w     = w;
  fb->h     = h;

  glBindFramebuffer( GL_FRAMEBUFFER, fb->handle_gl );
  {
    fb->output_texture.w = fb->w;
    fb->output_texture.h = fb->h;
    update_texture( &fb->output_texture, NULL, false );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->output_texture.handle_gl, 0 );

    fb->depth_texture.w = fb->w;
    fb->depth_texture.h = fb->h;
    update_texture( &fb->depth_texture, NULL, false );
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
framebuffer_t create_framebuffer( int w, int h ) {
  framebuffer_t fb;

  glGenFramebuffers( 1, &fb.handle_gl );
  fb.output_texture = ( texture_t ){ .w = w, .h = h, .n_channels = 4 };
  glGenTextures( 1, &fb.output_texture.handle_gl );
  fb.depth_texture = ( texture_t ){ .w = w, .h = h, .n_channels = 4, .is_depth = true };
  glGenTextures( 1, &fb.depth_texture.handle_gl );

  rebuild_framebuffer( &fb, w, h );
  return fb;
}

void bind_framebuffer( const framebuffer_t* fb ) {
  if ( !fb ) {
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    return;
  }
  glBindFramebuffer( GL_FRAMEBUFFER, fb->handle_gl );
}

void read_pixels( int x, int y, int w, int h, int n_channels, uint8_t* data ) {
  GLenum format = GL_RGB;
  if ( 4 == n_channels ) { format = GL_RGBA; }

  glPixelStorei( GL_PACK_ALIGNMENT, 1 );   // for irregular display sizes in RGB
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // affects glReadPixels and subsequent texture calls alignment format

  glReadBuffer( GL_COLOR_ATTACHMENT0 );
  glReadPixels( x, y, w, h, format, GL_UNSIGNED_BYTE, data ); // note can be eg float
}

// TODO(Anton) bind default framebuffer in here
// using Sean Barrett's stb_image_write.h to write framebuffer to PNG
void screenshot() {
  int w = 0, h = 0;
  glBindFramebuffer( GL_FRAMEBUFFER, 0 );
  glfwGetFramebufferSize( g_window, &w, &h );

  uint8_t* buffer = malloc( w * h * 3 );
  assert( buffer );
  {
    glPixelStorei( GL_PACK_ALIGNMENT, 1 ); // for irregular display sizes in RGB
    glReadPixels( 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buffer );
    char name[1024];
    long int t = time( NULL );
    sprintf( name, "screenshot_%ld.png", t );
    uint8_t* last_row = buffer + ( w * 3 * ( h - 1 ) );
    if ( !stbi_write_png( name, w, h, 3, last_row, -3 * w ) ) { return; }
  }
  free( buffer );
}
