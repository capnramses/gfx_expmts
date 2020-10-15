/*
apt install libavcodec-dev (also pulls in libavutils)

*/

#include "apg_gfx.h"
#include "apg_maths.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <stdio.h>
#include <stdint.h>

int main() {
  printf( "video playback with libavcodec\n" );

  gfx_start( "anton's video demo", false );

  vec2 scale = ( vec2 ){ .x = 1, .y = 1 };
  vec2 pos   = ( vec2 ){ .x = 0, .y = 0 };

  int x = 0, y = 0, comp = 0;
  uint8_t* image_ptr    = stbi_load( "anton.jpeg", &x, &y, &comp, 3 );
  gfx_texture_t texture = gfx_create_texture_from_mem( image_ptr, x, y, 3, ( gfx_texture_properties_t ){ .is_srgb = true } );
  free( image_ptr );

  while ( !gfx_should_window_close() ) {
    int w = 0, h = 0;
    gfx_framebuffer_dims( &w, &h );
    gfx_viewport( 0, 0, w, h );
    gfx_clear_colour_and_depth_buffers( 0.2, 0.2, 0.2, 1.0 );

    gfx_draw_textured_quad( texture, scale, pos );

    gfx_swap_buffer();
    gfx_poll_events();
  }

  gfx_stop();

  return 0;
}
