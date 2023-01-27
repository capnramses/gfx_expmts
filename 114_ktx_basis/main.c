// Demo using Basis Universal to on-the-fly transcode a .basis image to native
// / KTX compressed GPU textures, and displaying in OpenGL.
//
// The advantage to this is:
// 1. The .basis file is extremely compressed/small file size.
// 2. Being able to transcode to a device/API's native compressed equivalent
//    quickly makes it much more portable than dragging around a set of image
//    files to suit a variety of devices.
// 3.
//
// The .basis image, `out.basis`, was created with the basisu command-line tool
// from `out.png`. Default basisu settings were used.
// Basis Universal: https://github.com/BinomialLLC/basis_universal
// Basis Licence: Apache 2.0.
//
// Author   : Anton Gerdelan
// Date     : 2023 Jan 18
// Language : C++

#include "apg.h"                                          // Author's personal "anton.h" helper file.
#include "gfx.h"                                          // Author's personal OpenGL helper stuff.
#include "basis_universal/transcoder/basisu_transcoder.h" // Third Party dep.
#include <stdint.h>
#include <stdio.h>

int main( int argc, char** argv ) {
  if ( argc < 2 ) {
    printf( "Usage: %s IMAGE.BASIS\n", argv[0] );
    return 0;
  }

  const char* input_str = argv[1];
  printf( "loading input file `%s`\n", input_str );
  uint8_t* input_ptr = NULL;
  uint32_t input_sz  = 0;
  apg_file_t record;

  if ( !apg_read_entire_file( input_str, &record ) ) { fprintf( stderr, "ERROR: failed to read file `%s`\n", input_str ); }

  if ( !gfx_start( "ktx/basis demo", NULL, false ) ) { return 1; }
  printf( "renderer:\n%s\n", gfx_renderer_str() );

  printf( "starting basis transcoder.\n" );
  basist::basisu_transcoder_init();
  // We need to create a single instance of this class at startup.
  // NB looks like this was removed in latest version as its not supported by ktx anyway.
  // basist::etc1_global_selector_codebook sel_codebook(basist::g_global_selector_cb_size, basist::g_global_selector_cb);

  // Note: Can also use methods from the ktx2_transcoder class here in place of basisu_transcoder.

  // "start_transcoding() must be called before calling transcode_slice() or transcode_image_level().
  // For ETC1S files, this call decompresses the selector/endpoint codebooks, so ideally you would only call this once per .basis file (not each image/mipmap
  // level)."
  basist::basisu_transcoder trans;
  bool r = trans.start_transcoding( record.data_ptr, record.sz );
  if ( !r ) {
    fprintf( stderr, "ERROR transcoding failed\n" );
    return 1;
  }

  uint32_t n_images = trans.get_total_images( record.data_ptr, record.sz );
  printf( "get_total_images() = %u\n", n_images );

  basist::basisu_image_info image_info;
  uint32_t image_index = 0;
  trans.get_image_info( record.data_ptr, record.sz, image_info, image_index );

  basist::basisu_image_level_info level_info;
  uint32_t level_index = 0;
  trans.get_image_level_info( record.data_ptr, record.sz, level_info, image_index, level_index );

  void* pOutput_blocks                                = NULL;
  uint32_t output_blocks_buf_size_in_blocks_or_pixels = 0;
  // TODO alloc and sz
  basist::transcoder_texture_format fmt = cTFBC1_RGB; // TODO try plain RGB

  trans.transcode_image_level( record.data_ptr, record.sz, image_index, level_index, //
    pOutput_blocks, output_blocks_buf_size_in_blocks_or_pixels, fmt );               // NOTE skipped optional args


  up to here


  // Create OpenGL Texture to copy into and start the rendering.
  gfx_texture_t texture = gfx_create_texture_from_mem( NULL, 512, 512, 3, ( gfx_texture_properties_t ){ .is_srgb = true, .has_mips = true, .bilinear = true } );
  while ( !gfx_should_window_close() ) {
    gfx_poll_events();
    gfx_clear_colour_and_depth_buffers( 130 / 255.0f, 163 / 255.0f, 255 / 255.0f, 1.0f );
    int wh, ww;
    gfx_window_dims( &ww, &wh );
    gfx_viewport( 0, 0, ww, wh );

    gfx_draw_textured_quad( texture, ( vec2 ){ 0.75, 0.75 }, ( vec2 ){ 0 }, ( vec2 ){ 1, 1 }, ( vec4 ){ 1, 1, 1, 1 } );

    gfx_swap_buffer();
  }

  gfx_stop();

  return 0;
}
