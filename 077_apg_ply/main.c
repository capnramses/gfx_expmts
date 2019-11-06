/* demo motivations

- DONE create/delete a chunk's worth of voxels with the mouse lmb rmb
- DONE use 1-5 keys to choose a block type
- DONE reset button
- DONE export to ply with correct colours
- DONE export to custom vxl type with palette indices
*/

#include "apg_maths.h"
#include "apg_pixfont.h"
#include "apg_ply.h"
#include "apg_tga.h"
#include "camera.h"
#include "gl_utils.h"
#include "input.h"
#include <assert.h>
#include <stdio.h>
/* TODO(Anton)
- ASan
- AFL
*/

#include <stdlib.h>

int main( int argc, char** argv ) {
  if ( argc < 2 ) {
    printf( "a.exe mymesh.ply\n" );
    return 0;
  }
  if ( !start_gl( "apg_ply by Anton Gerdelan" ) ) { return 1; }
  init_input();

  mesh_t mesh;
  {
    apg_ply_t ply = apg_ply_read( argv[1] );
    if ( !ply.loaded ) {
      fprintf( stderr, "ERROR loading `%s` ply\n", argv[1] );
      return 1;
    }
    printf( "ply loaded from `%s` with %u verts, %u vp comps\n", argv[1], ply.n_vertices, ply.n_positions_comps );
    mesh = create_mesh_from_mem( ply.positions_ptr, ply.n_positions_comps, NULL, 0, ply.colours_ptr, ply.n_colours_comps, ply.n_vertices );
    apg_ply_delete( &ply );
  }
  texture_t text_texture;
  {
    int w, h;
    int n_channels = 1;
    int thickness  = 1;
    bool outlines  = true;
    bool vflip     = false;
    if ( APG_PIXFONT_FAILURE == apg_pixfont_image_size_for_str( "my_string", &w, &h, thickness, outlines ) ) {
      fprintf( stderr, "ERROR apg_pixfont_image_size_for_str\n" );
      return 1;
    }
    unsigned char* img_mem = (unsigned char*)malloc( w * h * n_channels );
    memset( img_mem, 0x00, w * h * n_channels );
    if ( APG_PIXFONT_FAILURE == apg_pixfont_str_into_image( "my_string", img_mem, w, h, n_channels, 0xFF, 0x7F, 0x00, 0xFF, thickness, outlines, vflip ) ) {
      fprintf( stderr, "ERROR apg_pixfont_image_size_for_str\n" );
      return 1;
    }
    text_texture = create_texture_from_mem( img_mem, w, h, n_channels, false, false, false );
    free( img_mem );
  }

  unsigned char* fps_img_mem = NULL;
  int fps_img_w = 256, fps_img_h = 256;
  int fps_n_channels = 4;
  { // allocate memory for FPS image

    fps_img_mem = (unsigned char*)calloc( fps_img_w * fps_img_h * fps_n_channels, 1 );
    if ( !fps_img_mem ) {
      fprintf( stderr, "ERROR: calloc returned NULL\n" );
      return 1;
    }
  }

  int fb_width, fb_height;
  framebuffer_dims( &fb_width, &fb_height );

  printf( "displaying...\n" );
  double prev_time  = get_time_s();
  double text_timer = 0.2;
  camera_t cam      = create_cam( (float)fb_width / (float)fb_height );
  {
    cam.pos = ( vec3 ){ .x = 1.6f, .y = 5.0f, .z = 5.0f };
    vec3 up = normalise_vec3( ( vec3 ){ .x = 0.5f, .y = 0.5f, .z = -0.5f } );
    cam.V   = look_at( cam.pos, ( vec3 ){ .x = 0, .y = 0, .z = 0 }, up );
  }

  vec2 text_scale = ( vec2 ){ .x = 1, .y = 1 }, text_pos = ( vec2 ){ .x = 0 };
  while ( !should_window_close() ) {
    int fb_width = 0, fb_height = 0, win_width = 0, win_height = 0;
    double mouse_x = 0, mouse_y = 0;
    double curr_time = get_time_s();
    double elapsed_s = curr_time - prev_time;
    prev_time        = curr_time;
    text_timer += elapsed_s;

    reset_last_polled_input_states();
    poll_events();

    { // TODO(anton) only do when actually resized
      framebuffer_dims( &fb_width, &fb_height );
      window_dims( &win_width, &win_height );
    }

    { // get mouse cursor and controls
      mouse_pos_win( &mouse_x, &mouse_y );

      if ( was_key_pressed( g_screenshot_key ) ) { screenshot(); }

      bool cam_fwd = false, cam_bk = false, cam_left = false, cam_rgt = false, turn_left = false, turn_right = false;
      {
        static double prev_mouse_x  = 0.0;
        static bool not_first_frame = false;
        if ( not_first_frame && mmb_held() ) {
          const double mouse_turn_sensitivity = 100.0;
          double turn_factor                  = mouse_turn_sensitivity * elapsed_s * fabs( mouse_x - prev_mouse_x ) / fb_width; // scale by resolution
          if ( mouse_x > prev_mouse_x ) {
            turn_cam_right( &cam, turn_factor );
          } else if ( mouse_x < prev_mouse_x ) {
            turn_cam_left( &cam, turn_factor );
          }
        }
        not_first_frame = true;
        prev_mouse_x    = g_mouse_x_win;
      }
      if ( is_key_held( g_forwards_key ) ) { cam_fwd = true; }
      if ( is_key_held( g_backwards_key ) ) { cam_bk = true; }
      if ( is_key_held( g_slide_left_key ) ) { cam_left = true; }
      if ( is_key_held( g_slide_right_key ) ) { cam_rgt = true; }
      if ( is_key_held( g_fly_up_key ) ) {
        move_cam_up( &cam, elapsed_s );
        cam.animating_y = false; // if animating a height change, don't let that fight us
      }
      if ( is_key_held( g_fly_down_key ) ) {
        move_cam_dn( &cam, elapsed_s );
        cam.animating_y = false;
      }
      if ( is_key_held( g_turn_left_key ) ) { turn_left = true; }
      if ( is_key_held( g_turn_right_key ) ) { turn_right = true; }
      if ( is_key_held( g_turn_up_key ) ) { turn_cam_up( &cam, elapsed_s ); }
      if ( is_key_held( g_turn_down_key ) ) { turn_cam_down( &cam, elapsed_s ); }

      if ( cam_fwd ) {
        move_cam_fwd( &cam, elapsed_s );
      } else if ( cam_bk ) {
        move_cam_bk( &cam, elapsed_s );
      }
      if ( cam_left ) {
        move_cam_lft( &cam, elapsed_s );
      } else if ( cam_rgt ) {
        move_cam_rgt( &cam, elapsed_s );
      }
      if ( turn_left ) {
        turn_cam_left( &cam, elapsed_s );
      } else if ( turn_right ) {
        turn_cam_right( &cam, elapsed_s );
      }
      recalc_cam_V( &cam );
    }

    clear_colour_and_depth_buffers( 0.5, 0.5, 0.9, 1.0 );
    viewport( 0, 0, fb_width, fb_height );

    draw_mesh( g_default_shader, cam.P, cam.V, identity_mat4(), mesh.vao, mesh.n_vertices, NULL, 0 );

    // update FPS image every so often
    if ( text_timer > 0.1 ) {
      int w = 0, h = 0; // actual image writing area can be smaller than img dims
      int thickness = 1;
      bool outlines = true;
      bool vflip    = false;
      char string[256];
      if ( elapsed_s == 0.0 ) { elapsed_s = 0.00001; }
      double fps = 1.0 / elapsed_s;

      text_timer = 0.0;
      memset( fps_img_mem, 0x00, fps_img_w * fps_img_h * fps_n_channels );

      sprintf( string, "FPS %.2f\nwin dims (%i,%i). fb dims (%i,%i)\nmouse xy (%.2f,%.2f)", fps, win_width, win_height, fb_width, fb_height, mouse_x, mouse_y );

      if ( APG_PIXFONT_FAILURE == apg_pixfont_image_size_for_str( string, &w, &h, thickness, outlines ) ) {
        fprintf( stderr, "ERROR apg_pixfont_image_size_for_str\n" );
        return 1;
      }
      if ( APG_PIXFONT_FAILURE == apg_pixfont_str_into_image( string, fps_img_mem, w, h, fps_n_channels, 0xFF, 0x7F, 0x00, 0xFF, thickness, outlines, vflip ) ) {
        fprintf( stderr, "ERROR apg_pixfont_image_size_for_str\n" );
        return 1;
      }
      text_texture = create_texture_from_mem( fps_img_mem, w, h, fps_n_channels, false, false, false );
      text_scale.x = (float)w / fb_width * 2.0f;
      text_scale.y = (float)h / fb_height * 2.0f;
      text_pos.x   = -1.0f + text_scale.x;
      text_pos.y   = 1.0f - text_scale.y;
    }
    draw_textured_quad( text_texture, text_scale, text_pos );

    swap_buffer();
  } // endwhile

  free( fps_img_mem );
  stop_gl();
  printf( "HALT\n" );
  return 0;
}
