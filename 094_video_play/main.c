/*
using plmpeg.
works but only accepts MPEG1-encoded video
convert a video to mpeg1 format like this:
ffmpeg -i sample-mp4-file.mp4 -c:v mpeg1video -c:a mp2 -format mpeg output.mpg
*/

#include "apg_gfx.h"
#include "apg_maths.h"
#define PL_MPEG_IMPLEMENTATION
#include "pl/plmpeg.h"
#include <stdio.h>
#include <stdint.h>

// This function gets called for each decoded video frame
void my_video_callback( plm_t* plm, plm_frame_t* frame, void* user ) {
  // Do something with frame->y.data, frame->cr.data, frame->cb.data
  fprintf( stderr, "got a frame of video\n" );
}

// This function gets called for each decoded audio frame
void my_audio_callback( plm_t* plm, plm_samples_t* frame, void* user ) {
  // Do something with samples->interleaved
  fprintf( stderr, "got a frame of audio\n" );
}

int main( int argc, const char** argv ) {
  printf( "video playback in opengl\n" );
  if ( argc < 2 ) {
    printf( "usage: ./a.out MYFILE.mp4\n" );
    return 0;
  }
  printf( "loading `%s`...\n", argv[1] );

  int w = 0, h = 0, n = 3;
  uint8_t* rgb_buffer = NULL;
  plm_t* plm          = NULL;
  {
    // Load a .mpg (MPEG Program Stream) file
    plm = plm_create_with_filename( argv[1] );
    if ( !plm ) {
      printf( "Couldn't open file" );
      return 1;
    }
    // If you only want to decode video *or* audio through these functions, you should
    plm_set_audio_enabled( plm, FALSE );
    w          = plm_get_width( plm );
    h          = plm_get_height( plm );
    rgb_buffer = (uint8_t*)malloc( w * h * 3 );
  }
  printf( "video loaded w=%i, h=%i\n", w, h );

  char title[1024];
  snprintf( title, 1023, "anton's video demo: %s", argv[1] );
  gfx_start( title, w, h, false );

  vec2 scale = ( vec2 ){ .x = 1, .y = 1 };
  vec2 pos   = ( vec2 ){ .x = 0, .y = 0 };

  gfx_texture_t texture = gfx_create_texture_from_mem( NULL, w, h, n, ( gfx_texture_properties_t ){ .is_srgb = true } );
  // free( image_ptr );

  while ( !gfx_should_window_close() ) {
    int fb_w = 0, fb_h = 0;
    gfx_framebuffer_dims( &fb_w, &fb_h );
    gfx_viewport( 0, 0, fb_w, fb_h );
    gfx_clear_colour_and_depth_buffers( 0.2, 0.2, 0.2, 1.0 );

    plm_frame_t* frame = plm_decode_video( plm );
    if ( frame ) {
      plm_frame_to_rgb( frame, rgb_buffer, w * 3 );
      gfx_update_texture( &texture, rgb_buffer, w, h, n );
    }

    gfx_draw_textured_quad( texture, scale, pos );

    gfx_swap_buffer();
    gfx_poll_events();
  }

  gfx_stop();
  free( rgb_buffer );
  plm_destroy( plm );

  return 0;
}
