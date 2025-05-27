#include "gfx.h"
#include "maths.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

texture_t gfx_create_texture_from_mem( int w, int h, int n, void* pixels ) {
  texture_t texture = { .handle = 0, .w = w, .h = h, .n = n };
  glActiveTexture( GL_TEXTURE0 );
  glGenTextures( 1, &texture.handle );
  glBindTexture( GL_TEXTURE_2D, texture.handle );
  if ( 3 == n ) {
    glTexImage2D( GL_TEXTURE_2D, 0, GL_SRGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels );
  } else if ( 4 == n ) {
    glTexImage2D( GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
  }
  assert( n == 3 || n == 4 );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glBindTexture( GL_TEXTURE_2D, 0 );
  return texture;
}

void gfx_update_texture_from_mem( texture_t* texture_ptr, void* pixels ) {
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, texture_ptr->handle );
  if ( texture_ptr->n == 3 ) {
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, texture_ptr->w, texture_ptr->h, GL_RGB, GL_UNSIGNED_BYTE, pixels );
  } else if ( texture_ptr->n == 4 ) {
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, texture_ptr->w, texture_ptr->h, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
  }
  assert( texture_ptr->n == 3 || texture_ptr->n == 4 );
  glBindTexture( GL_TEXTURE_2D, 0 );
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
      "  st.t = 1.0 - st.t;"
      "  gl_Position = vec4( vp * u_scale + u_pos, 0.0, 1.0 );"
      "}";
    const char* fragment_shader =
      "#version 410 core\n"
      "in vec2 st;"
      "uniform sampler2D u_tex;"
      "out vec4 frag_colour;"
      "void main() {"
      "  vec4 texel = texture( u_tex, st );"
      "  frag_colour = vec4( pow(texel.rgb, vec3( 1.0 / 2.2 ) ), texel.a );"
      "}";
    textured_shader = gfx_create_shader_from_strings( vertex_shader, fragment_shader );
    if ( 0 == textured_shader.program ) {
      glfwTerminate();
      return NULL;
    }
  }

  return window;
}

mesh_t quad_mesh;
shader_t textured_shader;

void plot_line( int x_i, int y_i, int x_f, int y_f, uint8_t* rgb, uint8_t* img_ptr, int w, int h, int n_chans ) {
  int x = x_i, y = y_i, d_x = x_f - x_i, d_y = y_f - y_i, i_x = 1, i_y = 1;
  if ( d_x < 0 ) {
    i_x = -1;
    d_x = abs( d_x );
  }
  if ( d_y < 0 ) {
    i_y = -1;
    d_y = abs( d_y );
  }
  int d2_x = d_x * 2, d2_y = d_y * 2;
  if ( d_x > d_y ) {
    int err = d2_y - d_x;
    for ( int i = 0; i <= d_x; i++ ) {
      if ( x < 0 || x >= w || y < 0 || y >= h ) { break; }
      memcpy( &img_ptr[( y * w + x ) * n_chans], rgb, n_chans );
      if ( err >= 0 ) {
        err -= d2_x;
        y += i_y;
        if ( y >= h ) { break; }
      }
      err += d2_y;
      x += i_x;
      if ( x >= w ) { break; }
    } // endfor
  } else {
    int err = d2_x - d_y;
    for ( int i = 0; i <= d_y; i++ ) {
      if ( x < 0 || x >= w || y < 0 || y >= h ) { break; }
      memcpy( &img_ptr[( y * w + x ) * n_chans], rgb, n_chans );
      if ( err >= 0 ) {
        err -= d2_y;
        x += i_x;
        if ( x >= w ) { break; }
      }
      err += d2_x;
      y += i_y;
      if ( y >= h ) { break; }
    } // endfor
  } // endif
} // endfunc

void _put_circle_px( int c_x, int c_y, int x, int y, uint8_t* img_ptr, int w, int h, int n, uint8_t* rgb_ptr ) {
  int ys[] = { c_y + y, c_y + y, c_y - y, c_y - y, c_y + x, c_y + x, c_y - x, c_y - x };
  int xs[] = { c_x + x, c_x - x, c_x + x, c_x - x, c_x + y, c_x - y, c_x + y, c_x - y };
  for ( int i = 0; i < 8; i++ ) {
    if ( ys[i] < 0 || ys[i] >= h || xs[i] < 0 || xs[i] >= w ) { continue; }
    int idx = ( ys[i] * w + xs[i] ) * n;
    memcpy( &img_ptr[idx], rgb_ptr, n );
  }
}

// Bresenham
void plot_circle( int c_x, int c_y, int r, uint8_t* img_ptr, int w, int h, int n, uint8_t* rgb_ptr ) {
  assert( img_ptr && rgb_ptr );
  int x = 0, y = r;
  int d = 3 - 2 * r;
  _put_circle_px( c_x, c_y, x, y, img_ptr, w, h, n, rgb_ptr );
  while ( y >= x ) {
    if ( d > 0 ) {
      y--;
      d = d + 4 * ( x - y ) + 10;
    } else {
      d = d + 4 * x + 6;
    }
    x++;
    _put_circle_px( c_x, c_y, x, y, img_ptr, w, h, n, rgb_ptr );
  }
}

void plot_t_cross( int c_x, int c_y, int r, uint8_t* img_ptr, int w, int h, int n, uint8_t* rgb_ptr ) {
  for ( int y = APG_CLAMP( c_y - r, 0, h - 1 ); y <= APG_CLAMP( c_y + r, 0, h - 1 ); y++ ) {
    if ( c_y < 0 || c_y >= h || c_x < 0 || c_x >= w ) { continue; }
    int mem_idx = ( y * w + c_x ) * n;
    memcpy( &img_ptr[mem_idx], rgb_ptr, n );
  }
  for ( int x = APG_CLAMP( c_x - r, 0, w - 1 ); x <= APG_CLAMP( c_x + r, 0, w - 1 ); x++ ) {
    if ( c_y < 0 || c_y >= h || c_x < 0 || c_x >= w ) { continue; }
    int mem_idx = ( c_y * w + x ) * n;
    memcpy( &img_ptr[mem_idx], rgb_ptr, n );
  }
}
