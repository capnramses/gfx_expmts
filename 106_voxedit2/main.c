#include "gfx.h"
#include "input.h"
#include <stdio.h>
#include <stdint.h>

/* create a 16x256x16 array of blocks */
#define N_VOXELS ( 16 * 256 * 16 )
uint8_t voxels[N_VOXELS];

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

int main() {
  gfx_start( "Voxedit2 by Anton Gerdelan\n", NULL, false );

  gfx_texture_t texture = gfx_texture_create_from_file( "some_file.png", ( gfx_texture_properties_t ){ .bilinear = 0 } );

  while ( !gfx_should_window_close() ) {
    int fb_w = 0, fb_h = 0;
    gfx_framebuffer_dims( &fb_w, &fb_h );
    gfx_viewport( 0, 0, fb_w, fb_h );
    gfx_clear_colour_and_depth_buffers( 0.2f, 0.2f, 0.2f, 1.0f );

    gfx_draw_textured_quad( texture, ( vec2 ){ 0.5, 0.5 }, ( vec2 ){ 0, 0 }, ( vec2 ){ 1, 1 }, ( vec4 ){ 1, 1, 1, 1 } );

    gfx_swap_buffer();
    gfx_poll_events();
    
  /* button to save in either ply or a voxel format (just dims and tile types) */

  /* button to load from voxel format */

  }

  /* render a single cube for each, with instancing */

  /* oct-tree raycast function within chunk. ignore air tiles. */
  // bool ray_aabb( vec3 ray_origin, vec3 ray_direction, vec3 aabb_min, vec3 aabb_max, float tmin, float tmax );

  /* pop up tile chooser */

  gfx_stop();

  printf( "normal exit\n" );
  return 0;
}
