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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  if ( !start_gl() ) { return 1; }
  mesh_t chunk_mesh;
  mesh_t cube_mesh;

  { // test some ray casts
    vec3 ray_origin    = ( vec3 ){ .x = 0, .y = 0, .z = 10 };
    vec3 ray_direction = normalise_vec3( ( vec3 ){ .x = 0, .y = 0, .z = -1 } );
    vec3 aabb_min      = ( vec3 ){ .x = -1, .y = -1, .z = -1 };
    vec3 aabb_max      = ( vec3 ){ .x = 1, .y = 1, .z = 1 };
    float tmin         = 0.0f;
    float tmax         = 100.0f;
    bool hit           = ray_aabb( ray_origin, ray_direction, aabb_min, aabb_max, tmin, tmax );
    printf( "ray hit = %u\n", (uint32_t)hit );
    float t = 0.0f;
    obb_t obb;
    obb.centre           = ( vec3 ){ .x = 0 };
    obb.norm_side_dir[0] = ( vec3 ){ .x = 1 };
    obb.norm_side_dir[1] = ( vec3 ){ .y = 1 };
    obb.norm_side_dir[2] = ( vec3 ){ .z = 1 };
    obb.half_lengths[0] = obb.half_lengths[1] = obb.half_lengths[2] = 1;
    int slab_i                                                      = -1;
    hit                                                             = ray_obb( obb, ray_origin, ray_direction, &t, &slab_i );
    printf( "ray hit = %u. t=%f slab=%i\n", (uint32_t)hit, t, slab_i );
  }

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

    vec3 mouse_ray_world = ( vec3 ){ .x = 0 };
    { // get mouse cursor ray in world space
      mouse_pos_win( &mouse_x, &mouse_y );
      vec4 ray_clip   = ( vec4 ){ .x = ( 2.0f * mouse_x ) / win_width - 1.0f, .y = 1.0f - ( 2.0f * mouse_y ) / win_height, .z = -1.0, .w = 1.0 };
      vec4 ray_eye    = mult_mat4_vec4( inverse_mat4( cam.P ), ray_clip );
      ray_eye         = ( vec4 ){ .x = ray_eye.x, .y = ray_eye.y, .z = -1.0f, .w = 0.0f };
      vec4 ray_wor    = mult_mat4_vec4( inverse_mat4( cam.V ), ray_eye );
      mouse_ray_world = normalise_vec3( ( vec3 ){ .x = ray_wor.x, .y = ray_wor.y, .z = ray_wor.z } );
    }
    const float min_aabb_cast = 0;
    const float max_aabb_cast = 10;   // limit this to avoid checking any distant chunk voxels
    const float voxel_scale   = 0.1f; // hard-coded in GLSL for now
                                      // NOTE(Anton) why is my scalex2 ? - each block is 2 units wide -1 to 1. first block centre is on 0, edge on +1
    bool ray_hit_chunk = ray_aabb( cam.pos_world, mouse_ray_world, ( vec3 ){ .x = -voxel_scale, .y = -voxel_scale, .z = -voxel_scale },
      ( vec3 ){ .x = ( CHUNK_X - 1 ) * voxel_scale * 2 + voxel_scale, .y = ( CHUNK_Y - 1 ) * voxel_scale * 2 + voxel_scale, .z = ( CHUNK_Z - 1 ) * voxel_scale * 2 + voxel_scale },
      min_aabb_cast, max_aabb_cast );
    int n_rays_cast    = 1;
    // find block within chunk
    ivec3 closest_voxel = ( ivec3 ){ .x = -1, .y = -1, .z = -1 };
    float closest_dist  = 10000.0f; // TODO(Anton) slightly inaccurate on some borders
    bool found_anything = false;
    int face_idx        = -69;
    if ( ray_hit_chunk ) {
      for ( int y = 0; y < CHUNK_Y; y++ ) {
        bool ray_hit_layer = ray_aabb( cam.pos_world, mouse_ray_world, ( vec3 ){ .x = -voxel_scale, .y = y * voxel_scale * 2 - voxel_scale, .z = -voxel_scale },
          ( vec3 ){ .x = ( CHUNK_X - 1 ) * voxel_scale * 2 + voxel_scale, .y = y * voxel_scale * 2 + voxel_scale, .z = ( CHUNK_Z - 1 ) * voxel_scale * 2 + voxel_scale },
          min_aabb_cast, max_aabb_cast );
        n_rays_cast++;
        if ( !ray_hit_layer ) { continue; }

        for ( int z = 0; z < CHUNK_Z; z++ ) {
          for ( int x = 0; x < CHUNK_X; x++ ) {
            block_type_t block_type = 0;
            bool ret                = get_block_type_in_chunk( &chunk_b, x, y, z, &block_type );
            assert( ret );
            if ( BLOCK_TYPE_AIR == block_type ) { continue; }

            bool ray_hit_voxel = false;
            {
              obb_t obb;
              obb.centre           = ( vec3 ){ .x = x * voxel_scale * 2, .y = y * voxel_scale * 2, .z = z * voxel_scale * 2 };
              obb.norm_side_dir[0] = ( vec3 ){ .x = 1 };
              obb.norm_side_dir[1] = ( vec3 ){ .y = 1 };
              obb.norm_side_dir[2] = ( vec3 ){ .z = 1 };
              obb.half_lengths[0] = obb.half_lengths[1] = obb.half_lengths[2] = 0.1f;

              float t       = 0.0f;
              int slab_i    = -69;
              ray_hit_voxel = ray_obb( obb, cam.pos_world, mouse_ray_world, &t, &slab_i );
              n_rays_cast++;
              if ( ray_hit_voxel ) {
                if ( !found_anything || t < closest_dist ) {
                  closest_dist   = t;
                  closest_voxel  = ( ivec3 ){ .x = x, .y = y, .z = z };
                  found_anything = true;
                  face_idx       = slab_i;
                }
              }
            }
            /*
            {
            bool ray_hit_voxel = ray_aabb( cam.pos_world, mouse_ray_world,
            ( vec3 ){ .x = x * voxel_scale * 2 - voxel_scale, .y = y * voxel_scale * 2 - voxel_scale, .z = z * voxel_scale * 2 - voxel_scale },
            ( vec3 ){ .x = x * voxel_scale * 2 + voxel_scale, .y = y * voxel_scale * 2 + voxel_scale, .z = z * voxel_scale * 2 + voxel_scale },
            min_aabb_cast, max_aabb_cast ); n_rays_cast++;
            }
            if ( ray_hit_voxel ) {
              float sqdist1d = length2_vec3( sub_vec3_vec3( ( vec3 ){ .x = x * voxel_scale * 2, .y = y * voxel_scale * 2, .z = z * voxel_scale * 2 },
            cam.pos_world ) ); if ( !found_anything || sqdist1d < closest_dist ) { closest_dist = sqdist1d; closest_voxel      = ( ivec3 ){ .x = x,
            .y = y, .z = z }; found_anything     = true;
              }
            }
            */
          }
        }
      }
    }

    clear_colour_and_depth_buffers();

    draw_mesh( cam, identity_mat4(), chunk_mesh.vao, chunk_mesh.n_vertices, false, 1.0f );
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
        fps, mouse_x, mouse_y, win_width, win_height, fb_width, fb_height, (uint32_t)ray_hit_chunk, mouse_ray_world.x, mouse_ray_world.y, mouse_ray_world.z,
        n_rays_cast, (int)found_anything, closest_voxel.x, closest_voxel.y, closest_voxel.z, face_idx );

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
