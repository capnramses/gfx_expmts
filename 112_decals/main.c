/*
Decals as per FoGED Ch 10.1
Anton Gerdelan - 31 Jan 2022

Decals here are different to projected texture decals or 'splat' decals:
- Decals have a bounding box.
- Decals are meshes, created based on geometry they intersect with.
- This is determined based on clipping of geometry with the decal's BB.

My take-away is this is:
  PRO - Slightly more flexible than just using an instanced list of quads because it can
	      appear to project a (larger) decal over more complex surfaces and on different angles.
  CON - Creating a lot of small, unique meshes may not be ideal in some run-time scenarios,
	      but they could be baked into a larger batched mesh if created as part of scene editing.
		  - I'm not sure I'd use this for generic small decals.

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

A similar technique is described here http://blog.wolfire.com/2009/06/how-to-project-decals/
Doom Eternal appears to use a different technique https://simoncoenen.com/blog/programming/graphics/DoomEternalStudy.html
*/

#include "gfx.h"
#include "apg_maths.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define FOOTPRINT_IMG "footprint.png"

typedef struct plane_t {
  vec3 n;
  float d;
} plane_t;

// "right" here is used to orient the decals
// could randomise decal rotation something like this:
//   versor decal_q = quat_from_axis_deg( ( rand() % 360 ) / 360.0f, ray_direction );
//   vec3 t         = mult_quat_vec3( decal_q, ray_up );
static void _create_decal( vec3 ray_direction, vec3 right, vec3 decal_centre ) {
  // TODO intersection test and mesh creation here

  // bounding box dimensions
  float r_x = 1.0f, r_y = 1.0f; // "radius" of decal == horiz/vert dims of bounding box in b and t directions.
  float d = 0.1f;               // half-height 'd' of bounding box
  // work out orthogonal alignment axes for oriented bounding box
  vec3 ray_up = cross_vec3( right, ray_direction );
  vec3 t      = ray_up; // fixed decal orientation (footprints etc).
  vec3 n      = ray_direction;
  vec3 b      = cross_vec3( n, t );
  vec3 p      = decal_centre;
  // work out oriented bounding box planes
  plane_t f_left   = ( plane_t ){ .n = t, .d = r_x - dot_vec3( t, p ) };
  plane_t f_right  = ( plane_t ){ .n = ( vec3 ){ .x = -t.x, .y = -t.y, .z = -t.z }, .d = r_x + dot_vec3( t, p ) };
  plane_t f_bottom = ( plane_t ){ .n = b, .d = r_y - dot_vec3( b, p ) };
  plane_t f_top    = ( plane_t ){ .n = ( vec3 ){ .x = -b.x, .y = -b.y, .z = -b.z }, .d = r_y + dot_vec3( b, p ) };
  plane_t f_back   = ( plane_t ){ .n = n, .d = d - dot_vec3( n, p ) };
  plane_t f_front  = ( plane_t ){ .n = ( vec3 ){ .x = -n.x, .y = -n.y, .z = -n.z }, .d = d + dot_vec3( n, p ) };
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
