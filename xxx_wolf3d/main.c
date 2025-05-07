#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct mesh_t {
  GLuint vao, vbo;
  int n_points;
  GLenum primitive;
} mesh_t;

typedef struct shader_t {
  GLuint program;
  GLint u_scale, u_pos;
} shader_t;

typedef struct texture_t {
  GLuint handle;
  int w, h;
} texture_t;

mesh_t quad_mesh;
shader_t textured_shader;

mesh_t gfx_create_mesh_from_mem( float* points, int n_comps, int n_points, GLenum primitive ) {
  mesh_t mesh = { .n_points = n_points, .primitive = primitive };

  glGenVertexArrays( 1, &mesh.vao );
  glGenBuffers( 1, &mesh.vbo );
  glBindVertexArray( mesh.vao );
  glBindBuffer( GL_ARRAY_BUFFER, mesh.vbo );
  glBufferData( GL_ARRAY_BUFFER, n_comps * n_points * sizeof( float ), points, GL_STATIC_DRAW );
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, n_comps, GL_FLOAT, GL_FALSE, 0, NULL );
  glBindBuffer( GL_ARRAY_BUFFER, 0 );
  glBindVertexArray( 0 );

  return mesh;
}

shader_t gfx_create_shader_from_strings( const char* vert_str, const char* frag_str ) {
  shader_t shader = { .program = 0 };
  int params = -1, max_length = 2048, actual_length = 0;
  char slog[2048];

  GLuint vs = glCreateShader( GL_VERTEX_SHADER );
  glShaderSource( vs, 1, &vert_str, NULL );
  glCompileShader( vs );
  glGetShaderiv( vs, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    glGetShaderInfoLog( vs, max_length, &actual_length, slog );
    fprintf( stderr, "ERROR: Shader index %u did not compile.\n%s\n", vs, slog );
    return shader;
  }

  GLuint fs = glCreateShader( GL_FRAGMENT_SHADER );
  glShaderSource( fs, 1, &frag_str, NULL );
  glCompileShader( fs );
  glGetShaderiv( fs, GL_COMPILE_STATUS, &params );
  if ( GL_TRUE != params ) {
    glGetShaderInfoLog( fs, max_length, &actual_length, slog );
    fprintf( stderr, "ERROR: Shader index %u did not compile.\n%s\n", vs, slog );
    return shader;
  }

  GLuint program = glCreateProgram();
  glAttachShader( program, fs );
  glAttachShader( program, vs );
  glLinkProgram( program );
  glGetProgramiv( program, GL_LINK_STATUS, &params );
  if ( GL_TRUE != params ) {
    glGetProgramInfoLog( program, max_length, &actual_length, slog );
    fprintf( stderr, "ERROR: Could not link shader program GL index %u.\n%s\n", program, slog );
    return shader;
  }

  shader.u_scale = glGetUniformLocation( program, "u_scale" );
  shader.u_pos   = glGetUniformLocation( program, "u_pos" );
  shader.program = program;
  return shader;
}

texture_t gfx_create_texture_from_mem( int w, int h, void* pixels ) {
  texture_t texture = { .handle = 0, .w = w, .h = h };
  glActiveTexture( GL_TEXTURE0 );
  glGenTextures( 1, &texture.handle );
  glBindTexture( GL_TEXTURE_2D, texture.handle );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glBindTexture( GL_TEXTURE_2D, 0 );
  return texture;
}

void gfx_update_texture_from_mem( texture_t* texture_ptr, void* pixels ) {
  // glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, texture_ptr->handle );
  glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, texture_ptr->w, texture_ptr->h, GL_RGB, GL_UNSIGNED_BYTE, pixels );
  glBindTexture( GL_TEXTURE_2D, 0 );
  // glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
}

GLFWwindow* gfx_start( int win_w, int win_h, const char* title ) {
  if ( !glfwInit() ) { return false; }
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
  glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  GLFWwindow* window = glfwCreateWindow( win_w, win_h, title, NULL, NULL );
  if ( !window ) {
    glfwTerminate();
    return NULL;
  }
  glfwMakeContextCurrent( window );
  int version_glad = gladLoadGL( glfwGetProcAddress );
  if ( version_glad == 0 ) { return false; }
  printf( "Loaded OpenGL %i.%i\n", GLAD_VERSION_MAJOR( version_glad ), GLAD_VERSION_MINOR( version_glad ) );
  printf( "Renderer: %s.\n", glGetString( GL_RENDERER ) );
  printf( "OpenGL version supported %s.\n", glGetString( GL_VERSION ) );

  { // 2D mesh.
    float points[] = { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f };
    quad_mesh      = gfx_create_mesh_from_mem( points, 2, 4, GL_TRIANGLE_STRIP );
  }

  {
    const char* vertex_shader =
      "#version 410 core\n"
      "in vec2 vp;"
      "uniform vec2 u_scale, u_pos;"
      "out vec2 st;"
      "void main() {"
      "  st = vp.xy * 0.5 + 0.5;"
      "  gl_Position = vec4( vp * u_scale + u_pos, 0.0, 1.0 );"
      "}";
    const char* fragment_shader =
      "#version 410 core\n"
      "in vec2 st;"
      "uniform sampler2D u_tex;"
      "out vec4 frag_colour;"
      "void main() {"
      "  vec4 texel = texture( u_tex, st );"
      "  frag_colour = vec4( texel.rgb, 1.0 );"
      "}";
    textured_shader = gfx_create_shader_from_strings( vertex_shader, fragment_shader );
    if ( 0 == textured_shader.program ) {
      glfwTerminate();
      return NULL;
    }
  }

  return window;
}

void raycast( int w, int h, uint8_t* main_img_ptr ) {
  memset( main_img_ptr, 0, w * h * 3 ); // Should really be a single colour pal index.
  for ( int x = 0; x < w; x++ ) {
    // Raycast distance to nearest square here.
    float dist                    = 100.0f;
    const float max_visible_range = 500.0f;
    float height_on_screen        = ( 1.0f - dist / max_visible_range ) * h;
    int half_gap_y                = (int)( ( h - height_on_screen ) / 2.0f );
    for ( int y = half_gap_y; y < h - half_gap_y; y++ ) {
      // Draw a column of texture here.
      int img_idx               = ( y * w + x ) * 3;
      main_img_ptr[img_idx + 0] = 0x00;
      main_img_ptr[img_idx + 1] = 0xFF;
      main_img_ptr[img_idx + 2] = 0x00;
    }
  }
}

int main() {
  int rt_res_w = 320, rt_res_h = 200;
  int map_res_w = 256, map_res_h = 256;
  int win_w = 1024, win_h = 768;
  GLFWwindow* window = gfx_start( win_w, win_h, "Antolf 3-D" );
  if ( !window ) { return 1; }

  uint8_t* main_img_ptr = calloc( rt_res_w * rt_res_h * 3, 1 );
  uint8_t* map_img_ptr  = calloc( map_res_w * map_res_h * 3, 1 );

  // for ( int i = 0; i < rt_res_w * rt_res_h * 3; i++ ) { main_img_ptr[i] = rand() % 255; }
  texture_t main_texture = gfx_create_texture_from_mem( rt_res_w, rt_res_h, main_img_ptr );
  texture_t map_texture  = gfx_create_texture_from_mem( map_res_w, map_res_h, map_img_ptr );

  raycast( rt_res_w, rt_res_h, main_img_ptr );
  gfx_update_texture_from_mem( &main_texture, main_img_ptr );

  while ( !glfwWindowShouldClose( window ) ) {
    glViewport( 0, 0, win_w, win_h );
    glfwPollEvents();
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_ESCAPE ) ) { break; }

    glViewport( 0, 0, win_w, win_h );
    glClearColor( 0.2f, 0.2f, 0.2f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram( textured_shader.program );
    glBindVertexArray( quad_mesh.vao );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, main_texture.handle );
    glUniform2f( textured_shader.u_scale, 1.0f, 1.0f );
    glUniform2f( textured_shader.u_pos, 0.0f, 0.0f );
    glDrawArrays( quad_mesh.primitive, 0, quad_mesh.n_points );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, map_texture.handle );
    glUniform2f( textured_shader.u_scale, 0.25f, 0.25f );
    glUniform2f( textured_shader.u_pos, 0.75f, 0.75f );
    glDrawArrays( quad_mesh.primitive, 0, quad_mesh.n_points );

    glfwSwapBuffers( window );
  }

  glfwTerminate();
  free( main_img_ptr );
  free( map_img_ptr );

  printf( "Normal exit.\n" );
  return 0;
}
