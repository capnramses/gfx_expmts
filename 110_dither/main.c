// Dither experiment
// Anton Gerdelan
// C99, OpenGl
// 26 Oct 2021

#include "gfx.h"
#include "apg_maths.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

// Bayer Matrix for dither pattern
// I created this with GIMP and 4 subdivisions to get more greys in the pattern
#define BAYER_IMAGE_FILENAME "bayer_matrix_8x8.png"

/* Unity's dither node works like this
https://docs.unity3d.com/Packages/com.unity.shadergraph@6.9/manual/Dither-Node.html
so the pixel values could just go directly in a shader without using a texture

void Unity_Dither_float4(float4 In, float4 ScreenPosition, out float4 Out)
{
    float2 uv = ScreenPosition.xy * _ScreenParams.xy;
    float DITHER_THRESHOLDS[16] =
    {
        1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
        13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
        4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
        16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0
    };
    uint index = (uint(uv.x) % 4) * 4 + uint(uv.y) % 4;
    Out = In - DITHER_THRESHOLDS[index];
}
*/

int main( int argc, char** argv ) {
  // enter
  if ( !gfx_start( "Dither Demo", NULL, false ) ) { return 1; }
  gfx_texture_t dither_texture = gfx_texture_create_from_file( BAYER_IMAGE_FILENAME,
    ( gfx_texture_properties_t ){ .bilinear = false, .has_mips = false, .is_array = false, .is_bgr = false, .is_depth = false, .is_srgb = false, .repeats = true } );
  if ( 0 == dither_texture.handle_gl ) { return 1; }

  // main loop
  double prev_time_s = gfx_get_time_s();
  while ( !gfx_should_window_close() ) {
    double curr_time_s    = gfx_get_time_s();
    double elapsed_time_s = curr_time_s - prev_time_s;
    prev_time_s           = curr_time_s;

    gfx_poll_events();
    gfx_clear_colour_and_depth_buffers( GFX_CORNFLOWER_R, GFX_CORNFLOWER_G, GFX_CORNFLOWER_B, 1.0f );

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
