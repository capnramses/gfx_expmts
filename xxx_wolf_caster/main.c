/**
 *
 * References:
 * https://lodev.org/cgtutor/raycasting.html
 * https://www.youtube.com/watch?v=gYRrGTC7GtA
 *
 */
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846f

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

typedef struct player_t {
  float pos_x, pos_y;
  float angle;
  float dir_x, dir_y;
} player_t;

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
  glTexImage2D( GL_TEXTURE_2D, 0, GL_SRGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels );
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
  glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, texture_ptr->w, texture_ptr->h, GL_RGB, GL_UNSIGNED_BYTE, pixels );
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
      "  frag_colour = vec4( pow(texel.rgb, vec3( 1.0 / 2.2 ) ), 1.0 );"
      "}";
    textured_shader = gfx_create_shader_from_strings( vertex_shader, fragment_shader );
    if ( 0 == textured_shader.program ) {
      glfwTerminate();
      return NULL;
    }
  }

  return window;
}

void raycast( int w, int h, uint8_t* main_img_ptr, player_t player ) {
  const uint8_t ceil_colour[]  = { 0x44, 0x44, 0x44 };
  const uint8_t floor_colour[] = { 0x33, 0x33, 0x33 };

  memset( main_img_ptr, 0, w * h * 3 ); // Should really be a single colour pal index.

  // Set ceiling and floor colours.
  for ( int y = 0; y < h / 2; y++ ) {
    for ( int x = 0; x < w; x++ ) {
      int img_idx = ( y * w + x ) * 3;
      memcpy( &main_img_ptr[img_idx], ceil_colour, 3 );
    }
  }
  for ( int y = h / 2; y < h; y++ ) {
    for ( int x = 0; x < w; x++ ) {
      int img_idx = ( y * w + x ) * 3;
      memcpy( &main_img_ptr[img_idx], floor_colour, 3 );
    }
  }

  for ( int x = 0; x < w; x++ ) {
    // Raycast distance to nearest square here.
    float dist                    = 100.0f;
    const float max_visible_range = 500.0f;
    float height_on_screen        = ( 1.0f - dist / max_visible_range ) * h;
    int half_gap_y                = (int)( ( h - height_on_screen ) / 2.0f );
    for ( int y = half_gap_y; y < h - half_gap_y; y++ ) {
      // Draw a column of texture here.
      int img_idx               = ( y * w + x ) * 3;
      main_img_ptr[img_idx + 0] = 0x11;
      main_img_ptr[img_idx + 1] = 0x22;
      main_img_ptr[img_idx + 2] = 0x88;
    }
  }
}

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
      memcpy( &img_ptr[( y * w + x ) * n_chans], rgb, n_chans );
      // plot( x, y + 1, R2, G2, B2 );
      // plot( x, y - 1, R2, G2, B2 );
      if ( err >= 0 ) {
        err -= d2_x;
        y += i_y;
      }
      err += d2_y;
      x += i_x;
    } // endfor
  } else {
    int err = d2_x - d_y;
    for ( int i = 0; i <= d_y; i++ ) {
      memcpy( &img_ptr[( y * w + x ) * n_chans], rgb, n_chans );
      // plot( x + 1, y, R2, G2, B2 );
      // plot( x - 1, y, R2, G2, B2 );
      if ( err >= 0 ) {
        err -= d2_y;
        x += i_x;
      }
      err += d2_x;
      y += i_y;
    } // endfor
  } // endif
} // endfunc

const int map_w     = 8;
const int map_h     = 8;
const uint8_t map[] = {
  1, 1, 1, 1, 1, 1, 1, 1, // 0
  1, 0, 1, 0, 1, 0, 0, 1, // 1
  1, 0, 1, 0, 1, 0, 0, 1, // 2
  1, 0, 0, 0, 0, 0, 0, 1, // 3
  1, 0, 0, 0, 1, 1, 1, 1, // 4
  1, 0, 0, 1, 1, 0, 0, 1, // 5
  1, 1, 0, 0, 0, 0, 0, 1, // 6
  1, 1, 1, 1, 1, 1, 1, 1  // 7
};

void draw_minimap( uint8_t* minimap_img_ptr, texture_t* minimap_texture, player_t player ) {
  const int wall_w     = 32;
  uint8_t wall_col[]   = { 0x77, 0x77, 0x77 };
  uint8_t floor_col[]  = { 0x22, 0x22, 0x22 };
  uint8_t player_col[] = { 0xCC, 0xCC, 0x00 };
  uint8_t ray_col[]    = { 0x00, 0xCC, 0x00 };
  memset( minimap_img_ptr, 0, minimap_texture->w * minimap_texture->h * 3 );
  // walls
  for ( int y = 0; y < map_h; y++ ) {
    for ( int x = 0; x < map_w; x++ ) {
      int map_idx      = y * map_w + x;
      uint8_t colour[] = { 0x22, 0x22, 0x22 };
      if ( 1 == map[map_idx] ) { memcpy( colour, wall_col, 3 ); }
      for ( int r = 1; r < wall_w - 1; r++ ) {
        for ( int c = 1; c < wall_w - 1; c++ ) {
          int img_idx = ( y * wall_w * minimap_texture->h + x * wall_w + r * minimap_texture->h + c ) * 3;
          memcpy( &minimap_img_ptr[img_idx], colour, 3 );
        }
      }
    }
  }
  { // draw player
    int x = (int)( player.pos_x / ( (float)map_w ) * minimap_texture->w );
    int y = (int)( player.pos_y / ( (float)map_h ) * minimap_texture->h );
    for ( int r = y - 5; r < y + 5; r++ ) {
      for ( int c = x - 5; c < x + 5; c++ ) {
        int img_idx = ( r * minimap_texture->w + c ) * 3;
        memcpy( &minimap_img_ptr[img_idx], player_col, 3 );
      }
    }
    plot_line( x, y, x + player.dir_x * 15, y + player.dir_y * 15, player_col, minimap_img_ptr, minimap_texture->w, minimap_texture->h, 3 );
  }

  float ray_angle = player.angle;
  float x_offs = 0, y_offs = 0;
  float rx = 0, ry = 0;
  int dof = 0, mx = 0, my = 0, mp = 0;
  float a_tan = -1.0f / tanf( ray_angle );
  for ( int r = 0; r < 1; r++ ) {
    if ( ray_angle > PI ) {
      ry     = ( ( (int)player.pos_y >> 6 ) << 6 )  - 0.0001f; // Round to nearest 64 by /64 then *64.
      rx     = ( player.pos_y - ry ) * a_tan + player.pos_x;
      y_offs = -64.0f;
      x_offs = -y_offs * a_tan;
    } else if ( ray_angle > 0.0f && ray_angle < PI ) {
      ry     = ( ( (int)player.pos_y >> 6 ) << 6 )+ 64.0f; // Round to nearest 64 by /64 then *64.
      rx     = ( player.pos_y - ry ) * a_tan + player.pos_x;
      y_offs = 64.0f;
      x_offs = -y_offs * a_tan;
    } else if ( 0.0f == ray_angle || PI == ray_angle ) { // Exactly left or right.
      rx  = player.pos_x;
      ry  = player.pos_y;
      dof = 8;
    }
    while ( dof < 8 ) {
      mx = (int)( rx ) >> 6;
      my = (int)( ry ) >> 6;
      mp = my * map_w + mx; // mp = map position i guess.
      if ( mp < map_w * map_h && 1 == map[mp] ) {
        dof = 8;
      } // Hit wall.
      else {
        rx += x_offs;
        ry + y_offs;
        dof += 1;
      } // next line.
    }
    plot_line( player.pos_x, player.pos_y, rx, ry, ray_col, minimap_img_ptr, minimap_texture->w, minimap_texture->h, 3 );
  }
}

void move_player( GLFWwindow* window, player_t* p_ptr, double elapsed_s ) {
  const float speed     = 1.0f;
  const float rot_speed = 2.0f;
  if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_LEFT ) ) { p_ptr->angle -= rot_speed * elapsed_s; }
  if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_RIGHT ) ) { p_ptr->angle += rot_speed * elapsed_s; }
  p_ptr->angle = fmodf( p_ptr->angle, 2.0 * PI );
  p_ptr->angle = p_ptr->angle >= 0.0f ? p_ptr->angle : p_ptr->angle + 2.0 * PI;
  p_ptr->dir_x = cosf( p_ptr->angle );
  p_ptr->dir_y = sinf( p_ptr->angle );
  if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_UP ) ) {
    p_ptr->pos_x += p_ptr->dir_x * speed * elapsed_s;
    p_ptr->pos_y += p_ptr->dir_y * speed * elapsed_s;
  }
  if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_DOWN ) ) {
    p_ptr->pos_x -= p_ptr->dir_x * speed * elapsed_s;
    p_ptr->pos_y -= p_ptr->dir_y * speed * elapsed_s;
  }
}

int main() {
  int rt_res_w = 320, rt_res_h = 200;
  int minimap_res_w = 256, minimap_res_h = 256;
  int win_w = 1024, win_h = 768;
  GLFWwindow* window = gfx_start( win_w, win_h, "Antolf 3-D" );
  if ( !window ) { return 1; }

  player_t player = { .pos_x = 1.5f, .pos_y = 3.5f };

  uint8_t* main_img_ptr    = calloc( rt_res_w * rt_res_h * 3, 1 );
  uint8_t* minimap_img_ptr = calloc( minimap_res_w * minimap_res_h * 3, 1 );

  for ( int i = 0; i < minimap_res_w * minimap_res_h * 3; i++ ) { minimap_img_ptr[i] = rand() % 255; }
  texture_t main_texture    = gfx_create_texture_from_mem( rt_res_w, rt_res_h, main_img_ptr );
  texture_t minimap_texture = gfx_create_texture_from_mem( minimap_res_w, minimap_res_h, minimap_img_ptr );

  double prev_s = glfwGetTime();
  while ( !glfwWindowShouldClose( window ) ) {
    double curr_s    = glfwGetTime();
    double elapsed_s = curr_s - prev_s;
    prev_s           = curr_s;
    glfwPollEvents();
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_ESCAPE ) ) { break; }

    move_player( window, &player, elapsed_s );

    raycast( rt_res_w, rt_res_h, main_img_ptr, player );
    gfx_update_texture_from_mem( &main_texture, main_img_ptr );
    draw_minimap( minimap_img_ptr, &minimap_texture, player );
    gfx_update_texture_from_mem( &minimap_texture, minimap_img_ptr );

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
    glBindTexture( GL_TEXTURE_2D, minimap_texture.handle );
    glUniform2f( textured_shader.u_scale, 0.5f, 0.5f );
    glUniform2f( textured_shader.u_pos, 0.5f, 0.5f );
    glDrawArrays( quad_mesh.primitive, 0, quad_mesh.n_points );

    glfwSwapBuffers( window );
  }

  glfwTerminate();
  free( main_img_ptr );
  free( minimap_img_ptr );

  printf( "Normal exit.\n" );
  return 0;
}
