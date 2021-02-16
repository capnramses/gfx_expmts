#include "camera.h"
#include "gfx.h"
#include "input.h"
#include <stdio.h>
#include <stdint.h>

/* create a 16x256x16 array of blocks */
#define CHUNK_X 16
#define CHUNK_Y 32
#define CHUNK_Z 16
#define N_VOXELS ( CHUNK_X * CHUNK_Y * CHUNK_Z )
uint8_t voxels[N_VOXELS];
float positions[N_VOXELS * 3];

bool save_vox( const char* filename ) {
  FILE* f_ptr = fopen( filename, "wb" );
  fwrite( voxels, sizeof( uint8_t ), N_VOXELS, f_ptr );
  fclose( f_ptr );
  return true;
}

bool load_vox( const char* filename ) {
  FILE* f_ptr = fopen( filename, "rb" );
  fread( voxels, sizeof( uint8_t ), N_VOXELS, f_ptr );
  fclose( f_ptr );
  return true;
}

vec3 ray_d_wor_from_mouse( mat4 inv_P, mat4 inv_V ) {
  int win_x = 0, win_y = 0;
  gfx_window_dims( &win_x, &win_y );
  vec4 ray_clip = ( vec4 ){ ( 2.0f * input_mouse_x_win ) / win_x - 1.0f, 1.0f - ( 2.0f * input_mouse_y_win ) / win_y, -1.0, 1.0 };
  vec4 ray_eye  = mult_mat4_vec4( inv_P, ray_clip );
  ray_eye       = ( vec4 ){ ray_eye.x, ray_eye.y, -1.0, 0.0 };
  vec4 tmp_wor  = mult_mat4_vec4( inv_V, ray_eye );
  vec3 ray_wor  = normalise_vec3( v3_v4( tmp_wor ) );
  return ray_wor;
}

bool raycast_voxel( vec3 ray_o, vec3 ray_d ) {
  bool hit = ray_aabb( ray_o, ray_d, ( vec3 ){ -0.1, -0.1, -0.1 }, ( vec3 ){ 3.2 - 0.1, 6.4 - 0.1, 3.2 - 0.1 }, 0.1, 100 );
  return hit;
}

int main() {
  gfx_start( "Voxedit2 by Anton Gerdelan\n", NULL, false );
  input_init();

  int fb_w = 0, fb_h = 0;
  gfx_framebuffer_dims( &fb_w, &fb_h );
  camera_t cam = create_cam( (float)fb_w / (float)fb_h );
  cam.pos      = ( vec3 ){ .x = 1.6, .y = 10, .z = 10 };
  recalc_cam_V( &cam );

  gfx_shader_t shader = gfx_create_shader_program_from_files( "instanced.vert", "instanced.frag" );
  if ( shader.program_gl == 0 ) { return 1; }
  gfx_texture_t texture = gfx_texture_create_from_file( "some_file.png", ( gfx_texture_properties_t ){ .bilinear = 0 } );
  gfx_mesh_t mesh       = gfx_mesh_create_from_ply( "unit_cube.ply" );
  if ( mesh.n_vertices == 0 ) { return 1; }
  for ( int y = 0; y < CHUNK_Y; y++ ) {
    for ( int z = 0; z < CHUNK_Z; z++ ) {
      for ( int x = 0; x < CHUNK_X; x++ ) {
        int idx                = y * ( CHUNK_X * CHUNK_Z ) + z * CHUNK_Z + x;
        positions[idx * 3 + 0] = x;
        positions[idx * 3 + 1] = y;
        positions[idx * 3 + 2] = z;
      }
    }
  }
  gfx_buffer_t voxel_positions = gfx_buffer_create( positions, 3, N_VOXELS, true );

  double prev_s = gfx_get_time_s();
  while ( !gfx_should_window_close() ) {
    double curr_s    = gfx_get_time_s();
    double elapsed_s = curr_s - prev_s;
    prev_s           = curr_s;

    gfx_framebuffer_dims( &fb_w, &fb_h );
    gfx_viewport( 0, 0, fb_w, fb_h );
    gfx_clear_colour_and_depth_buffers( 0.2f, 0.2f, 0.2f, 1.0f );

    recalc_cam_P( &cam, (float)fb_w / (float)fb_h );
    mat4 M     = scale_mat4( ( vec3 ){ 0.1, 0.1, 0.1 } );
    mat4 inv_P = inverse_mat4( cam.P );
    mat4 inv_V = inverse_mat4( cam.V );

    /* pop up tile chooser */
    // TODO
    // gfx_draw_textured_quad( texture, ( vec2 ){ 0.5, 0.5 }, ( vec2 ){ 0, 0 }, ( vec2 ){ 1, 1 }, ( vec4 ){ 1, 1, 1, 1 } );

    gfx_uniform4f( &shader, shader.u_tint, 1, 1, 1, 1 );

    /* render a single cube for each, with instancing */
    // TODO
    int n_instances = N_VOXELS;
    gfx_draw_mesh_instanced( &shader, cam.P, cam.V, M, mesh.vao, mesh.n_vertices, &texture, 1, n_instances, &voxel_positions, 1 );

    gfx_swap_buffer();
    input_reset_last_polled_input_states();
    gfx_poll_events();

    /* button to save in either ply or a voxel format (just dims and tile types) */
    if ( input_was_key_pressed( 'S' ) ) {
      printf( "saving...\n" );
      save_vox( "my.vox" );
    }
    /* button to load from voxel format */
    if ( input_was_key_pressed( 'L' ) ) {
      printf( "loading...\n" );
      load_vox( "my.vox" );
      // TODO validate
      // TODO update buffer used for palettes
    }

    if ( input_lmb_clicked() ) {
      vec3 ray_d_wor = ray_d_wor_from_mouse( inv_P, inv_V );
      printf( "ray_d_wor= " );
      print_vec3( ray_d_wor );
      if ( raycast_voxel( cam.pos, ray_d_wor ) ) {
        printf( "HIT\n" );
      } else {
        printf( "MISS\n" );
      }
    }

    /* oct-tree raycast function within chunk. ignore air tiles. */

    // bool ray_aabb( vec3 ray_origin, vec3 ray_direction, vec3 aabb_min, vec3 aabb_max, float tmin, float tmax );
  }

  gfx_delete_shader_program( &shader );
  gfx_delete_mesh( &mesh );
  gfx_delete_texture( &texture );
  gfx_buffer_delete( &voxel_positions );
  gfx_stop();

  printf( "normal exit\n" );
  return 0;
}
