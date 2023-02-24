// Pixel shader demo.
// by Anton Gerdelan, Feb 2023

#include "gfx.h"
#include <math.h>
#include <stdio.h>

const char* image_filename = "images/3.png";

int main() {
  if ( !gfx_start( "pixelart antialiasing demo", NULL, false ) ) { return 1; }

  gfx_shader_t pixel_shader = gfx_create_shader_program_from_files( "pixel.vert", "pixel.frag" );
  if ( pixel_shader.program_gl == 0 ) { return 1; }
  gfx_texture_t pixel_texture = gfx_texture_create_from_file( image_filename,
    ( gfx_texture_properties_t ){ .bilinear = true, .has_mips = false, .is_array = false, .is_bgr = false, .is_depth = false, .is_srgb = true, .repeats = false } );
  uint32_t u_fb_dims_loc      = gfx_uniform_loc( &pixel_shader, "u_fb_dims" );

  while ( !gfx_should_window_close() ) {
    int ww = 0, wh = 0, fbw = 0, fbh = 0;
    double t = gfx_get_time_s();
    mat4 S   = scale_mat4( ( vec3 ){ .9 + cosf( t ) * 0.1f, .9, .2 } );
    mat4 R   = rot_z_deg_mat4( t * 0.1f );
    mat4 T   = translate_mat4( ( vec3 ){ .x = cosf( t * 0.2f ) * 0.1, .y = 0, .z = sinf( t * 0.1f ) * 0.2f } );
    mat4 M   = mult_mat4_mat4( mult_mat4_mat4( T, R ), S );

    gfx_poll_events();
    gfx_clear_colour_and_depth_buffers( 0.5f, 0.5f, 0.5f, 1.0f );
    gfx_window_dims( &ww, &wh );
    gfx_framebuffer_dims( &fbw, &fbh );
    gfx_viewport( 0, 0, ww, wh );

    // No need for alpha-blending if using a hard discard.
    gfx_alpha_testing( true );
    gfx_uniform2f( &pixel_shader, u_fb_dims_loc, fbw, fbh );
    gfx_draw_mesh( GFX_PT_TRIANGLE_STRIP, &pixel_shader, identity_mat4(), identity_mat4(), M, gfx_ss_quad_mesh.vao, gfx_ss_quad_mesh.n_vertices, &pixel_texture, 1 );
    gfx_alpha_testing( false );

    gfx_swap_buffer();
  }

  gfx_stop();
  return 0;
}
