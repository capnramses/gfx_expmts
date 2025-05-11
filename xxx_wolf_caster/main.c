/**
 *
 * References:
 * https://lodev.org/cgtutor/raycasting.html
 * https://www.youtube.com/watch?v=gYRrGTC7GtA
 *
 */
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APG_MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define APG_MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define APG_CLAMP( x, lo, hi ) ( APG_MIN( hi, APG_MAX( lo, x ) ) )
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

typedef struct rgb_t {
  uint8_t r, g, b;
} rgb_t;

mesh_t quad_mesh;
shader_t textured_shader;

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
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
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

  // Which box of the map we're in. Assumes boxes are 1.0 wide.
  int map_x = (int)player.pos_x;
  int map_y = (int)player.pos_y;

  float dir_x = player.dir_x, dir_y = player.dir_y;
  float plane_x = 0.0f, plane_y = 0.0f;

  for ( int x = 0; x < w; x++ ) {
    float angle_max       = 0.7854f;
    float angle_per_pixel = angle_max / ( w / 2 );
    const int ray_dist    = 1000;
    int xxx               = x;
    if ( x >= w / 2 ) { xxx = w / 2 - x; }
    float angle_a = xxx * angle_per_pixel + player.angle;
    float ray_dir_x   = cosf( angle_a );
    float ray_dir_y   = sinf( angle_a );

   // float camera_x  = 2 * x / (float)w - 1; // x-coordinate in camera space
   // float ray_dir_x = dir_x + plane_x * camera_x;
  //  float ray_dir_y = dir_y + plane_y * camera_x;

    // Length of ray from current position to next x or y-side.
    float side_dist_x;
    float side_dist_y;

    // Length of ray from one x or y-side to next x or y-side.
    float delta_dist_x = ( ray_dir_x == 0 ) ? FLT_MAX : fabsf( 1 / ray_dir_x );
    float delta_dist_y = ( ray_dir_y == 0 ) ? FLT_MAX : fabsf( 1 / ray_dir_y );
    float perpWallDist;

    // what direction to step in x or y-direction (either +1 or -1)
    int stepX;
    int stepY;

    int hit = 0; // was there a wall hit?
    int side;    // was a NS or a EW wall hit?

    // calculate step and initial sideDist
    if ( ray_dir_x < 0 ) {
      stepX       = -1;
      side_dist_x = ( player.pos_x - map_x ) * delta_dist_x;
    } else {
      stepX       = 1;
      side_dist_x = ( map_x + 1.0 - player.pos_x ) * delta_dist_x;
    }
    if ( ray_dir_y < 0 ) {
      stepY       = -1;
      side_dist_y = ( player.pos_y - map_y ) * delta_dist_y;
    } else {
      stepY       = 1;
      side_dist_y = ( map_y + 1.0 - player.pos_y ) * delta_dist_y;
    }

    // perform DDA
    while ( hit == 0 ) {
      // jump to next map square, either in x-direction, or in y-direction
      if ( side_dist_x < side_dist_y ) {
        side_dist_x += delta_dist_x;
        map_x += stepX;
        side = 0;
      } else {
        side_dist_y += delta_dist_y;
        map_y += stepY;
        side = 1;
      }
      // Check if ray has hit a wall
      if ( map[map_y * map_w + map_x] > 0 ) hit = 1;
    }

    // Calculate distance projected on camera direction (Euclidean distance would give fisheye effect!)
    if ( side == 0 )
      perpWallDist = ( side_dist_x - delta_dist_x );
    else
      perpWallDist = ( side_dist_y - delta_dist_y );

    // Calculate height of line to draw on screen
    int lineHeight = (int)( h / perpWallDist );

    // calculate lowest and highest pixel to fill in current stripe
    int drawStart = -lineHeight / 2 + h / 2;
    if ( drawStart < 0 ) drawStart = 0;
    int drawEnd = lineHeight / 2 + h / 2;
    if ( drawEnd >= h ) drawEnd = h - 1;

    // choose wall color
    rgb_t colour;
    switch ( map[map_y * map_w + map_x] ) {
    case 1: colour = (rgb_t){ 0x11, 0x22, 0x88 }; break;  // blue
    case 2: colour = (rgb_t){ 0x11, 0x88, 0x22 }; break;  // green
    case 3: colour = (rgb_t){ 0x88, 0x22, 0x11 }; break;  // red
    case 4: colour = (rgb_t){ 0x88, 0x88, 0x88 }; break;  // white
    case 0: colour = (rgb_t){ 0x00, 0x00, 0x00 }; break;  // white
    default: colour = (rgb_t){ 0x88, 0x88, 0x11 }; break; // yellow
    }

    // give x and y sides different brightness
    if ( side == 1 ) { colour = (rgb_t){ colour.r / 2, colour.g / 2, colour.b / 2 }; }

    // draw the pixels of the stripe as a vertical line
    for ( int y = drawStart; y < drawEnd; y++ ) {
      // Draw a column of texture here.
      int img_idx = ( y * w + x ) * 3;
      memcpy( &main_img_ptr[img_idx], &colour, 3 );
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
      if ( x < 0 || x >= w || y < 0 || y >= h ) { break; }
      memcpy( &img_ptr[( y * w + x ) * n_chans], rgb, n_chans );
      // plot( x, y + 1, R2, G2, B2 );
      // plot( x, y - 1, R2, G2, B2 );
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
      // plot( x + 1, y, R2, G2, B2 );
      // plot( x - 1, y, R2, G2, B2 );
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

void draw_minimap( uint8_t* minimap_img_ptr, texture_t* minimap_texture, player_t player ) {
  int wall_w           = minimap_texture->w / 8;
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
      for ( int r = 1; r < wall_w; r++ ) {
        for ( int c = 1; c < wall_w; c++ ) {
          int img_idx = ( y * wall_w * minimap_texture->h + x * wall_w + r * minimap_texture->h + c ) * 3;
          memcpy( &minimap_img_ptr[img_idx], colour, 3 );
        }
      }
    }
  }
  { // draw player
    int x = (int)( player.pos_x / ( (float)map_w ) * minimap_texture->w );
    int y = (int)( player.pos_y / ( (float)map_h ) * minimap_texture->h );
    for ( int r = y - 10; r < y + 10; r++ ) {
      for ( int c = x - 10; c < x + 10; c++ ) {
        int img_idx = ( r * minimap_texture->w + c ) * 3;
        memcpy( &minimap_img_ptr[img_idx], player_col, 3 );
      }
    }
    plot_line( x, y, x + player.dir_x * 20, y + player.dir_y * 20, player_col, minimap_img_ptr, minimap_texture->w, minimap_texture->h, 3 );
  }
  /*
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
    }*/
}

void cast_rays( player_t player, int res_w, int res_h, uint8_t* minimap_img_ptr, texture_t minimap_texture ) {
  const float viewplane_w = 1.0f;
  const float viewplane_d = 0.5f; // Equiv. to near clip plane distance.
  /*     w/2      w/2 (o)
    min--------+-------max
      \        |        /
          \    | d (a)
             \ | /
               p
    angle = tan^-1( (0.5*w) / d ) == (tOa)
          = tan^-1( 0.5 / 0.5 )
          = 45 degrees or 0.7854 radians
  */
  float angle_max       = 0.7854f;
  float angle_per_pixel = angle_max / ( res_w / 2 );

  uint8_t ray_rgb[] = { 0x00, 0xFF, 0x00 };
  for ( int i = 0; i < res_w; i++ ) {
    const int ray_dist = 1000;
    int x              = (int)( player.pos_x / ( (float)map_w ) * minimap_texture.w );
    int y              = (int)( player.pos_y / ( (float)map_h ) * minimap_texture.h );
    int xxx            = i;
    if ( i >= res_w / 2 ) { xxx = res_w / 2 - i; }
    float angle_a = xxx * angle_per_pixel + player.angle;
    float dir_x   = cosf( angle_a );
    float dir_y   = sinf( angle_a );
    plot_line( x, y, x + dir_x * ray_dist, y + dir_y * ray_dist, ray_rgb, minimap_img_ptr, minimap_texture.w, minimap_texture.h, 3 );

    {}

    // TODO
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
  int minimap_res_w = 1024, minimap_res_h = 1024;
  int win_w = 1024, win_h = 768;
  GLFWwindow* window = gfx_start( win_w, win_h, "Antolf 3-D" );
  if ( !window ) { return 1; }

  player_t player = { .pos_x = 1.5f, .pos_y = 3.5f };

  uint8_t* main_img_ptr    = calloc( rt_res_w * rt_res_h * 3, 1 );
  uint8_t* minimap_img_ptr = calloc( minimap_res_w * minimap_res_h * 3, 1 );

  for ( int i = 0; i < minimap_res_w * minimap_res_h * 3; i++ ) { minimap_img_ptr[i] = rand() % 255; }
  texture_t main_texture    = gfx_create_texture_from_mem( rt_res_w, rt_res_h, main_img_ptr );
  texture_t minimap_texture = gfx_create_texture_from_mem( minimap_res_w, minimap_res_h, minimap_img_ptr );

  bool draw_rays    = true;
  bool ray_key_down = false;

  double prev_s = glfwGetTime();
  while ( !glfwWindowShouldClose( window ) ) {
    double curr_s    = glfwGetTime();
    double elapsed_s = curr_s - prev_s;
    prev_s           = curr_s;
    glfwPollEvents();
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_ESCAPE ) ) { break; }
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_C ) ) {
      if ( !ray_key_down ) {
        draw_rays    = !draw_rays;
        ray_key_down = true;
      }
    } else {
      ray_key_down = false;
    }

    move_player( window, &player, elapsed_s );

    raycast( rt_res_w, rt_res_h, main_img_ptr, player );
    gfx_update_texture_from_mem( &main_texture, main_img_ptr );
    draw_minimap( minimap_img_ptr, &minimap_texture, player );
    if ( draw_rays ) { cast_rays( player, rt_res_w, rt_res_h, minimap_img_ptr, minimap_texture ); }
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
