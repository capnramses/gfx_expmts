#include "gfx.h"
#include "glcontext.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define GFX_SHADER_BINDING_VP 0
#define GFX_SHADER_BINDING_VT 1
#define GFX_SHADER_BINDING_VN 2
#define GFX_SHADER_BINDING_VC 3
#define GFX_MAX_SHADER_STR 10000

GLFWwindow* gfx_window_ptr; // extern in apg_glcontext.h

static GLFWmonitor* gfx_monitor_ptr;
static int g_win_width = 1920, g_win_height = 1080;

gfx_shader_t gfx_default_shader;
gfx_mesh_t gfx_cube_mesh;

bool gfx_start( const char* window_title, int w, int h, bool fullscreen ) {
  g_win_width  = w;
  g_win_height = h;
  { // GLFW
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
  { // GLAD
    if ( !gladLoadGL() ) {
      fprintf( stderr, "FATAL ERROR: Could not load OpenGL header using Glad.\n" );
      return false;
    }
    printf( "OpenGL %d.%d\n", GLVersion.major, GLVersion.minor );
  }

  const GLubyte* renderer = glGetString( GL_RENDERER );
  const GLubyte* version  = glGetString( GL_VERSION );
  printf( "Renderer: %s\n", renderer );
  printf( "OpenGL version supported %s\n", version );

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
  { //
    gfx_cube_mesh  = ( gfx_mesh_t ){ .n_vertices = 36 };
    float points[] = { -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f };
    glGenBuffers( 1, &gfx_cube_mesh.points_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, gfx_cube_mesh.points_vbo );
    glBufferData( GL_ARRAY_BUFFER, 3 * 36 * sizeof( GLfloat ), &points, GL_STATIC_DRAW );
    glGenVertexArrays( 1, &gfx_cube_mesh.vao );
    glBindVertexArray( gfx_cube_mesh.vao );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );
  }
  {
    glClearColor( 0.5, 0.5, 0.5, 1.0 );
    glDepthFunc( GL_LESS );
    glEnable( GL_DEPTH_TEST );
    glCullFace( GL_BACK );
    glFrontFace( GL_CCW );
    glEnable( GL_CULL_FACE );

    glfwSwapInterval( 1 ); // vsync
  }

  return true;
}

void gfx_stop() {
  gfx_delete_shader_program( &gfx_default_shader );
  gfx_delete_mesh( &gfx_cube_mesh );
  glfwTerminate();
}

bool gfx_should_window_close() {
  assert( gfx_window_ptr );
  return glfwWindowShouldClose( gfx_window_ptr );
}

void gfx_viewport( int x, int y, int w, int h ) { glViewport( x, y, w, h ); }

void gfx_clear_colour_and_depth_buffers( float r, float g, float b, float a ) {
  glClearColor( r, g, b, a );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void gfx_framebuffer_dims( int* width, int* height ) {
  assert( width && height && gfx_window_ptr );
  glfwGetFramebufferSize( gfx_window_ptr, width, height );
}
void gfx_window_dims( int* width, int* height ) {
  assert( width && height && gfx_window_ptr );
  glfwGetWindowSize( gfx_window_ptr, width, height );
}

void gfx_swap_buffer() {
  assert( gfx_window_ptr );

  glfwSwapBuffers( gfx_window_ptr );
}

void gfx_poll_events() {
  assert( gfx_window_ptr );

  glfwPollEvents();
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

void gfx_depth_mask( bool enable ) {
  if ( enable ) {
    glDepthMask( GL_TRUE );
  } else {
    glDepthMask( GL_FALSE );
  }
}

gfx_mesh_t gfx_create_mesh_from_mem(                                                     //
  const float* points_buffer, int n_points_comps,                                        //
  const float* texcoords_buffer, int n_texcoord_comps,                                   //
  const float* normals_buffer, int n_normal_comps,                                       //
  const float* colours_buffer, int n_colours_comps,                                      //
  const void* indices_buffer, size_t indices_buffer_sz, gfx_indices_type_t indices_type, //
  int n_vertices, bool dynamic ) {
  gfx_mesh_t mesh = ( gfx_mesh_t ){ .indices_type = indices_type, .n_vertices = n_vertices, .dynamic = dynamic };

  if ( !points_buffer || n_points_comps <= 0 ) {
    fprintf( stderr, "ERROR:loading mesh from memory. No vertex points given.\n" );
    return mesh;
  }

  glGenVertexArrays( 1, &mesh.vao );
  glGenBuffers( 1, &mesh.points_vbo );
  if ( colours_buffer && n_colours_comps > 0 ) { glGenBuffers( 1, &mesh.colours_vbo ); }
  if ( texcoords_buffer && n_texcoord_comps > 0 ) { glGenBuffers( 1, &mesh.texcoords_vbo ); }
  if ( normals_buffer && n_normal_comps > 0 ) { glGenBuffers( 1, &mesh.normals_vbo ); }
  if ( indices_buffer ) { glGenBuffers( 1, &mesh.indices_vbo ); }

  gfx_update_mesh_from_mem( &mesh, points_buffer, n_points_comps, texcoords_buffer, n_texcoord_comps, normals_buffer, n_normal_comps, colours_buffer,
    n_colours_comps, indices_buffer, indices_buffer_sz, indices_type, n_vertices, dynamic );

  return mesh;
}

void gfx_update_mesh_from_mem( gfx_mesh_t* mesh,                                         //
  const float* points_buffer, int n_points_comps,                                        //
  const float* texcoords_buffer, int n_texcoord_comps,                                   //
  const float* normals_buffer, int n_normal_comps,                                       //
  const float* colours_buffer, int n_colours_comps,                                      //
  const void* indices_buffer, size_t indices_buffer_sz, gfx_indices_type_t indices_type, //
  int n_vertices, bool dynamic ) {
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
    glEnableVertexAttribArray( GFX_SHADER_BINDING_VP );
    glVertexAttribPointer( GFX_SHADER_BINDING_VP, n_points_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( texcoords_buffer && n_texcoord_comps > 0 && mesh->texcoords_vbo ) {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->texcoords_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_texcoord_comps * n_vertices, texcoords_buffer, usage );
    glEnableVertexAttribArray( GFX_SHADER_BINDING_VT );
    glVertexAttribPointer( GFX_SHADER_BINDING_VT, n_texcoord_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( normals_buffer && n_normal_comps > 0 && mesh->normals_vbo ) {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->normals_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_normal_comps * n_vertices, normals_buffer, usage );
    glEnableVertexAttribArray( GFX_SHADER_BINDING_VN );
    glVertexAttribPointer( GFX_SHADER_BINDING_VN, n_normal_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( colours_buffer && n_colours_comps > 0 && mesh->colours_vbo ) {
    glBindBuffer( GL_ARRAY_BUFFER, mesh->colours_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( float ) * n_colours_comps * n_vertices, colours_buffer, usage );
    glEnableVertexAttribArray( GFX_SHADER_BINDING_VC );
    glVertexAttribPointer( GFX_SHADER_BINDING_VC, n_colours_comps, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
  }
  if ( indices_buffer && mesh->indices_vbo ) {
    mesh->indices_type = indices_type;
    mesh->n_indices    = 0;
    if ( indices_buffer_sz > 0 ) {
      switch ( indices_type ) {
      case 0: mesh->n_indices = indices_buffer_sz; break;     // ubyte
      case 1: mesh->n_indices = indices_buffer_sz / 2; break; // ushort
      case 2: mesh->n_indices = indices_buffer_sz / 4; break; // uint
      default: break;
      }
    }
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh->indices_vbo );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices_buffer_sz, indices_buffer, usage );
  }
  glBindVertexArray( 0 );
}

void gfx_delete_mesh( gfx_mesh_t* mesh ) {
  assert( mesh );

  if ( mesh->indices_vbo ) { glDeleteBuffers( 1, &mesh->indices_vbo ); }
  if ( mesh->colours_vbo ) { glDeleteBuffers( 1, &mesh->colours_vbo ); }
  if ( mesh->normals_vbo ) { glDeleteBuffers( 1, &mesh->normals_vbo ); }
  if ( mesh->texcoords_vbo ) { glDeleteBuffers( 1, &mesh->texcoords_vbo ); }
  if ( mesh->points_vbo ) { glDeleteBuffers( 1, &mesh->points_vbo ); }
  if ( mesh->vao ) { glDeleteVertexArrays( 1, &mesh->vao ); }
  *mesh = ( gfx_mesh_t ){ .n_vertices = 0 };
}

gfx_shader_t gfx_create_shader_program_from_files( const char* vert_shader_filename, const char* frag_shader_filename ) {
  gfx_shader_t shader = ( gfx_shader_t ){ .program_gl = 0 };

  char vert_shader_str[GFX_MAX_SHADER_STR], frag_shader_str[GFX_MAX_SHADER_STR];
  {
    FILE* f_ptr = fopen( vert_shader_filename, "rb" );
    if ( !f_ptr ) {
      fprintf( stderr, "ERROR: opening shader file `%s`\n", vert_shader_filename );
      return shader;
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
      return shader;
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
  glBindAttribLocation( shader.program_gl, GFX_SHADER_BINDING_VP, "a_vp" );
  glBindAttribLocation( shader.program_gl, GFX_SHADER_BINDING_VT, "a_vt" );
  glBindAttribLocation( shader.program_gl, GFX_SHADER_BINDING_VN, "a_vn" );
  glBindAttribLocation( shader.program_gl, GFX_SHADER_BINDING_VC, "a_vc" );
  glLinkProgram( shader.program_gl );
  glDeleteShader( vs );
  glDeleteShader( fs );

  shader.u_M                         = glGetUniformLocation( shader.program_gl, "u_M" );
  shader.u_V                         = glGetUniformLocation( shader.program_gl, "u_V" );
  shader.u_P                         = glGetUniformLocation( shader.program_gl, "u_P" );
  shader.u_texture_a                 = glGetUniformLocation( shader.program_gl, "u_texture_a" );
  shader.u_texture_b                 = glGetUniformLocation( shader.program_gl, "u_texture_b" );
  shader.u_texture_albedo            = glGetUniformLocation( shader.program_gl, "u_texture_albedo" );
  shader.u_texture_metal_roughness   = glGetUniformLocation( shader.program_gl, "u_texture_metal_roughness" );
  shader.u_texture_emissive          = glGetUniformLocation( shader.program_gl, "u_texture_emissive" );
  shader.u_texture_ambient_occlusion = glGetUniformLocation( shader.program_gl, "u_texture_ambient_occlusion" );
  shader.u_texture_normal            = glGetUniformLocation( shader.program_gl, "u_texture_normal" );
  shader.u_texture_environment       = glGetUniformLocation( shader.program_gl, "u_texture_environment" );
  shader.u_alpha                     = glGetUniformLocation( shader.program_gl, "u_alpha" );
  shader.u_base_colour_rgba          = glGetUniformLocation( shader.program_gl, "u_base_colour_rgba" );
  shader.u_roughness_factor          = glGetUniformLocation( shader.program_gl, "u_roughness_factor" );
  shader.u_cam_pos_wor               = glGetUniformLocation( shader.program_gl, "u_cam_pos_wor" );
  glProgramUniform1i( shader.program_gl, shader.u_texture_a, 0 );
  glProgramUniform1i( shader.program_gl, shader.u_texture_b, 1 );
  glProgramUniform1i( shader.program_gl, shader.u_texture_albedo, GFX_TEXTURE_UNIT_ALBEDO );
  glProgramUniform1i( shader.program_gl, shader.u_texture_metal_roughness, GFX_TEXTURE_UNIT_METAL_ROUGHNESS );
  glProgramUniform1i( shader.program_gl, shader.u_texture_emissive, GFX_TEXTURE_UNIT_EMISSIVE );
  glProgramUniform1i( shader.program_gl, shader.u_texture_ambient_occlusion, GFX_TEXTURE_UNIT_AMBIENT_OCCLUSION );
  glProgramUniform1i( shader.program_gl, shader.u_texture_normal, GFX_TEXTURE_UNIT_NORMAL );
  glProgramUniform1i( shader.program_gl, shader.u_texture_environment, GFX_TEXTURE_UNIT_ENVIRONMENT );

  int u_texture_albedo;            // default to value 0
  int u_texture_metal_roughness;   // default to value 1
  int u_texture_emissive;          // default to value 2
  int u_texture_ambient_occlusion; // default to value 3
  int u_texture_normal;            // default to value 4
  int u_texture_environment;       // default to value 5

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
  if ( !res_a || !res_b || !linked ) { return shader; } // should return default shader here
  shader.loaded = true;
  return shader;
}

void gfx_delete_shader_program( gfx_shader_t* shader ) {
  assert( shader );
  if ( shader->program_gl ) { glDeleteProgram( shader->program_gl ); }
  *shader = ( gfx_shader_t ){ .program_gl = 0 };
}

bool gfx_uniform1f( gfx_shader_t shader, int uniform_location, float f ) {
  if ( !shader.program_gl ) { return false; }
  if ( uniform_location < 0 ) { return false; }
  glProgramUniform1f( shader.program_gl, uniform_location, f );
  return true;
}

bool gfx_uniform3f( gfx_shader_t shader, int uniform_location, float x, float y, float z ) {
  if ( !shader.program_gl ) { return false; }
  if ( uniform_location < 0 ) { return false; }
  glProgramUniform3f( shader.program_gl, uniform_location, x, y, z );
  return true;
}

bool gfx_uniform4f( gfx_shader_t shader, int uniform_location, float x, float y, float z, float w ) {
  if ( !shader.program_gl ) { return false; }
  if ( uniform_location < 0 ) { return false; }
  glProgramUniform4f( shader.program_gl, uniform_location, x, y, z, w );
  return true;
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

gfx_texture_t gfx_create_cube_texture_from_mem( uint8_t* imgs_buffer[6], int w, int h, int n_channels, gfx_texture_properties_t properties ) {
  assert( 4 == n_channels || 3 == n_channels || 1 == n_channels ); // 2 not used yet so not impl
  gfx_texture_t texture      = ( gfx_texture_t ){ .properties = properties };
  texture.properties.is_cube = true;
  glGenTextures( 1, &texture.handle_gl );
  glBindTexture( GL_TEXTURE_CUBE_MAP, texture.handle_gl );
  {
    for ( int i = 0; i < 6; i++ ) {
      if ( 4 == n_channels ) {
        glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgs_buffer[i] );
      } else if ( 3 == n_channels ) {
        glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, imgs_buffer[i] );
      } else if ( 1 == n_channels ) {
        glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, imgs_buffer[i] );
      }
    }

    // format cube map texture
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  }
  glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
  return texture;
}

void gfx_delete_texture( gfx_texture_t* texture ) {
  assert( texture && texture->handle_gl );
  glDeleteTextures( 1, &texture->handle_gl );
  memset( texture, 0, sizeof( gfx_texture_t ) );
}

void gfx_draw_mesh( gfx_mesh_t mesh, gfx_primitive_type_t pt, gfx_shader_t shader, float* P, float* V, float* M, gfx_texture_t* textures, int n_textures ) {
  if ( 0 == mesh.points_vbo || 0 == mesh.n_vertices ) { return; }

  GLenum mode = GL_TRIANGLES;
  if ( pt == GFX_PT_TRIANGLE_STRIP ) { mode = GL_TRIANGLE_STRIP; }
  if ( pt == GFX_PT_POINTS ) { mode = GL_POINTS; }

  for ( int i = 0; i < n_textures; i++ ) {
    GLenum tex_type = !textures[i].properties.is_array ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;
    if ( textures[i].properties.is_cube ) { tex_type = GL_TEXTURE_CUBE_MAP; }
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( tex_type, textures[i].handle_gl );
  }

  glUseProgram( shader.program_gl );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_P, 1, GL_FALSE, P );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_V, 1, GL_FALSE, V );
  glProgramUniformMatrix4fv( shader.program_gl, shader.u_M, 1, GL_FALSE, M );
  glBindVertexArray( mesh.vao );
  if ( mesh.indices_vbo ) {
    GLenum type = GL_UNSIGNED_BYTE;
    switch ( mesh.indices_type ) {
    case GFX_INDICES_TYPE_UBYTE: type = GL_UNSIGNED_BYTE; break;
    case GFX_INDICES_TYPE_UINT16: type = GL_UNSIGNED_SHORT; break;
    case GFX_INDICES_TYPE_UINT32: type = GL_UNSIGNED_INT; break;
    default: break;
    }
    glDrawElements( mode, mesh.n_indices, type, NULL );
  } else {
    glDrawArrays( mode, 0, mesh.n_vertices );
  }
  glBindVertexArray( 0 );
  glUseProgram( 0 );
  for ( int i = 0; i < n_textures; i++ ) {
    GLenum tex_type = !textures[i].properties.is_array ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;
    if ( textures[i].properties.is_cube ) { tex_type = GL_TEXTURE_CUBE_MAP; }
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( tex_type, 0 );
  }
}

void gfx_wireframe_mode() { glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); }

void gfx_polygon_mode() { glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ); }

double gfx_get_time_s() { return glfwGetTime(); }

bool input_is_key_held( int keycode ) {
  assert( gfx_window_ptr && keycode >= 32 && keycode <= GLFW_KEY_LAST );
  return glfwGetKey( gfx_window_ptr, keycode );
}
