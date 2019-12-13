// Copyright Anton Gerdelan <antonofnote@gmail.com>. 2019

#include "gfx.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

int main() {
  printf( "starting demo\n" );
	glog_start();

  if ( !gfx_start( 1920, 1080, true, "demo", NULL ) ) { return 1; }

	gfx_buffer_colour( NULL, 0.5f, 0.5f, 0.5f, 1.0f );
  while ( !gfx_should_window_close() ) {
		gfx_clear_colour_and_depth_buffers();

		gfx_draw_mesh( gfx_unit_cube_mesh, gfx_fallback_shader );

		gfx_swap_buffer_and_poll_events();
	}

  gfx_stop();
  printf( "exited normally\n" );

  return 0;
}
