// Dither experiment
// Anton Gerdelan
// C99, OpenGl
// 26 Oct 2021

#include "gfx.h"
#include "apg_maths.h"
#include <stdint.h>
#include <stdio.h>

// Bayer Matrix for dither pattern
// I created this with GIMP and 4 subdivisions to get more greys in the pattern
#define BAYER_IMAGE_FILENAME "bayer_matrix_8x8.png"

int main( int argc, char** argv ) {
  // enter
  if ( !gfx_start( "Dither Demo", NULL, false ) ) { return 1; }
  gfx_texture_t dither_texture = gfx_texture_create_from_file( BAYER_IMAGE_FILENAME,
    ( gfx_texture_properties_t ){ .bilinear = false, .has_mips = false, .is_array = false, .is_bgr = false, .is_depth = false, .is_srgb = false, .repeats = true } );
  if ( 0 == dither_texture.handle_gl ) { return 1; }

  //
  //
  //
  while ( !gfx_should_window_close() ) {
    gfx_poll_events();
    gfx_clear_colour_and_depth_buffers( 100 / 255.0f, 149 / 255.0f, 237 / 255.0f, 1.0f ); // sRGB cornflower blue

    vec2 scale          = ( vec2 ){ 1.0f, 1.0f };
    vec2 pos            = ( vec2 ){ 0.0f, 0.0f };
    vec2 texcoord_scale = ( vec2 ){ 1.0f, 1.0f };
    vec4 tint_rgba      = ( vec4 ){ 1.0f, 1.0f, 1.0f, 1.0f };
    gfx_draw_textured_quad( dither_texture, scale, pos, texcoord_scale, tint_rgba );

    gfx_swap_buffer();
  }

  // exit
  gfx_stop();
  printf( "Normal exit\n" );

  return 0;
}
