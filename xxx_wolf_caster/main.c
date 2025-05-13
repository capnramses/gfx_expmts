/**
 *
 * References:
 * Game Engine Black Book - Wolfenstein 3D, Fabien Sanglard.
 * https://lodev.org/cgtutor/raycasting.html
 * https://www.youtube.com/watch?v=gYRrGTC7GtA
 *
 */
#include "gfx.h"
#include "maths.h"
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct player_t {
  vec2_t pos, dir;
  float angle;
} player_t;

const int map_w     = 8;
const int map_h     = 8;
const uint8_t map[] = {
  1, 1, 1, 1, 1, 1, 1, 1, // 0
  1, 0, 1, 0, 1, 0, 0, 1, // 1
  2, 0, 1, 0, 1, 0, 0, 1, // 2
  2, 0, 0, 0, 0, 0, 0, 1, // 3
  3, 0, 0, 0, 1, 1, 1, 1, // 4
  3, 0, 0, 1, 1, 0, 0, 1, // 5
  1, 1, 0, 0, 0, 0, 0, 1, // 6
  1, 1, 1, 1, 1, 1, 1, 1  // 7
};

void draw_main_col( int i, int height, rgb_t rgb, uint8_t* main_img_ptr, int w, int h ) {
  height         = APG_CLAMP( height, 0, h );
  int half_hdiff = ( h - height ) / 2;
  int draw_start = half_hdiff, draw_end = h - half_hdiff;
  for ( int y = draw_start; y < draw_end; y++ ) {
    int img_idx = ( y * w + i ) * 3;
    memcpy( &main_img_ptr[img_idx], &rgb.r, 3 );
  }
}

void reset_main_image( uint8_t* main_img_ptr, int w, int h ) {
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
#if 0
  // Which box of the map we're in. Assumes boxes are 1.0 wide.
  int map_x = (int)player.pos.x;
  int map_y = (int)player.pos.y;

  float dir_x = player.dir.x, dir_y = player.dir.y;
  float plane_x = 0.0f, plane_y = 0.0f;

  for ( int x = 0; x < w; x++ ) {
    float angle_max       = 0.7854f;
    float angle_per_pixel = angle_max / ( w / 2 );
    const int ray_dist    = 1000;
    int xxx               = x;
    if ( x >= w / 2 ) { xxx = w / 2 - x; }
    float angle_a  = xxx * angle_per_pixel + player.angle;
    vec2_t ray_dir = { cosf( angle_a ), sinf( angle_a ) };

    // float camera_x  = 2 * x / (float)w - 1; // x-coordinate in camera space
    // float ray_dir.x = dir.x + plane_x * camera_x;
    //  float ray_dir.y = dir.y + plane_y * camera_x;

    // Length of ray from current position to next x or y-side.
    float side_dist_x;
    float side_dist_y;

    // Length of ray from one x or y-side to next x or y-side.
    float delta_dist_x = ( ray_dir.x == 0 ) ? FLT_MAX : fabsf( 1 / ray_dir.x );
    float delta_dist_y = ( ray_dir.y == 0 ) ? FLT_MAX : fabsf( 1 / ray_dir.y );
    float perpWallDist;

    // what direction to step in x or y-direction (either +1 or -1)
    int stepX;
    int stepY;

    int hit = 0; // was there a wall hit?
    int side;    // was a NS or a EW wall hit?

    // calculate step and initial sideDist
    if ( ray_dir.x < 0 ) {
      stepX       = -1;
      side_dist_x = ( player.pos.x - map_x ) * delta_dist_x;
    } else {
      stepX       = 1;
      side_dist_x = ( map_x + 1.0 - player.pos.x ) * delta_dist_x;
    }
    if ( ray_dir.y < 0 ) {
      stepY       = -1;
      side_dist_y = ( player.pos.y - map_y ) * delta_dist_y;
    } else {
      stepY       = 1;
      side_dist_y = ( map_y + 1.0 - player.pos.y ) * delta_dist_y;
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
#endif
}

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
      if ( 0 != map[map_idx] ) { memcpy( colour, wall_col, 3 ); }
      for ( int r = 1; r < wall_w; r++ ) {
        for ( int c = 1; c < wall_w; c++ ) {
          int img_idx = ( y * wall_w * minimap_texture->h + x * wall_w + r * minimap_texture->h + c ) * 3;
          memcpy( &minimap_img_ptr[img_idx], colour, 3 );
        }
      }
    }
  }
  { // draw player
    int x = (int)( player.pos.x / ( (float)map_w ) * minimap_texture->w );
    int y = (int)( player.pos.y / ( (float)map_h ) * minimap_texture->h );
    for ( int r = y - 10; r < y + 10; r++ ) {
      for ( int c = x - 10; c < x + 10; c++ ) {
        int img_idx = ( r * minimap_texture->w + c ) * 3;
        memcpy( &minimap_img_ptr[img_idx], player_col, 3 );
      }
    }
    plot_line( x, y, x + player.dir.x * 20, y + player.dir.y * 20, player_col, minimap_img_ptr, minimap_texture->w, minimap_texture->h, 3 );
  }
}

void cast_rays( player_t player, uint8_t* minimap_img_ptr, texture_t minimap_texture, uint8_t* main_img_ptr, texture_t main_texture ) {
  /*
           viewplane_width
         |-------------------|

    +    min--------+-------max  --> viewplane_dir
    |      \        |        /
    |        \      |      /
    |          \    |    /
    |           \   |   /
    |             \ | /
    +               +
viewplane_dist      player_pos


    angle = tan^-1( (0.5*w) / d ) == (tOa)
          = tan^-1( 0.5 / 0.5 )
          = 45 degrees or 0.7854 radians
  */

  const float viewplane_width = 1.0f, viewplane_dist = 0.5f; // Equiv. to near clip plane distance.
  vec2_t viewplane_mid_pos = add_vec2( player.pos, mul_vec2_f( player.dir, viewplane_dist ) );
  vec2_t viewplane_dir     = rot_90_ccw_vec2( player.dir );
  vec2_t viewplane_min_pos = sub_vec2( viewplane_mid_pos, mul_vec2_f( viewplane_dir, viewplane_width / 2 ) );
  vec2_t viewplane_max_pos = add_vec2( viewplane_min_pos, mul_vec2_f( viewplane_dir, viewplane_width ) );

  uint8_t ray_rgb[]       = { 0x00, 0xFF, 0x00 };
  uint8_t viewplane_rgb[] = { 0x66, 0x66, 0xFF };

  int xi = (int)( player.pos.x / ( (float)map_w ) * minimap_texture.w );
  int yi = (int)( player.pos.y / ( (float)map_h ) * minimap_texture.h );
  int xf = (int)( viewplane_min_pos.x / ( (float)map_w ) * minimap_texture.w );
  int yf = (int)( viewplane_min_pos.y / ( (float)map_h ) * minimap_texture.h );
  plot_line( xi, yi, xf, yf, viewplane_rgb, minimap_img_ptr, minimap_texture.w, minimap_texture.h, 3 );
  xf = (int)( viewplane_max_pos.x / ( (float)map_w ) * minimap_texture.w );
  yf = (int)( viewplane_max_pos.y / ( (float)map_h ) * minimap_texture.h );
  plot_line( xi, yi, xf, yf, viewplane_rgb, minimap_img_ptr, minimap_texture.w, minimap_texture.h, 3 );
  xi = (int)( viewplane_min_pos.x / ( (float)map_w ) * minimap_texture.w );
  yi = (int)( viewplane_min_pos.y / ( (float)map_h ) * minimap_texture.h );
  xf = (int)( viewplane_max_pos.x / ( (float)map_w ) * minimap_texture.w );
  yf = (int)( viewplane_max_pos.y / ( (float)map_h ) * minimap_texture.h );
  plot_line( xi, yi, xf, yf, viewplane_rgb, minimap_img_ptr, minimap_texture.w, minimap_texture.h, 3 );

  for ( int i = 0; i < main_texture.w; i++ ) {
    float fraction           = (float)i / main_texture.w;
    vec2_t viewplane_inc_pos = add_vec2( viewplane_min_pos, mul_vec2_f( viewplane_dir, fraction ) );
    vec2_t ray_dir           = sub_vec2( viewplane_inc_pos, player.pos );
    ray_dir                  = normalise_vec2( ray_dir ); // Needed?

    int player_map_x = (int)player.pos.x; // Just truncate to go from world->map because map cell width 1 is 1.0 real.
    int player_map_y = (int)player.pos.y;
    if ( 0 == i ) { printf( "player map %i,%i\n", player_map_x, player_map_y ); }

    // 1. get dist to x and y sides by using remainder after truncation or floor().
    // 2. divide x by x gradient of ray_dir, same for y to get side_dists.
    //    biggest component or shortest side dist is first side hit
    // We already know which is pos ray dir so can ignore 2 distances here.

    float dist_to_cell_x = ( player.pos.x - floorf( player.pos.x ) );
    float dist_to_cell_y = ( player.pos.y - floorf( player.pos.y ) );
    if ( ray_dir.x > 0 ) { dist_to_cell_x = 1.0f - dist_to_cell_x; }
    if ( ray_dir.y > 0 ) { dist_to_cell_y = 1.0f - dist_to_cell_y; }
    if ( 0 == i ) { printf( "dist to cell x,y %.2f,%.2f\n", dist_to_cell_x, dist_to_cell_y ); }

    if ( 0 == i ) {
      xi = (int)( player.pos.x / ( (float)map_w ) * minimap_texture.w );
      yi = (int)( player.pos.y / ( (float)map_h ) * minimap_texture.h );

      // plot ray dir so its less confusing
      xf = (int)( ( player.pos.x + ray_dir.x ) / ( (float)map_w ) * minimap_texture.w );
      yf = (int)( ( player.pos.y + ray_dir.y ) / ( (float)map_w ) * minimap_texture.w );
      plot_line( xi, yi, xf, yf, ray_rgb, minimap_img_ptr, minimap_texture.w, minimap_texture.h, 3 );

      if ( ray_dir.x > 0 ) {
        xf = (int)( ( player.pos.x + dist_to_cell_x ) / ( (float)map_w ) * minimap_texture.w );
      } else {
        xf = (int)( ( player.pos.x - dist_to_cell_x ) / ( (float)map_w ) * minimap_texture.w );
      }
      yf = (int)( ( player.pos.y ) / ( (float)map_w ) * minimap_texture.w );
      // if ( dist_to_cell_x <= dist_to_cell_y) { ray_rgb[0] = 0xFF;} else { ray_rgb[0] = 0x00;} // Note <= so starting angle chooses this.
      plot_line( xi, yi, xf, yf, ray_rgb, minimap_img_ptr, minimap_texture.w, minimap_texture.h, 3 );
      if ( ray_dir.y > 0 ) {
        yf = (int)( ( player.pos.y + dist_to_cell_y ) / ( (float)map_w ) * minimap_texture.w );
      } else {
        yf = (int)( ( player.pos.y - dist_to_cell_y ) / ( (float)map_w ) * minimap_texture.w );
      }
      xf = (int)( ( player.pos.x ) / ( (float)map_w ) * minimap_texture.w );
      // if ( dist_to_cell_y < dist_to_cell_x) { ray_rgb[0] = 0xFF;} else { ray_rgb[0] = 0x00;}
      plot_line( xi, yi, xf, yf, ray_rgb, minimap_img_ptr, minimap_texture.w, minimap_texture.h, 3 );
    }

    float xs_per_y = ray_dir.x / ray_dir.y;
    float ys_per_x = ray_dir.y / ray_dir.x;
    float x_dash   = dist_to_cell_y * xs_per_y;
    float y_dash   = dist_to_cell_x * ys_per_x;
    vec2_t isect_x, isect_y;
    float hypot_x, hypot_y;
    { // x hit first
      hypot_x       = sqrtf( y_dash * y_dash + dist_to_cell_x * dist_to_cell_x );
      float reverse = ray_dir.x > 0.0f ? -1.0f : 1.0f;

      isect_x = add_vec2( player.pos, mul_vec2_f( ray_dir, hypot_x ) );

      if ( 0 == i ) {
        uint8_t isect_rgb[] = { 0x00, 0xFF, 0xFF };
        xi                  = (int)( ( player.pos.x - dist_to_cell_x * reverse ) / ( (float)map_w ) * minimap_texture.w );
        yi                  = (int)( ( player.pos.y ) / ( (float)map_h ) * minimap_texture.h );
        xf                  = (int)( ( isect_x.x ) / ( (float)map_w ) * minimap_texture.w );
        yf                  = (int)( ( isect_x.y ) / ( (float)map_w ) * minimap_texture.w );
        plot_line( xi, yi, xf, yf, isect_rgb, minimap_img_ptr, minimap_texture.w, minimap_texture.h, 3 );
      }
    }
    { // y hit first
      hypot_y       = sqrtf( x_dash * x_dash + dist_to_cell_y * dist_to_cell_y );
      float reverse = ray_dir.y > 0.0f ? -1.0f : 1.0f;

      isect_y = add_vec2( player.pos, mul_vec2_f( ray_dir, hypot_y ) );

      if ( 0 == i ) {
        uint8_t isect_rgb[] = { 0x00, 0xFF, 0xFF };
        xi                  = (int)( ( player.pos.x ) / ( (float)map_w ) * minimap_texture.w );
        yi                  = (int)( ( player.pos.y - dist_to_cell_y * reverse ) / ( (float)map_h ) * minimap_texture.h );
        xf                  = (int)( ( isect_y.x ) / ( (float)map_w ) * minimap_texture.w );
        yf                  = (int)( ( isect_y.y ) / ( (float)map_w ) * minimap_texture.w );
        plot_line( xi, yi, xf, yf, isect_rgb, minimap_img_ptr, minimap_texture.w, minimap_texture.h, 3 );
      }
      //  float corner_angle = atanf( x_dash / dist_to_cell_y );
      // if ( 0 == i ) { printf( "corner_angle=%.2f\n", corner_angle ); }
    }
    int imap_x, imap_y, side_dist;
    float direct_dist, perspective_corrected_dist;
    if ( fabsf( hypot_x ) < fabsf( hypot_y ) ) {
      direct_dist = hypot_x;
      imap_x      = (int)isect_x.x + ray_dir.x * 0.000001f;
      imap_y      = (int)isect_x.y;
      float dx    = isect_x.x - player.pos.x; // TODO move to a global single 'intersectxy' so that this can be called in a function continuously.
      float dy    = isect_x.y - player.pos.y;
      perspective_corrected_dist = dx * cosf( player.angle ) - dy * sin( player.angle ); // This is a bit of trig. See Black Book.
      if ( 0 == i ) {
        printf( "xintersect %i,%i\n", imap_x, imap_y );
        printf( "dwd %.2f pcwd %.2f\n", direct_dist, perspective_corrected_dist );
      }
    } else {
      direct_dist                = hypot_y;
      imap_x                     = (int)isect_y.x;
      imap_y                     = (int)isect_y.y + ray_dir.y * 0.000001f;
      float dx                   = isect_y.x - player.pos.x;
      float dy                   = isect_y.y - player.pos.y;
      perspective_corrected_dist = dx * cosf( player.angle ) - dy * sin( player.angle );
      if ( 0 == i ) {
        printf( "yintersect %i,%i\n", imap_x, imap_y );
        printf( "dwd %.2f pcwd %.2f\n", direct_dist, perspective_corrected_dist );
      }
    }
    int imap_idx  = imap_y * map_w + imap_x;
    int tile_type = map[imap_idx];
    rgb_t colour  = { 0xFF };
    switch ( tile_type ) {
    case 0: break;
    case 1: colour = (rgb_t){ 0x11, 0x22, 0x88 }; break; // blue
    case 2: colour = (rgb_t){ 0x11, 0x88, 0x22 }; break; // green
    case 3: colour = (rgb_t){ 0x88, 0x22, 0x11 }; break; // red
    case 4: colour = (rgb_t){ 0x88, 0x88, 0x88 }; break; // whit
    default: colour = (rgb_t){ .r = 0xFF, .g = 0x00, .b = 0xFF };
    } // endswitch

    int line_height = (int)( (main_texture.h) / (fabsf(perspective_corrected_dist)) );

    if ( 0 != tile_type ) { draw_main_col( i, line_height, colour, main_img_ptr, main_texture.w, main_texture.h ); }
    if ( 0 == i ) { printf( "tile type =%i line_height=%i/%i\n", tile_type, line_height, main_texture.h ); }
  } // endfor res_w
}

void move_player( GLFWwindow* window, player_t* p_ptr, double elapsed_s ) {
  const float speed = 1.0f, rot_speed = 2.0f;
  if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_LEFT ) ) { p_ptr->angle -= rot_speed * elapsed_s; }
  if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_RIGHT ) ) { p_ptr->angle += rot_speed * elapsed_s; }
  p_ptr->angle = fmodf( p_ptr->angle, 2.0 * PI );
  p_ptr->angle = p_ptr->angle >= 0.0f ? p_ptr->angle : p_ptr->angle + 2.0 * PI;
  p_ptr->dir.x = cosf( p_ptr->angle );
  p_ptr->dir.y = sinf( p_ptr->angle );
  if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_UP ) ) {
    p_ptr->pos.x += p_ptr->dir.x * speed * elapsed_s;
    p_ptr->pos.y += p_ptr->dir.y * speed * elapsed_s;
  }
  if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_DOWN ) ) {
    p_ptr->pos.x -= p_ptr->dir.x * speed * elapsed_s;
    p_ptr->pos.y -= p_ptr->dir.y * speed * elapsed_s;
  }
}

int main() {
  int rt_res_w = 320, rt_res_h = 200;
  int minimap_res_w = 512, minimap_res_h = 512;
  int win_w = 1024, win_h = 1024;
  GLFWwindow* window = gfx_start( win_w, win_h, "Antolf 3-D" );
  if ( !window ) { return 1; }

  player_t player = { .pos = { 1.5f, 3.5f } };

  uint8_t* main_img_ptr    = calloc( rt_res_w * rt_res_h * 3, 1 );
  uint8_t* minimap_img_ptr = calloc( minimap_res_w * minimap_res_h * 3, 1 );

  for ( int i = 0; i < minimap_res_w * minimap_res_h * 3; i++ ) { minimap_img_ptr[i] = rand() % 255; }
  texture_t main_texture    = gfx_create_texture_from_mem( rt_res_w, rt_res_h, main_img_ptr );
  texture_t minimap_texture = gfx_create_texture_from_mem( minimap_res_w, minimap_res_h, minimap_img_ptr );

  bool draw_rays    = true;
  bool ray_key_down = false;

  double prev_s                   = glfwGetTime();
  double timer_update_countdown_s = 0.2;
  while ( !glfwWindowShouldClose( window ) ) {
    double curr_s    = glfwGetTime();
    double elapsed_s = curr_s - prev_s;
    prev_s           = curr_s;
    timer_update_countdown_s -= elapsed_s;
    if ( timer_update_countdown_s <= 0.0 && elapsed_s > 0.0 ) {
      timer_update_countdown_s = 0.2;
      char title[256];
      float fps = 1.0 / elapsed_s;
      sprintf( title, "%.2f FPS", fps );
      glfwSetWindowTitle( window, title );
    }
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

    reset_main_image( main_img_ptr, rt_res_w, rt_res_h );
    draw_minimap( minimap_img_ptr, &minimap_texture, player );
    cast_rays( player, minimap_img_ptr, minimap_texture, main_img_ptr, main_texture );
    gfx_update_texture_from_mem( &minimap_texture, minimap_img_ptr );
    gfx_update_texture_from_mem( &main_texture, main_img_ptr );

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
