
// Use discrete GPU by default. not sure if it works on other OS. if in C++ must be in extern "C" { } block
#ifdef _WIN32
// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
__declspec( dllexport ) unsigned long int NvOptimusEnablement = 0x00000001;
// https://gpuopen.com/amdpowerxpressrequesthighperformance/
__declspec( dllexport ) int AmdPowerXpressRequestHighPerformance = 1;
#endif

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
#include "glcontext.h"
#include "gl_utils.h"
#include "input.h"
#include "voxels.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
  if ( !start_gl( "Voxedit by Anton Gerdelan" ) ) { return 1; }
  init_input();

  uint32_t seed = time( NULL );
  printf( "seed = %u\n", seed );

  chunks_create( seed );

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

  framebuffer_t picking_fb = create_framebuffer( fb_width, fb_height );
  assert( picking_fb.built );

  printf( "displaying...\n" );
  double prev_time  = get_time_s();
  double text_timer = 0.2;
  camera_t cam      = create_cam( (float)fb_width / (float)fb_height );
  {
    cam.pos = ( vec3 ){ .x = 1.6f, .y = 5.0f, .z = 5.0f };
    vec3 up = normalise_vec3( ( vec3 ){ .x = 0.5f, .y = 0.5f, .z = -0.5f } );
    cam.V   = look_at( cam.pos, ( vec3 ){ .x = 0, .y = 0, .z = 0 }, up );
  }

  block_type_t block_type_to_create = BLOCK_TYPE_STONE;
  char hovered_voxel_str[256];
  hovered_voxel_str[0] = '\0';
  int picked_x = -1, picked_y = -1, picked_z = -1, picked_face = -1, picked_chunk_id = -1;
  bool picked = false;

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
      rebuild_framebuffer( &picking_fb, fb_width, fb_height );
      assert( picking_fb.built );
    }

    { // get mouse cursor and controls
      mouse_pos_win( &mouse_x, &mouse_y );

      for ( int i = 1; i <= 4; i++ ) {
        if ( was_key_pressed( g_palette_1_key + i - 1 ) ) {
          printf( "block type set to %i\n", i );
          block_type_to_create = (block_type_t)i;
        }
      }

      if ( picked ) {
        bool changed = false;
        if ( lmb_clicked() ) {
          changed = chunks_create_block_on_face( picked_chunk_id, picked_x, picked_y, picked_z, picked_face, block_type_to_create );
        } else if ( rmb_clicked() ) {
          changed = chunks_set_block_type_in_chunk( picked_chunk_id, picked_x, picked_y, picked_z, BLOCK_TYPE_AIR );
        }
      }
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

    re_extract_frustum_planes( cam.fovy_deg, cam.aspect, cam.neard, cam.fard, cam.pos, cam.V );
    clear_colour_and_depth_buffers( 0.5, 0.5, 0.9, 1.0 );
    viewport( 0, 0, fb_width, fb_height );

    chunks_sort_draw_queue( cam.pos );
    chunks_draw( cam.forward, cam.P, cam.V );
    int chunks_drawn = chunks_get_drawn_count();

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

      sprintf( string, "FPS %.2f\n%s\nwin dims (%i,%i). fb dims (%i,%i)\nmouse xy (%.2f,%.2f)\nhovered voxel: %s\nchunks drawn: %i\nseed: %u", fps, gfx_renderer_str(),
        win_width, win_height, fb_width, fb_height, mouse_x, mouse_y, hovered_voxel_str, chunks_drawn, seed );

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

    { // render colour IDs around mouse cursor
      // subwindow rectangle. NOTE THAT FB BUFFER SHOULD STILL BE ORIGINAL BUFFER SIZE.
      int h            = 3;
      int w            = 3;
      int x            = mouse_x - h / 2;
      int y            = fb_height - ( mouse_y + w / 2 );
      mat4 offcentre_P = perspective_offcentre_viewport( fb_width, fb_height, x, y, w, h, cam.P );

      bind_framebuffer( &picking_fb );
      viewport( x, y, w, h );
      // clear to 1,1,1,1 because 0,0,0,0 is voxel number 0
      clear_colour_and_depth_buffers( 1, 1, 1, 1 );
      // clear_depth_buffer();

      chunks_draw_colour_picking( offcentre_P, cam.V );

      uint8_t data[4] = { 0, 0, 0, 0 };
      // TODO(Anton) this is blocking cpu/gpu sync and can stall. to speed up use the 2-PBO method perhaps.
      read_pixels( x + w / 2, y + h / 2, 1, 1, 4, data );
      picked = chunks_picked_colour_to_voxel_idx( data[0], data[1], data[2], data[3], &picked_x, &picked_y, &picked_z, &picked_face, &picked_chunk_id );
      if ( picked ) {
        sprintf( hovered_voxel_str, "chunk_id=%i voxel=(%i,%i,%i) face=%i", picked_chunk_id, picked_x, picked_y, picked_z, picked_face );
      } else {
        sprintf( hovered_voxel_str, "none" );
      }

      bind_framebuffer( NULL );
    }
    swap_buffer();
  }

  chunks_free();
  free( fps_img_mem );
  stop_gl();
  printf( "HALT\n" );
  return 0;
}
