/*
apt install libavcodec-dev (also pulls in libavutils)

*/

#include "apg_gfx.h"
#include <stdio.h>

int main() {
  printf( "video playback with libavcodec\n" );

  gfx_start( "anton's video demo", false );

  while ( !gfx_should_window_close() ) {
    int w = 0, h = 0;
    gfx_framebuffer_dims( &w, &h );
    gfx_viewport( 0, 0, w, h );
    gfx_clear_colour_and_depth_buffers( 0.2, 0.2, 0.2, 1.0 );

    gfx_swap_buffer();
    gfx_poll_events();
  }

  gfx_stop();

  return 0;
}
