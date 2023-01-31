// Demo using Basis Universal to on-the-fly transcode a .basis image to native
// / KTX compressed GPU textures, and displaying in OpenGL.
//
// The advantage to this is:
// 1. The .basis file is extremely compressed/small file size.
// 2. Being able to transcode to a device/API's native compressed equivalent
//    quickly makes it much more portable than dragging around a set of image
//    files to suit a variety of devices.
//
// The .basis image, `out.basis`, was created with the basisu command-line tool
// from `out.png`. Default basisu settings were used.
// Basis Universal: https://github.com/BinomialLLC/basis_universal
// Basis Licence: Apache 2.0.
//
// Author   : Anton Gerdelan
// Date     : 2023 Jan 31
// Language : C++

/**
 *
 * Compiling
 * -------------
 * ./build.sh or build.bat
 *
 * Running
 * --------------
 * ./a.out out.basis (or some other .basis file).
 * (the windows .exe has a different name because my laptop forces "a.exe" to run on the software driver.)
 *
 * Thoughts after getting it working:
 * ------------------------------------
 *
 *               | .basis | .ktx  | .dds  | .jpeg |   .png |
 * --------------------------------------------------------
 *  file compr   |  great | great | great | great | ok     |
 *  file size    |  34553 |  ?    |   ?   | 37746 | 203307 |
 *  GPU compr    |   yes  | yes   | yes   | no    | no     |
 *  load time    |  4ms   |   ?   |    ?  |  8ms  | 10ms   |
 *  multi-device |   yes  |    no |    no |   yes |    yes |
 *
 * The S3TC compression isn't great for hand-crafted art, and compression is pointless for small images.
 * For PBR high-res or photo-realistic images pre-compressed textures are really useful, particularly for over-the-web and mobile devices.
 * Basis transcoder runs pretty fast compared to decompressing a jpeg, has similar file size, is easy to integrate (drop-in code vs library),
 * and can transcode to suit the device without needing to prepare device-specific compressed files first.
 * It's basically a 'the same as loading a jpeg and making a compressed texture, but faster'. More portable than .dds textures and .ktx2 alone.
 *
 * 1. This is pretty similar to using DDS textures with OpenGL.
 *    glCompressedTexImage2D() -> input is pre-compressed texture data, like we are using here, or from a DDS file.
 *    glTexImage2D()           -> input is raw RGB image data, but it can be used to create compressed textures. (see /010_compressed_textures/).
 *
 * 2. I just had a trivial 1-level mip here. For most files there will be >1 mipmap level. These can be iterated over fairly simply to set up textures.
 * 3. The key API function is transcode_image_level(). This is pretty fiddly to use, and would benefit from a set of examples.
 *    - For different devices (mobile, web) we would change the compressed texture format here.
 *    - Use the table to match basis compressed formats with OpenGL compressed formats.
 *      Catch: compressed format numbers don't match DXT numbers.
 *    - I never figured out the correct block size allocation required there.
 *      Catch: It crashes if it's too small. I just gave it full pixels x*y. This may not be optimal.
 * 4. Basis prefers power-of-two image sizes for some formats, but seems to do a decent job of resizing.
 *    My original 400x400 out.png renders a nicer image than my 512x512 out_po2.png image. Not sure why!
 * 5. Depsite some compressed textures not supporting alpha channels, the transcoder handles it well (run with out.basis for an example).
 * 6. I think you could probably use a faster jpg/png loader to rival the transcoding time of .basis, but if you also want to compress the textures - add some
 *    more time. The real advantage here is the device portability of the compressed approach by using the on-the-fly transcoding.
 * 7. If transcoding speed is an issue you can also use the CLI basis tool to create device-specific images in a variety of formats, ready-to-go, which is also
 *    a win.
 */

#include "apg.h" // Author's personal "anton.h" helper file.
#include "gfx.h" // Author's personal OpenGL helper stuff.
#include "glcontext.h"
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

  // A nice guide to using basis is actually for Metal
  // https://metalbyexample.com/basis-universal/

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

  // tells if texture is 2d, 3d, array, cube, etc.
  basist::basis_texture_type basisTextureType = trans.get_texture_type( record.data_ptr, record.sz );

  // for cubemaps expect a multiple of 6. for arrays this is the number of slices.
  uint32_t n_images = trans.get_total_images( record.data_ptr, record.sz );
  printf( "get_total_images() = %u\n", n_images );

  basist::basisu_image_info image_info;
  uint32_t image_index = 0;
  trans.get_image_info( record.data_ptr, record.sz, image_info, image_index );

  // We need to iterate over mipmap levels in each image and transcode level-by-level.

  printf( "# levels = %i\n", image_info.m_total_levels );

  basist::basisu_image_level_info level_info;
  uint32_t level_index = 0;
  trans.get_image_level_info( record.data_ptr, record.sz, level_info, image_index, level_index );

  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  // VERY IMPORTANT! These need to match!
  // https://github.com/BinomialLLC/basis_universal/wiki/OpenGL-texture-format-enums-table
  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  basist::transcoder_texture_format fmt = basist::transcoder_texture_format::cTFBC3_RGBA;
  GLenum internal_format                = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; // NOTE DXt5 is for cTFBC3_RGBA!!!
  GLenum base_internal_format           = GL_RGBA;
  int tex_w                             = level_info.m_width;
  int tex_h                             = level_info.m_height;
  //////////////////////////////////////////////////////////////////////////////////////////////////////////

  if ( !trans.get_ready_to_transcode() ) {
    fprintf( stderr, "ERROR not ready to transcode.\n" );
    return 1;
  }

  // output blocks is 'level data' and level size in blocks.
  uint32_t output_blocks_buf_size_in_blocks_or_pixels = tex_w * tex_h; // using total blocks here, as indicated in the api doc, segfaults. *shrug*
  uint8_t* pOutput_blocks                             = (uint8_t*)malloc( output_blocks_buf_size_in_blocks_or_pixels );

  double trans_i = gfx_get_time_s();

  r = trans.transcode_image_level( record.data_ptr, record.sz, image_index, level_index, pOutput_blocks, output_blocks_buf_size_in_blocks_or_pixels, fmt );
  if ( !r ) {
    fprintf( stderr, "ERROR transcoding image level failed.\n" );
    return 1;
  }

  double trans_f = gfx_get_time_s();

  printf( "transcoded in %lf ms...\n", ( trans_f - trans_i ) * 1000.0 );

  double tex_i = gfx_get_time_s();
  gfx_texture_t compressed_texture;
  {
    GLuint tex_cmpr1 = 0;
    glGenTextures( 1, &tex_cmpr1 );
    glBindTexture( GL_TEXTURE_2D, tex_cmpr1 );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

    // Instead of glTexuImage2D - which takes raw RGB data and _converts_ into compressed formats.
    /*
    glTexImage2D(           //
      GL_TEXTURE_2D,        // As fetched from get_texture_type()
      level_index,                    // Mipmap level
      internal_format,      //
      tex_w, tex_h,         // width, height
      0,                    // border
      base_internal_format, // format
      GL_UNSIGNED_BYTE,     // type
      pOutput_blocks        // data
    );*/
    // Use https://docs.gl/gl4/glCompressedTexImage2D which takes compressed data.

    printf( "tex_w = %i tex_h = %i\n", tex_w, tex_h );
    glCompressedTexImage2D( GL_TEXTURE_2D, level_index, internal_format, tex_w, tex_h, 0, output_blocks_buf_size_in_blocks_or_pixels, pOutput_blocks );

    glBindTexture( GL_TEXTURE_2D, 0 );

    compressed_texture.w          = level_info.m_width; // maybe if it's not Po2 it mangles
    compressed_texture.h          = level_info.m_height;
    compressed_texture.handle_gl  = tex_cmpr1;
    compressed_texture.n_channels = 4;
    gfx_texture_properties_t props;
    props.bilinear                = false;
    props.has_mips                = false;
    props.is_array                = false;
    props.is_bgr                  = false;
    props.is_depth                = false;
    props.is_srgb                 = false;
    props.repeats                 = false;
    compressed_texture.properties = props;
  }
  glFinish();
  glFlush();
  double tex_f = gfx_get_time_s();
  printf( "texture uploaded in %lf ms\n", ( tex_f - tex_i ) * 1000.0 );

  { /* For comparison load a .jpg or .png */
    double comp_i      = gfx_get_time_s();
    gfx_texture_t comp = gfx_texture_create_from_file( "out.jpg", ( gfx_texture_properties_t ){ .bilinear = false } );
    glFinish();
    glFlush();
    double comp_f = gfx_get_time_s();
    printf( "comparison PNG texture decompressed and uploaded in %lf ms\n", ( comp_f - comp_i ) * 1000.0 );
  }

  // Create OpenGL Texture to copy into and start the rendering.
  while ( !gfx_should_window_close() ) {
    gfx_poll_events();
    gfx_clear_colour_and_depth_buffers( 130 / 255.0f, 163 / 255.0f, 255 / 255.0f, 1.0f );
    int wh, ww;
    gfx_window_dims( &ww, &wh );
    gfx_viewport( 0, 0, ww, wh );

    gfx_alpha_testing( true );
    gfx_draw_textured_quad( compressed_texture, ( vec2 ){ 0.75, 0.75 }, ( vec2 ){ 0 }, ( vec2 ){ 1, 1 }, ( vec4 ){ 1, 1, 1, 1 } );
    gfx_alpha_testing( false );

    gfx_swap_buffer();
  }

  gfx_stop();

  free( pOutput_blocks );

  return 0;
}
