// Dither experiment
// Anton Gerdelan
// C99, OpenGl
// 26 Oct 2021

/* The basic idea is
- have a dither pattern you can apply to any rendered 3d mesh
- the dither pattern is applied in _screen space_ - so dither isn't a texture on the mesh but flat on the screen with a 1:1 pattern-pixel:screen-pixel.
*/

#include "gfx.h"
#include "apg_maths.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define DITHER_VS "dither.vert"
#define DITHER_FS "dither.frag"

int main( int argc, char** argv ) {
  // enter
  if ( !gfx_start( "Dither Demo", NULL, false ) ) { return 1; }
  gfx_shader_t dither_shader = gfx_create_shader_program_from_files( DITHER_VS, DITHER_FS );
  if ( !dither_shader.loaded ) { return 1; }

  // main loop
  double prev_time_s = gfx_get_time_s();
  while ( !gfx_should_window_close() ) {
    double curr_time_s    = gfx_get_time_s();
    double elapsed_time_s = curr_time_s - prev_time_s;
    prev_time_s           = curr_time_s;

    gfx_poll_events();

    int win_w = 0, win_h = 0, fb_w = 0, fb_h = 0;
    gfx_window_dims( &win_w, &win_h );
    gfx_framebuffer_dims( &fb_w, &fb_h );
    gfx_viewport( 0, 0, win_w, win_h );
    gfx_clear_colour_and_depth_buffers( GFX_CORNFLOWER_R, GFX_CORNFLOWER_G, GFX_CORNFLOWER_B, 1.0f );

		// just some scale animation to show that the dither doesn't also scale like a texture - it's a screen-space thing here
    float s = 0.8f + 0.2f * fabs( cosf( curr_time_s ) );
    mat4 S  = scale_mat4( ( vec3 ){ s, s, s } );
		mat4 M = S; // identity_mat4();

    gfx_alpha_testing( true );
    gfx_uniform2f( &dither_shader, dither_shader.u_screen_dims, fb_w, fb_h );
		gfx_uniform1f( &dither_shader, dither_shader.u_time, s );
    gfx_draw_mesh( GFX_PT_TRIANGLE_STRIP, &dither_shader, identity_mat4(), identity_mat4(), M, gfx_ss_quad_mesh.vao, gfx_ss_quad_mesh.n_vertices, NULL, 0 );
    gfx_alpha_testing( false );

    gfx_swap_buffer();
  }

  // exit
	gfx_delete_shader_program( &dither_shader );
  gfx_stop();
  printf( "Normal exit\n" );

  return 0;
}
