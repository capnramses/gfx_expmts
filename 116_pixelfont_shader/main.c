// Pixel shader demo.
// by Anton Gerdelan, Feb 2023

#include "apg_unicode.h"
#include "gfx.h"
#include "glcontext.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char* image_filename = "atlas.png";

void _draw_mesh_instanced( const gfx_shader_t* shader_ptr, mat4 M, uint32_t vao, size_t n_vertices, const gfx_texture_t* textures_ptr, int n_textures,
  int n_instances, const gfx_buffer_t* instanced_buffer, int n_buffers ) {
  for ( int i = 0; i < n_textures; i++ ) {
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( GL_TEXTURE_2D, textures_ptr[i].handle_gl );
  }
  glUseProgram( shader_ptr->program_gl );
  glProgramUniformMatrix4fv( shader_ptr->program_gl, shader_ptr->u_M, 1, GL_FALSE, M.m );
  glBindVertexArray( vao );

  // bind instanced buffer(s)
  const int divisor = 1; // don't need this yet but could be a buffer param for interesting buffer use in instancing
  for ( int i = 0; i < n_buffers; i++ ) {
    glEnableVertexAttribArray( 7 + i );
    glBindBuffer( GL_ARRAY_BUFFER, instanced_buffer[i].vbo_gl );
    GLsizei stride = instanced_buffer[i].n_components * sizeof( float ); // NOTE(Anton) do we need this for instanced?
    glVertexAttribIPointer( 7 + i, instanced_buffer[i].n_components, GL_INT, stride, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glVertexAttribDivisor( 7 + i, divisor ); // NOTE(Anton) is this affected by bound buffer?
  }

  glDrawArraysInstanced( GL_TRIANGLE_STRIP, 0, (GLsizei)n_vertices, n_instances );

  // disable so we can still call regular non-instanced draws after this with the same VAO
  for ( int i = 0; i < n_buffers; i++ ) { glDisableVertexAttribArray( 7 + i ); }

  glBindVertexArray( 0 );
  glUseProgram( 0 );
  for ( int i = 0; i < n_textures; i++ ) {
    glActiveTexture( GL_TEXTURE0 + i );
    glBindTexture( GL_TEXTURE_2D, 0 );
  }
}

// In pixels.
static int _get_spacing_for_codepoint( uint32_t codepoint ) {
  if ( 'l' == codepoint || '!' == codepoint || '\'' == codepoint || '|' == codepoint || ':' == codepoint ) { return 2; }
  if ( ',' == codepoint || '.' == codepoint || '`' == codepoint || ';' == codepoint ) { return 3; }
  if ( '(' == codepoint || ')' == codepoint || ' ' == codepoint ) { return 4; }
  if ( '{' == codepoint || '}' == codepoint ) { return 5; }
  if ( '&' == codepoint ) { return 7; }
  return 6;
}

// TODO get rid of scale_x,scale_y and replace with aspect ratio and a single scale factor.
// TODO avoid boxes overlapping - make them smaller to avoid artifacts, and adjust scales to suit.

/**
 * @param str_ptr         Address of a null-terminated UTF-8 or ASCII string.
 * @param points_vbo      An existing (previously generated) vertex buffer object for vertex points.
 * @param texcoords_vbo   An existing (previously generated) vertex buffer object for texture coordinates.
 * @param n_verts_ptr     Address of a variable to store the number of vertex points created. Must not be NULL.
 */
void text_to_vbo( const char* str_ptr, GLuint points_vbo, GLuint texcoords_vbo, size_t* n_verts_ptr, float* total_w, float* total_h ) {
  const int atlas_cols = 16, atlas_rows = 16;
  const size_t max_len = 2048;
  // Size of a full glyph box in clip space, before aspect ratio calculation. 2.0 would cover the whole range from -1,1 on x and y on a square viewport.
  const float quad_scale        = 2.0f;
  const float magic_num         = 0.0625f; // TODO !!!! I have no idea if this is correct.
  const float magic_linespacing = 0.0025f; // TODO !!!! relate to scale - try different scales/aspects/etc.
  float at_x = 0.0f, at_y = 0.0f;          // This is starting centre of the top-left glyph box in clip space.
  float max_x = 0.0f, min_y = 0.0f;

  assert( str_ptr && points_vbo && texcoords_vbo && n_verts_ptr );

  size_t n_bytes = strnlen( str_ptr, max_len );
  if ( n_bytes < 1 ) { return; }
  int n_codepoints = apg_utf8_count_cp( str_ptr );
  assert( n_codepoints > 0 );

  float* points_tmp = (float*)malloc( sizeof( float ) * n_codepoints * 12 ); // TODO make this a triangle strip. Remove run-time malloc, use a max string length.
  float* texcoords_tmp = (float*)malloc( sizeof( float ) * n_codepoints * 12 ); // TODO make this a triangle strip. Remove run-time malloc, use a max string length.

  const float line_start_x = at_x;
  for ( int i = 0, curr_byte = 0; curr_byte < n_bytes; i++, curr_byte++ ) {
    int bytes_in_codepoint = 0;
    uint32_t codepoint     = apg_utf8_to_cp( str_ptr + curr_byte, &bytes_in_codepoint );
    assert( bytes_in_codepoint > 0 );
    curr_byte += ( bytes_in_codepoint - 1 );
    assert( codepoint < atlas_cols * atlas_rows );

    int atlas_col = codepoint % atlas_cols;
    int atlas_row = codepoint / atlas_cols;

    float s     = atlas_col * ( 1.0f / atlas_cols );
    float t     = ( atlas_row + 1 ) * ( 1.0f / atlas_rows ); // TODO why is this row + 1 - because texturing up from bottom?
    float x_pos = at_x;
    float y_pos = at_y;

    // TODO check spacing scaling.
    int spacing    = _get_spacing_for_codepoint( codepoint );
    float extent_x = spacing * magic_num * quad_scale;
    float extent_y = quad_scale;

    // Move along for start of next glypg.
    float gap_between_quads = 0.1f;                                   // 0.1 of a glyph image pixel
    at_x += ( spacing + gap_between_quads ) * magic_num * quad_scale; // Move next glyph along to the end of this one.
    if ( '\n' == codepoint ) {
      at_x = line_start_x;
      at_y -= ( quad_scale + magic_linespacing );
      n_codepoints--; // Don't include vertices/accounting for linebreak.
      i--;
    } else if ( ' ' == codepoint || '\r' == codepoint ) { // TODO all other control symbols here.
      n_codepoints--;                                     // Don't include vertices/accounting for space.
      i--;
    } else {
      max_x = x_pos + extent_x > max_x ? x_pos + extent_x : max_x;
      min_y = y_pos - extent_y < min_y ? y_pos - extent_y : min_y;

      // Add 6 points and texture coordinates to buffers for each glyph.
      points_tmp[i * 12]     = x_pos;
      points_tmp[i * 12 + 1] = y_pos;
      points_tmp[i * 12 + 2] = x_pos;
      points_tmp[i * 12 + 3] = y_pos - extent_y;
      points_tmp[i * 12 + 4] = x_pos + extent_x;
      points_tmp[i * 12 + 5] = y_pos - extent_y;

      points_tmp[i * 12 + 6]  = x_pos + extent_x;
      points_tmp[i * 12 + 7]  = y_pos - extent_y;
      points_tmp[i * 12 + 8]  = x_pos + extent_x;
      points_tmp[i * 12 + 9]  = y_pos;
      points_tmp[i * 12 + 10] = x_pos;
      points_tmp[i * 12 + 11] = y_pos;

      texcoords_tmp[i * 12]     = s;
      texcoords_tmp[i * 12 + 1] = 1.0 - t + 1.0 / atlas_rows;
      texcoords_tmp[i * 12 + 2] = s;
      texcoords_tmp[i * 12 + 3] = 1.0 - t;
      texcoords_tmp[i * 12 + 4] = s + 1.0 / atlas_cols * spacing * magic_num;
      texcoords_tmp[i * 12 + 5] = 1.0 - t;

      texcoords_tmp[i * 12 + 6]  = s + 1.0 / atlas_cols * spacing * magic_num;
      texcoords_tmp[i * 12 + 7]  = 1.0 - t;
      texcoords_tmp[i * 12 + 8]  = s + 1.0 / atlas_cols * spacing * magic_num;
      texcoords_tmp[i * 12 + 9]  = 1.0 - t + 1.0 / atlas_rows;
      texcoords_tmp[i * 12 + 10] = s;
      texcoords_tmp[i * 12 + 11] = 1.0 - t + 1.0 / atlas_rows;
    }
  }

  glBindBuffer( GL_ARRAY_BUFFER, points_vbo );
  glBufferData( GL_ARRAY_BUFFER, n_codepoints * 12 * sizeof( float ), points_tmp, GL_DYNAMIC_DRAW ); // TODO make triangle strip
  glBindBuffer( GL_ARRAY_BUFFER, texcoords_vbo );
  glBufferData( GL_ARRAY_BUFFER, n_codepoints * 12 * sizeof( float ), texcoords_tmp, GL_DYNAMIC_DRAW ); // TODO make triangle strip
  glBindBuffer( GL_ARRAY_BUFFER, 0 );

  free( points_tmp );
  free( texcoords_tmp );

  *n_verts_ptr = n_codepoints * 6; // TODO make triangle strip
  *total_w     = max_x;
  *total_h     = -min_y;
}

int main( int argc, char** argv ) {
  const char* default_str = "\"'Hello', `World`!\"\náéíóú\n{12345}\n(09876)\n{HellÓ}\n&is the&biggest& char\n";
  const char* user_str    = default_str;

  // Allow user-specified string.
  if ( argc < 2 ) {
    printf( "Usage: %s SOME_USER_STRING\nUsing default string.\n", argv[0] );
  } else {
    user_str = argv[1];
  }

  // General graphics prep.
  if ( !gfx_start( "bitmap font antialiasing demo", NULL, false ) ) { return 1; }
  gfx_shader_t pixel_shader = gfx_create_shader_program_from_files( "pixel_text.vert", "pixel_text.frag" );
  if ( pixel_shader.program_gl == 0 ) { return 1; }
  gfx_texture_t pixel_texture = gfx_texture_create_from_file( image_filename, ( gfx_texture_properties_t ){ .bilinear = true, .is_srgb = true } );
  uint32_t u_fb_dims_loc      = gfx_uniform_loc( &pixel_shader, "u_fb_dims" );
  uint32_t u_atlas_index_loc  = gfx_uniform_loc( &pixel_shader, "u_atlas_index" );

  // Create the quads with atlas-looked texture coords.
  gfx_mesh_t mesh = gfx_mesh_create_empty( true, false, false, true, false, false, false, false, true );
  float total_x = 0.0f, total_y = 0.0f;

  // TODO - at_x, at_y for position should be a uniform. this should always use 0,0 for top-left, otherwise it's a nightmare to align with aspect ratio.
  text_to_vbo( user_str, mesh.points_vbo, mesh.texcoords_vbo, &mesh.n_vertices, &total_x, &total_y );

  printf( "\nn_verts = %u\n", (uint32_t)mesh.n_vertices );

  { // Do a little dance to link shader attributes to vertex buffer memory.
    glBindVertexArray( mesh.vao );
    glBindBuffer( GL_ARRAY_BUFFER, mesh.points_vbo );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, mesh.texcoords_vbo );
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, NULL );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );
  }

  while ( !gfx_should_window_close() ) {
    int ww = 0, wh = 0, fbw = 0, fbh = 0;
    gfx_framebuffer_dims( &fbw, &fbh );
    double t = gfx_get_time_s();

    if ( fbw != 0 && fbh != 0 ) {
      mat4 S = scale_mat4( ( vec3 ){ .9 + cosf( t ) * 0.1f, .9, .2 } );
      mat4 R = rot_z_deg_mat4( t * 0.5f );
      mat4 T = translate_mat4( ( vec3 ){ .x = cosf( t * 0.2f ) * 0.1, .y = 0, .z = sinf( t * 0.1f ) * 0.2f } );
      mat4 M = mult_mat4_mat4( mult_mat4_mat4( T, R ), S );

      gfx_poll_events();
      gfx_clear_colour_and_depth_buffers( 0.5f, 0.5f, 0.5f, 1.0f );
      gfx_window_dims( &ww, &wh );
      gfx_viewport( 0, 0, ww, wh );

      // No need for alpha-blending if using a hard discard.
      gfx_alpha_testing( true );
      gfx_depth_testing( false );
      gfx_uniform2f( &pixel_shader, u_fb_dims_loc, fbw, fbh );
      gfx_uniform4f( &pixel_shader, pixel_shader.u_tint, fmod( t, 2.0 ), fmod( t, 5.0 ), fmod( t, 3.0 ), 1.0f );

      float text_scale = 16.0f / fbh * 8.123f; // 8.123x the actual pixel size of the font.
      gfx_uniform2f( &pixel_shader, pixel_shader.u_scale, text_scale, text_scale );

      // To centre the text:
      bool centring = true;
      if ( centring ) {
        gfx_uniform2f( &pixel_shader, pixel_shader.u_pos, -total_x, total_y );
      } else {
        gfx_uniform2f( &pixel_shader, pixel_shader.u_pos, 0.0f, 0.0f );
      }
      gfx_draw_mesh( GFX_PT_TRIANGLES, &pixel_shader, identity_mat4(), identity_mat4(), M, mesh.vao, mesh.n_vertices, &pixel_texture, 1 );

      gfx_alpha_testing( false );
    }

    gfx_swap_buffer();
  }

  gfx_stop();
  return 0;
}
