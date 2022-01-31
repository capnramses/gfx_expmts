/*
Decals as per FoGED Ch 10.1
Anton Gerdelan - 31 Jan 2022

Decals here are different to older projected textures or 'splat' decals:
- Decals have a bounding box.
- Decals are meshes created based on geometry they intersect with.
- This is determined based on clipping of geometry with the decal's BB.

Algorithm:
A) create 6 planes of decal's BB
2. inputs are centre position of box P, and normal vector N.
3. choose a tangent vector T to be perp to N (e.g. Listing 9.10).
   tangent can have random rotation (varied splats) or aligned to something (e.g. for footprints).
4. complete right-handed coordinate system B = NxT. Normalise all 3 vectors if necessary.
5. decide on decal's BB height +-d. Make large enough to absorb bumps in surface, but not so big
   that decal will be applied to distant surfaces unintentionally.
6. determine 6 inward-facing planes
   r_x and r_y are radius of decal on x,y.
   f_left   =  [  T | r_x - T dot P ]    f_right =  [ -T | r_x + T dot P ]
   f_bottom =  [  B | r_y - B dot P ]    f_top   =  [ -B | r_y + B dot P ]
   f_back   =  [  N | d   - N dot P ]    f_front =  [ -N | d   + N dot P ]
B) build the mesh for the decal.
1. search the world for other geometries that intersect with BB
2. for each geometry: clip its triangles against all 6 planes of BB using method described in Sec 9.1
   - transform BB into object space of the geometry being intersected.
   - discard clipped triangles
   - remaining triangles form decal
   - using CONSISTENT vertex ordering so maintain exact edge coordinates for new vertices and avoid seams/gaps.
   -
*/

#include "gfx.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define FOOTPRINT_IMG "footprint.png"

static void _create_decal( vec3 ray_origin, vec3 ray_direction ) {
  // TODO intersection test and mesh creation here
}

int main() {
  gfx_start( "Decals Demo", NULL, false );
  gfx_texture_t footprint_tex = gfx_texture_create_from_file( FOOTPRINT_IMG, ( gfx_texture_properties_t ){ .is_srgb = true, .has_mips = true, .bilinear = true } );

  // TODO create scene mesh here

  // TODO load decal shader and scene shader

  while ( !gfx_should_window_close() ) {
    int fb_w = 0, fb_h = 0;
    gfx_poll_events();

    // TODO -- camera freelook/move here

    gfx_framebuffer_dims( &fb_w, &fb_h );
    if ( 0 == fb_w || 0 == fb_h ) { continue; }
    gfx_viewport( 0, 0, fb_w, fb_h );
    gfx_clear_colour_and_depth_buffers( 0.2, 0.2, 0.2, 1.0 );

    // TODO -- render scene here

    // TODO -- render list of decals here

    gfx_swap_buffer();
  }

  gfx_delete_texture( &footprint_tex );
  gfx_stop();

  printf( "normal exit\n" );
  return 0;
}
