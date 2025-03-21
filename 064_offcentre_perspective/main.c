/* demo motivations

- move the mouse cursor over to the right-ish area of the screen to reveal coloured subwindow
- stdout shows what pixel colour is under middle of cursor subwindow
- clear only depth buffer to show that it exactly matches original perspective. slightly modify values to skew matrix to show it works and would shimmer if wrong
- uncomment bits to send to separate buffer
- NOTE THAT FB BUFFER SHOULD STILL BE ORIGINAL BUFFER SIZE.

TODO
- regen picking buffer if main fb resizes so it doesnt mismatch
*/

#include "apg_maths.h"
#include "apg_pixfont.h"
#include "apg_ply.h"
#include "gl_utils.h"
#include "voxels.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  if ( !start_gl() ) { return 1; }
  mesh_t chunk_mesh;
  mesh_t cube_mesh;
  { // generate and save
    chunk_t chunk = chunk_generate();
    bool result   = chunk_save_to_file( "out.chunk", &chunk );
    if ( !result ) {
      fprintf( stderr, "ERROR: writing chunk\n" );
      return 1;
    }
    result = chunk_write_heightmap( "first.tga", &chunk );
    if ( !result ) {
      fprintf( stderr, "ERROR: writing first.tga\n" );
      return 1;
    }
    chunk_free( &chunk );
  }
  chunk_t chunk_b = ( chunk_t ){ .n_non_air_voxels = 0 };
  { // load
    bool result = chunk_load_from_file( "out.chunk", &chunk_b );
    if ( !result ) {
      fprintf( stderr, "ERROR: loading chunk\n" );
      return 1;
    }
    result = chunk_write_heightmap( "second.tga", &chunk_b );
    if ( !result ) {
      fprintf( stderr, "ERROR: writing second.tga\n" );
      return 1;
    }
    {
      chunk_vertex_data_t vertex_data = chunk_gen_vertex_data( &chunk_b );
      printf( "voxel mesh %ix%ix%i has %u verts, %u triangles\n%u bytes\n", CHUNK_X, CHUNK_Y, CHUNK_Z, (uint32_t)vertex_data.n_vertices,
        (uint32_t)vertex_data.n_vertices / 3, (uint32_t)vertex_data.sz * 2 );
      chunk_mesh = create_mesh_from_mem( vertex_data.positions_ptr, vertex_data.n_vp_comps, vertex_data.colours_ptr, vertex_data.n_vc_comps, vertex_data.n_vertices );
      apg_ply_write( "out.ply", ( apg_ply_t ){ .n_positions_comps = 3,
                                  .n_colours_comps                = 3,
                                  .positions_ptr                  = vertex_data.positions_ptr,
                                  .colours_ptr                    = vertex_data.colours_ptr,
                                  .n_vertices                     = vertex_data.n_vertices,
                                  .loaded                         = true } );
      chunk_free_vertex_data( &vertex_data );
    }
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
    text_texture = create_texture_from_mem( img_mem, w, h, n_channels, false, false );
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
  framebuffer_t picking_fb;
  picking_fb.width  = fb_width;
  picking_fb.height = fb_height;

  bool ret = create_framebuffer( &picking_fb );
  assert( ret );

  printf( "displaying...\n" );
  double prev_time  = get_time_s();
  double text_timer = 0.2;
  camera_t cam      = ( camera_t ){ .V = identity_mat4(), .P = identity_mat4(), .pos_world = ( vec3 ){ .x = -5.0f, .y = 5.0f, .z = 5.0f } };
  vec3 up           = normalise_vec3( ( vec3 ){ .x = 0.5f, .y = 0.5f, .z = -0.5f } );
  cam.V             = look_at( cam.pos_world, ( vec3 ){ .x = 0, .y = 0, .z = 0 }, up );

  vec2 text_scale = ( vec2 ){ .x = 1, .y = 1 }, text_pos = ( vec2 ){ .x = 0 };
  while ( !should_window_close() ) {
    int fb_width = 0, fb_height = 0, win_width = 0, win_height = 0;
    double mouse_x = 0, mouse_y = 0;
    double curr_time    = get_time_s();
    double elapsed_time = curr_time - prev_time;
    prev_time           = curr_time;
    text_timer += elapsed_time;

    framebuffer_dims( &fb_width, &fb_height );
    window_dims( &win_width, &win_height );

    { // update camera TODO(Anton) only if FB changed
      float aspect = (float)fb_width / (float)fb_height;
      cam.P        = perspective( 67.0f, aspect, 0.1f, 1000.0f );
    }

    { // get mouse cursor
      mouse_pos_win( &mouse_x, &mouse_y );
    }

    clear_colour_and_depth_buffers( 0.5, 0.5, 0.9, 1.0 );
    viewport( 0, 0, fb_width, fb_height );

    draw_mesh( cam, identity_mat4(), chunk_mesh.vao, chunk_mesh.n_vertices, false, 1.0f );

    const float voxel_scale = 0.1f;
    int face_idx            = 0;
    ivec3 closest_voxel     = ( ivec3 ){ .x = -1 };
    bool found_anything     = false;
    if ( found_anything ) {
      clear_depth_buffer();
      mat4 M = translate_mat4( ( vec3 ){ .x = closest_voxel.x * voxel_scale * 2, .y = closest_voxel.y * voxel_scale * 2, .z = closest_voxel.z * voxel_scale * 2 } );
      draw_mesh( cam, M, cube_mesh.vao, cube_mesh.n_vertices, true, 0.5f );
    }

    // update FPS image every so often
    if ( text_timer > 0.1 ) {
      int w = 0, h = 0; // actual image writing area can be smaller than img dims
      int thickness = 1;
      bool outlines = true;
      bool vflip    = false;
      char string[256];
      if ( elapsed_time == 0.0 ) { elapsed_time = 0.00001; }
      double fps = 1.0 / elapsed_time;

      text_timer = 0.0;
      memset( fps_img_mem, 0x00, fps_img_w * fps_img_h * fps_n_channels );

      sprintf( string,
        "FPS %.2f\nmouse xy (%.2f,%.2f)\nwin dims (%i,%i)\nfb dims (%i,%i)\nray hit chunk = %u\nmouse ray (%.2f,%.2f,%.2f)\nrays cast %i\nfound anything "
        "%i\nvoxel (%i,%i,%i)\nface=%i",
        fps, mouse_x, mouse_y, win_width, win_height, fb_width, fb_height, (int)found_anything, closest_voxel.x, closest_voxel.y, closest_voxel.z, face_idx );

      if ( APG_PIXFONT_FAILURE == apg_pixfont_image_size_for_str( string, &w, &h, thickness, outlines ) ) {
        fprintf( stderr, "ERROR apg_pixfont_image_size_for_str\n" );
        return 1;
      }
      if ( APG_PIXFONT_FAILURE == apg_pixfont_str_into_image( string, fps_img_mem, w, h, fps_n_channels, 0xFF, 0x7F, 0x00, 0xFF, thickness, outlines, vflip ) ) {
        fprintf( stderr, "ERROR apg_pixfont_image_size_for_str\n" );
        return 1;
      }
      text_texture = create_texture_from_mem( fps_img_mem, w, h, fps_n_channels, false, false );
      text_scale.x = (float)w / fb_width * 2.0f;
      text_scale.y = (float)h / fb_height * 2.0f;
      text_pos.x   = -1.0f + text_scale.x;
      text_pos.y   = 1.0f - text_scale.y;
    }
    draw_textured_quad( text_texture, text_scale, text_pos );

    { // render colour IDs around mouse cursor
      camera_t offcentre_cam = cam;
      // subwindow rectangle. NOTE THAT FB BUFFER SHOULD STILL BE ORIGINAL BUFFER SIZE.
      int h = 64;
      int w = 64;
      int x                  = mouse_x - h / 2;
      int y                  = fb_height - ( mouse_y + w / 2 );
      offcentre_cam.P        = perspective_offcentre_viewport( fb_width, fb_height, x, y, w, h, cam.P );

    //  bind_framebuffer( &picking_fb );
      viewport( x, y, w, h );
      clear_colour_and_depth_buffers( 0, 0, 0, 1 );
      // clear_depth_buffer();

      draw_mesh( offcentre_cam, identity_mat4(), chunk_mesh.vao, chunk_mesh.n_vertices, false, 1.0f );

      uint8_t data[4] = { 0, 0, 0, 0 };
      read_pixels( x + w / 2, y + h / 2, 1, 1, 4, data );
      printf( "mouse RGBA {%x %x %x %x}\n", data[0], data[1], data[2], data[3] );

      bind_framebuffer( NULL );
    }
    swap_buffer_and_poll_events();
  }

  chunk_free( &chunk_b );
  free( fps_img_mem );
  delete_mesh( &cube_mesh );
  delete_mesh( &chunk_mesh );
  stop_gl();
  printf( "HALT\n" );
  return 0;
}
