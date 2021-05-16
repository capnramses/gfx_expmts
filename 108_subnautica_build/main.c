#include "gfx.h"
#include <stdio.h>
#include <string.h>

int main( int argc, char** argv ) {
  char mesh_filename[256];
  mesh_filename[0] = '\0';
  if ( argc < 2 ) {
    printf( "Usage: %s MYMESH.PLY\n", argv[0] );
    return 0;
  }
  strncat( mesh_filename, argv[1], 255 );
  printf( "Loading mesh: %s\n", mesh_filename );
  gfx_mesh_t mesh = gfx_mesh_create_from_ply( mesh_filename );

  gfx_start( "shader effect", NULL, false );

  while ( !gfx_should_window_close() ) {
    gfx_poll_events();
    gfx_clear_colour_and_depth_buffers( 0.2f, 0.2f, 0.2f, 1.0f );

    gfx_swap_buffer();
  }

  gfx_delete_mesh( &mesh );
  gfx_stop();

  return 0;
}
