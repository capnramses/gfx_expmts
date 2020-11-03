/*
 */

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "apg_gfx.h"
#include "apg_maths.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main( int argc, const char** argv ) {
  printf( "gltf + draco in opengl\n" );
  if ( argc < 2 ) {
    printf( "usage: ./a.out MYFILE.gltf\n" );
    return 0;
  }

  cgltf_data* data = NULL;
  {
    printf( "loading `%s`...\n", argv[1] );
    cgltf_options options = { 0 };
    cgltf_result result   = cgltf_parse_file( &options, argv[1], &data );
    if ( result != cgltf_result_success ) {
      fprintf( stderr, "ERROR: loading GLTF file\n" );
      return 1;
    }
    printf( "loaded GLTF model\n" );
  }

  char title[1024];
  snprintf( title, 1023, "anton's gltf demo: %s", argv[1] );
  if ( !gfx_start( title, 1920, 1080, false ) ) {
    fprintf( stderr, "ERROR - starting window\n" );
    return 1;
  }

  vec2 scale = ( vec2 ){ .x = 1, .y = 1 };
  vec2 pos   = ( vec2 ){ .x = 0, .y = 0 };

  uint8_t* rgb_buffer_ptr = NULL;
  int w = 0, h = 0, n = 3;
  gfx_texture_t texture = gfx_create_texture_from_mem( NULL, w, h, n, ( gfx_texture_properties_t ){ .is_srgb = true } );
  // free( image_ptr );

  while ( !gfx_should_window_close() ) {
    int fb_w = 0, fb_h = 0;
    gfx_framebuffer_dims( &fb_w, &fb_h );
    gfx_viewport( 0, 0, fb_w, fb_h );
    gfx_clear_colour_and_depth_buffers( 0.2, 0.2, 0.2, 1.0 );

    //    gfx_draw_textured_quad( texture, scale, pos );

    gfx_swap_buffer();
    gfx_poll_events();
  }

  gfx_stop();
  free( rgb_buffer_ptr );

  cgltf_free( data );
  return 0;
}
