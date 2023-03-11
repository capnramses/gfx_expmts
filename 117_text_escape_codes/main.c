// A Demo Using ANSII Escape Sequences in Strings to Set Text Rendering Properties.
// by Anton Gerdelan.   Mar 2023.   antongerdelan.net

#include "apg_unicode.h"
#include "gfx.h"
#include "glcontext.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const int thickness = 1; // HACK
const int outline   = 1; // HACK

const char* image_filenames[] = { "atlas.png", "atlas_bold.png", "atlas_italic.png", "atlas_underline.png", "atlas_strikethrough.png" };

// TODO replace with look-up tables from each atlas.
static int _get_spacing_for_codepoint_px( uint32_t codepoint ) {
  if ( 'l' == codepoint || '!' == codepoint || '\'' == codepoint || '|' == codepoint || ':' == codepoint ) { return 2; }
  if ( ',' == codepoint || '.' == codepoint || '`' == codepoint || ';' == codepoint ) { return 3; }
  if ( '(' == codepoint || ')' == codepoint || ' ' == codepoint ) { return 4; }
  if ( ' ' == codepoint || '{' == codepoint || '}' == codepoint ) { return 5; }
  if ( '&' == codepoint ) { return 6; } // TODO 7 when changed the glyph
  return 6;
}

static bool _md_bold          = false;
static bool _md_italic        = false;
static bool _md_underlined    = false;
static bool _md_strikethrough = false;

static void _read_markdown( const char* str, size_t max_n, int* bytes_used_ptr ) {
  *bytes_used_ptr = 0;
  assert( str && max_n > 0 && bytes_used_ptr );
  if ( !str || max_n < 1 || !bytes_used_ptr ) { return; }
  size_t str_len = strnlen( str, max_n );
  if ( str_len < 1 ) { return; }

  switch ( str[0] ) {
  case '*':
    if ( str_len > 1 && str[1] == '*' ) { // **word is bold.
      if ( _md_bold ) {
        _md_bold        = false;
        *bytes_used_ptr = 2;
        printf( "md_bold off\n" );
        return;
      } else if ( str_len > 2 && str[2] > ' ' ) {
        _md_bold        = !_md_bold;
        *bytes_used_ptr = 2;
        printf( "md_bold %i\n", _md_bold );
        return;
      }
    } else {
      if ( _md_italic ) {
        _md_italic      = false;
        *bytes_used_ptr = 1;
        printf( "_md_italic off\n" );
        return;
      } else if ( str_len > 1 && str[1] > ' ' ) { // *word is italic.
        _md_italic      = !_md_italic;
        *bytes_used_ptr = 1;
        printf( "_md_italic %i\n", _md_italic );
        return;
      }
      break;
    case '_':
      if ( _md_underlined ) {
        _md_underlined  = false;
        *bytes_used_ptr = 1;
        printf( "_md_underlined off\n" );
        return;
      } else if ( str_len > 1 && str[1] > ' ' ) { // _word is underlined.
        _md_underlined  = !_md_underlined;
        *bytes_used_ptr = 1;
        printf( "_md_underlined %i\n", _md_underlined );
        return;
      }
      break;
    case '~':
      if ( _md_strikethrough ) {
        _md_strikethrough = false;
        *bytes_used_ptr   = 1;
        printf( "_md_strikethrough off\n" );
        return;
      } else if ( str_len > 1 && str[1] > ' ' ) { // _word is underlined.
        _md_strikethrough = !_md_strikethrough;
        *bytes_used_ptr   = 1;
        printf( "_md_strikethrough %i\n", _md_strikethrough );
        return;
      }
      break;
    default: return;
    }
  } // endswitch.
}

static int _read_control_code( const char* str, int max_n, int* bytes_used_ptr ) {
  int esc_code = -1;
  char tmp[10] = { 0 };

  int str_len = strnlen( str, max_n );
  int tmp_len = str_len < 9 ? str_len : 9;
  strncat( tmp, str, str_len );
  printf( "db: tmp=%s len %i\n", tmp, tmp_len );

  if ( 0 == strncmp( tmp, "\\033[m", tmp_len ) ) {
    *bytes_used_ptr = 6;
    return 0;
  }
  // TODO sscanf fails here: \033[1nm - it think thats valid because it sees the n _eventually_. urgh.
  if ( 1 == sscanf( tmp, "\\033[%im", &esc_code ) ) {
    int hundreds    = esc_code / 100;
    int tens        = ( esc_code - hundreds * 100 ) / 10;
    int code_len    = hundreds > 0 ? 3 : tens > 0 ? 2 : 1;
    *bytes_used_ptr = 6 + code_len;
    return esc_code;
  }
  *bytes_used_ptr = 0;
  return -1;
}

/**
 * @param str_ptr         Address of a null-terminated UTF-8 or ASCII string.
 * @param points_vbo      An existing (previously generated) vertex buffer object for vertex points.
 * @param texcoords_vbo   An existing (previously generated) vertex buffer object for texture coordinates.
 * @param n_verts_ptr     Address of a variable to store the number of vertex points created. Must not be NULL.
 * @param total_w,total_h Total width and height of the whole text area in i.e. clip space. Can be used to centre the text when given to a position uniform.
 */
void text_to_vbo( const char* str_ptr, GLuint points_vbo, GLuint texcoords_vbo, GLuint palidx_vbo, size_t* n_verts_ptr, float* total_w, float* total_h ) {
  const int atlas_cols = 16, atlas_rows = 16;
  const size_t max_len = 2048;
  // Size of a full glyph box in clip space, before aspect ratio calculation. 2.0 would cover the whole range from -1,1 on x and y on a square viewport.
  const float quad_scale               = 2.0f;
  const float pixels_to_cells          = 1.0f / 16.0f; // Convert glyph width in pixels, to a factor 0 to 1 of a full 16 pixel width cell.
  const float pixels_gap_between_quads = -0.05f;       // Small overlap to hide any seams.
  const int pixels_of_linespacing      = 0;            // To allow extra spacing between lines, given in a number of glyph pixels.
  float at_x = 0.0f, at_y = 0.0f;                      // This is starting centre of the top-left glyph box in clip space.
  float max_x = 0.0f, min_y = 0.0f;

  assert( str_ptr && points_vbo && texcoords_vbo && palidx_vbo && n_verts_ptr );

  size_t n_bytes = strnlen( str_ptr, max_len );
  if ( n_bytes < 1 ) { return; }
  int n_codepoints = apg_utf8_count_cp( str_ptr );
  assert( n_codepoints > 0 );

  float* points_tmp    = (float*)malloc( sizeof( float ) * n_codepoints * 12 ); // vec2    Remove run-time malloc, use a max string length.
  float* texcoords_tmp = (float*)malloc( sizeof( float ) * n_codepoints * 12 ); // vec2    Remove run-time malloc, use a max string length.
  float* style_tmp     = (float*)malloc( sizeof( float ) * n_codepoints * 6 );  // float   Remove run-time malloc, use a max string length.

  const float line_start_x = at_x;
  for ( int i = 0, curr_byte = 0; curr_byte < n_bytes; i++, curr_byte++ ) {
    {
      int bytes_used_in_md = 0;
      _read_markdown( str_ptr + curr_byte, max_len, &bytes_used_in_md );
      if ( bytes_used_in_md > 0 ) {
        curr_byte += ( bytes_used_in_md - 1 );
        continue;
      }
    }
    /* {
       int bytes_in_esc_code = 0;
       int esc_code          = _read_control_code( str_ptr + curr_byte, max_len, &bytes_in_esc_code );
       if ( esc_code > -1 && bytes_in_esc_code > 0 ) {
         printf( "DB: Setting escape code %i biec=%i\n", esc_code, bytes_in_esc_code );
         curr_byte += ( bytes_in_esc_code - 1 );
         continue;
       }
     }*/

    uint32_t style = 0;
    if ( _md_bold ) { style = 1; }
    if ( _md_italic ) { style = 2; }
    if ( _md_underlined ) { style = 3; }
    if ( _md_strikethrough ) { style = 4; }

    int bytes_in_codepoint = 0;
    uint32_t codepoint     = apg_utf8_to_cp( str_ptr + curr_byte, &bytes_in_codepoint );
    assert( bytes_in_codepoint > 0 );
    curr_byte += ( bytes_in_codepoint - 1 );
    assert( codepoint < atlas_cols * atlas_rows );

    uint32_t atlas_idx = codepoint - ' ';                    // Atlas starts at space.
    if ( codepoint > 127 ) { atlas_idx -= ( 0xA0 - 0x80 ); } // Also skip Latin-1 control codes (not in atlas).
    uint32_t atlas_col = atlas_idx % atlas_cols;
    uint32_t atlas_row = atlas_idx / atlas_cols;

    float s     = atlas_col * ( 1.0f / atlas_cols );
    float t     = ( atlas_row + 1 ) * ( 1.0f / atlas_rows );
    float x_pos = at_x;
    float y_pos = at_y;

    int pixels_wide = _get_spacing_for_codepoint_px( codepoint );
    if ( style == 1 ) { pixels_wide++; }
    if ( 2 == style ) { pixels_wide = pixels_wide + 7; } // TODO LUT
    if ( 3 == style || 4 == style ) { pixels_wide = pixels_wide + thickness; } // LUT
    if ( outline ) { pixels_wide++; }

    float extent_x = (float)pixels_wide * pixels_to_cells * quad_scale;
    float extent_y = quad_scale;

    // Move along for start of next glypg.                                                // 0.1 of a glyph image pixel
    at_x += ( (float)pixels_wide + pixels_gap_between_quads ) * pixels_to_cells * quad_scale; // Move next glyph along to the end of this one.
    if ( '\n' == codepoint ) {
      at_x = line_start_x;
      at_y -= ( quad_scale + ( pixels_gap_between_quads + (float)pixels_of_linespacing * 2.0f ) * pixels_to_cells );
      n_codepoints--; // Don't include vertices/accounting for linebreak.
      i--;
    } else if ( codepoint < 32 || 127 == codepoint ) { // Control symbols and DEL. Space ' ' is included for underline/strikethrough.
      n_codepoints--;                                  // Don't include vertices/accounting for space.
      i--;
    } else {
      max_x = x_pos + extent_x > max_x ? x_pos + extent_x : max_x;
      min_y = y_pos - extent_y < min_y ? y_pos - extent_y : min_y;

      // Add 6 points and texture coordinates to buffers for each glyph.
      points_tmp[i * 12 + 0] = x_pos;
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

      texcoords_tmp[i * 12 + 0] = s;
      texcoords_tmp[i * 12 + 1] = 1.0 - t + 1.0 / atlas_rows;
      texcoords_tmp[i * 12 + 2] = s;
      texcoords_tmp[i * 12 + 3] = 1.0 - t;
      texcoords_tmp[i * 12 + 4] = s + 1.0 / atlas_cols * pixels_wide * pixels_to_cells;
      texcoords_tmp[i * 12 + 5] = 1.0 - t;

      texcoords_tmp[i * 12 + 6]  = s + 1.0 / atlas_cols * pixels_wide * pixels_to_cells;
      texcoords_tmp[i * 12 + 7]  = 1.0 - t;
      texcoords_tmp[i * 12 + 8]  = s + 1.0 / atlas_cols * pixels_wide * pixels_to_cells;
      texcoords_tmp[i * 12 + 9]  = 1.0 - t + 1.0 / atlas_rows;
      texcoords_tmp[i * 12 + 10] = s;
      texcoords_tmp[i * 12 + 11] = 1.0 - t + 1.0 / atlas_rows;

      for ( int s = 0; s < 6; s++ ) { style_tmp[i * 6 + s] = (float)style; }
    }
  }

  glBindBuffer( GL_ARRAY_BUFFER, points_vbo );
  glBufferData( GL_ARRAY_BUFFER, n_codepoints * 12 * sizeof( float ), points_tmp, GL_DYNAMIC_DRAW );
  glBindBuffer( GL_ARRAY_BUFFER, texcoords_vbo );
  glBufferData( GL_ARRAY_BUFFER, n_codepoints * 12 * sizeof( float ), texcoords_tmp, GL_DYNAMIC_DRAW );
  glBindBuffer( GL_ARRAY_BUFFER, palidx_vbo );
  glBufferData( GL_ARRAY_BUFFER, n_codepoints * 6 * sizeof( float ), style_tmp, GL_DYNAMIC_DRAW );
  glBindBuffer( GL_ARRAY_BUFFER, 0 );

  free( points_tmp );
  free( texcoords_tmp );
  free( style_tmp );

  *n_verts_ptr = n_codepoints * 6; // NB Triangle Strip can't work because it will try to join geom across gaps.
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
  gfx_texture_t pixel_texture_array;
  pixel_texture_array =
    gfx_texture_array_create_from_files( image_filenames, 5, 256, 256, 4, ( gfx_texture_properties_t ){ .bilinear = true, .is_srgb = true, .is_array = true } );
  uint32_t u_fb_dims_loc     = gfx_uniform_loc( &pixel_shader, "u_fb_dims" );
  uint32_t u_atlas_index_loc = gfx_uniform_loc( &pixel_shader, "u_atlas_index" );

  // Create the quads with atlas-looked texture coords.
  gfx_mesh_t mesh = gfx_mesh_create_empty( true, true, false, true, false, false, false, false, true );
  float total_x = 0.0f, total_y = 0.0f;

  text_to_vbo( user_str, mesh.points_vbo, mesh.texcoords_vbo, mesh.palidx_vbo, &mesh.n_vertices, &total_x, &total_y );

  printf( "\nn_verts = %u\n", (uint32_t)mesh.n_vertices );

  { // Do a little dance to link shader attributes to vertex buffer memory.
    glBindVertexArray( mesh.vao );
    glBindBuffer( GL_ARRAY_BUFFER, mesh.points_vbo );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, NULL ); // Vertex positions.
    glBindBuffer( GL_ARRAY_BUFFER, mesh.texcoords_vbo );
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, NULL ); // Texture coordinates.ns.
    glBindBuffer( GL_ARRAY_BUFFER, mesh.palidx_vbo );
    glEnableVertexAttribArray( 4 );
    glVertexAttribPointer( 4, 1, GL_FLOAT, GL_FALSE, 0, NULL ); // Pallette index is 4.
    // TODO - Vertex colours from escape codes. Maybe
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
      gfx_draw_mesh( GFX_PT_TRIANGLES, &pixel_shader, identity_mat4(), identity_mat4(), M, mesh.vao, mesh.n_vertices, &pixel_texture_array, 1 );

      gfx_alpha_testing( false );
    }

    gfx_swap_buffer();
  }

  gfx_stop();
  return 0;
}
