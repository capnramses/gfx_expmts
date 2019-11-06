/* demo motivations

1. NOT DONE sparse chunks (no storing of air-filled layers etc)
2. DONE better generation code
3. multi-chunk: threaded generation of chunk vertex data (but not opengl stuff)
4. DONE loading and saving to disk
5. multi-chunk: 'infinite' world
6. better random testbed
7. DONE try using whole-voxel colours from a palette instead of textures
8. DONE export to Ply
9. DONE maintain heightmap
*/

#include "apg_maths.h"
#include "apg_pixfont.h"
#include "apg_ply.h"
#include "gl_utils.h"
#include "voxels.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  if ( !start_gl() ) { return 1; }
  mesh_t chunk_mesh;
  mesh_t cube_mesh;

  { // test cube creation from faces
    chunk_vertex_data_t data = _test_cube_from_faces();

    apg_ply_write( "test_cube.ply",
      ( apg_ply_t ){
        .n_positions_comps = 3, .n_colours_comps = 3, .positions_ptr = data.positions_ptr, .colours_ptr = data.colours_ptr, .n_vertices = data.n_vertices, .loaded = true } );
    printf( "cube mesh %ix%ix%i has %u verts, %u triangles\n%u bytes\n", CHUNK_X, CHUNK_Y, CHUNK_Z, (uint32_t)data.n_vertices, (uint32_t)data.n_vertices / 3,
      (uint32_t)data.sz * 2 );
    cube_mesh = create_mesh_from_mem( data.positions_ptr, data.n_vp_comps, data.colours_ptr, data.n_vc_comps, data.n_vertices );

    chunk_free_vertex_data( &data );
  }

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
  { // load
    chunk_t chunk_b;
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

    chunk_free( &chunk_b );
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

  printf( "displaying...\n" );
  double prev_time  = get_time_s();
  double text_timer = 0.2;
  while ( !should_window_close() ) {
    double curr_time    = get_time_s();
    double elapsed_time = curr_time - prev_time;
    prev_time           = curr_time;
    text_timer += elapsed_time;

    clear_colour_and_depth_buffers();

    draw_mesh( chunk_mesh.vao, chunk_mesh.n_vertices );

    if ( text_timer > 0.1 ) {
      if ( elapsed_time == 0.0 ) { elapsed_time = 0.00001; }
      text_timer = 0.0;
      double fps = 1.0 / elapsed_time;
      int w, h;
      int n_channels = 4;
      int thickness  = 1;
      bool outlines  = true;
      bool vflip     = false;
      char string[256];
      sprintf( string, "FPS %.2f", fps );

      if ( APG_PIXFONT_FAILURE == apg_pixfont_image_size_for_str( string, &w, &h, thickness, outlines ) ) {
        fprintf( stderr, "ERROR apg_pixfont_image_size_for_str\n" );
        return 1;
      }
      unsigned char* img_mem = (unsigned char*)calloc( w * h * n_channels, 1 );
      memset( img_mem, 0x00, w * h * n_channels );
      if ( APG_PIXFONT_FAILURE == apg_pixfont_str_into_image( string, img_mem, w, h, n_channels, 0xFF, 0x7F, 0x00, 0xFF, thickness, outlines, vflip ) ) {
        fprintf( stderr, "ERROR apg_pixfont_image_size_for_str\n" );
        return 1;
      }
      text_texture = create_texture_from_mem( img_mem, w, h, n_channels, false, false );
      free( img_mem );
    }
    draw_textured_quad( text_texture );

    swap_buffer_and_poll_events();
  }

  delete_mesh( &cube_mesh );
  delete_mesh( &chunk_mesh );
  stop_gl();
  printf( "HALT\n" );
  return 0;
}
