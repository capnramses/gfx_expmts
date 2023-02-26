// Pixel shader demo.
// by Anton Gerdelan, Feb 2023

#include "apg_unicode.h"
#include "gfx.h"
#include "glcontext.h"
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


work in progress - convert this function, stop trying to be too clever with instanced rendering.

void text_to_vbo( const char* str, float at_x, float at_y, float scale_px, GLuint* points_vbo, GLuint* texcoords_vbo, int* point_count ) {
  int len = strlen( str );

  float* points_tmp    = (float*)malloc( sizeof( float ) * len * 12 );
  float* texcoords_tmp = (float*)malloc( sizeof( float ) * len * 12 );
  for ( int i = 0; i < len; i++ ) {
    // get ascii code as integer
    int ascii_code = str[i];

    // work out row and column in atlas
    int atlas_col = ( ascii_code - ' ' ) % ATLAS_COLS;
    int atlas_row = ( ascii_code - ' ' ) / ATLAS_COLS;

    // work out texture coordinates in atlas
    float s = atlas_col * ( 1.0 / ATLAS_COLS );
    float t = ( atlas_row + 1 ) * ( 1.0 / ATLAS_ROWS );

    // work out position of glyphtriangle_width
    float x_pos = at_x;
    float y_pos = at_y - scale_px / g_viewport_height * glyph_y_offsets[ascii_code];

    // move next glyph along to the end of this one
    if ( i + 1 < len ) {
      // upper-case letters move twice as far
      at_x += glyph_widths[ascii_code] * scale_px / g_viewport_width;
    }
    // add 6 points and texture coordinates to buffers for each glyph
    points_tmp[i * 12]     = x_pos;
    points_tmp[i * 12 + 1] = y_pos;
    points_tmp[i * 12 + 2] = x_pos;
    points_tmp[i * 12 + 3] = y_pos - scale_px / g_viewport_height;
    points_tmp[i * 12 + 4] = x_pos + scale_px / g_viewport_width;
    points_tmp[i * 12 + 5] = y_pos - scale_px / g_viewport_height;

    points_tmp[i * 12 + 6]  = x_pos + scale_px / g_viewport_width;
    points_tmp[i * 12 + 7]  = y_pos - scale_px / g_viewport_height;
    points_tmp[i * 12 + 8]  = x_pos + scale_px / g_viewport_width;
    points_tmp[i * 12 + 9]  = y_pos;
    points_tmp[i * 12 + 10] = x_pos;
    points_tmp[i * 12 + 11] = y_pos;

    texcoords_tmp[i * 12]     = s;
    texcoords_tmp[i * 12 + 1] = 1.0 - t + 1.0 / ATLAS_ROWS;
    texcoords_tmp[i * 12 + 2] = s;
    texcoords_tmp[i * 12 + 3] = 1.0 - t;
    texcoords_tmp[i * 12 + 4] = s + 1.0 / ATLAS_COLS;
    texcoords_tmp[i * 12 + 5] = 1.0 - t;

    texcoords_tmp[i * 12 + 6]  = s + 1.0 / ATLAS_COLS;
    texcoords_tmp[i * 12 + 7]  = 1.0 - t;
    texcoords_tmp[i * 12 + 8]  = s + 1.0 / ATLAS_COLS;
    texcoords_tmp[i * 12 + 9]  = 1.0 - t + 1.0 / ATLAS_ROWS;
    texcoords_tmp[i * 12 + 10] = s;
    texcoords_tmp[i * 12 + 11] = 1.0 - t + 1.0 / ATLAS_ROWS;
  }

  glBindBuffer( GL_ARRAY_BUFFER, *points_vbo );
  glBufferData( GL_ARRAY_BUFFER, len * 12 * sizeof( float ), points_tmp, GL_DYNAMIC_DRAW );
  glBindBuffer( GL_ARRAY_BUFFER, *texcoords_vbo );
  glBufferData( GL_ARRAY_BUFFER, len * 12 * sizeof( float ), texcoords_tmp, GL_DYNAMIC_DRAW );

  free( points_tmp );
  free( texcoords_tmp );

  *point_count = len * 6;
}

int main( int argc, char** argv ) {
  if ( argc < 2 ) {
    printf( "Usage: %s SOME_USER_STRING\n", argv[0] );
    return 0;
  }
  const char* user_str = argv[1];
  int spacing_cumu     = 0;
  uint32_t n_cps       = apg_utf8_count_cp( user_str );
  printf( "User string: %s has %u codepoints\n", user_str, n_cps );
  uint32_t* cps_ptr = malloc( n_cps * sizeof( uint32_t ) );
  if ( !cps_ptr ) { return 1; }
  uint32_t* spacing_ptr = malloc( n_cps * sizeof( uint32_t ) );
  if ( !spacing_ptr ) { return 1; }
  uint32_t curr_byte = 0;
  for ( uint32_t i = 0; i < n_cps; i++ ) {
    const char* curr_utf8_cp = &user_str[curr_byte];
    int sz                   = 0;
    cps_ptr[i]               = apg_utf8_to_cp( curr_utf8_cp, &sz );
    spacing_ptr[i]           = spacing_cumu; // TODO
    curr_byte += sz;
    spacing_cumu += 8;
    printf( "%i) %u\n", i, cps_ptr[i] );
  }

  if ( !gfx_start( "pixelart antialiasing demo", NULL, false ) ) { return 1; }

  gfx_shader_t pixel_shader = gfx_create_shader_program_from_files( "pixel_text.vert", "pixel_text.frag" );
  if ( pixel_shader.program_gl == 0 ) { return 1; }
  gfx_texture_t pixel_texture = gfx_texture_create_from_file( image_filename, ( gfx_texture_properties_t ){ .bilinear = true, .is_srgb = true } );
  uint32_t u_fb_dims_loc      = gfx_uniform_loc( &pixel_shader, "u_fb_dims" );
  uint32_t u_atlas_index_loc  = gfx_uniform_loc( &pixel_shader, "u_atlas_index" );

  gfx_buffer_t instanced_buffers[2];
  instanced_buffers[0] = gfx_buffer_create( (float*)cps_ptr, 1, n_cps, true );
  instanced_buffers[1] = gfx_buffer_create( (float*)spacing_ptr, 1, n_cps, true );

  while ( !gfx_should_window_close() ) {
    int ww = 0, wh = 0, fbw = 0, fbh = 0;
    double t = gfx_get_time_s();
    mat4 S   = scale_mat4( ( vec3 ){ .9 + cosf( t ) * 0.1f, .9, .2 } );
    mat4 R   = rot_z_deg_mat4( t * 0.1f );
    mat4 T   = translate_mat4( ( vec3 ){ .x = cosf( t * 0.2f ) * 0.1, .y = 0, .z = sinf( t * 0.1f ) * 0.2f } );
    mat4 M   = mult_mat4_mat4( mult_mat4_mat4( T, R ), S );

    gfx_poll_events();
    gfx_clear_colour_and_depth_buffers( 0.5f, 0.5f, 0.5f, 1.0f );
    gfx_window_dims( &ww, &wh );
    gfx_framebuffer_dims( &fbw, &fbh );
    gfx_viewport( 0, 0, ww, wh );

    // No need for alpha-blending if using a hard discard.
    gfx_alpha_testing( true );
    gfx_depth_testing( false );
    gfx_uniform1i( &pixel_shader, u_atlas_index_loc, cps_ptr[0] );
    gfx_uniform2f( &pixel_shader, u_fb_dims_loc, fbw, fbh );
    // gfx_draw_mesh( GFX_PT_TRIANGLE_STRIP, &pixel_shader, identity_mat4(), identity_mat4(), M, gfx_ss_quad_mesh.vao, gfx_ss_quad_mesh.n_vertices,
    // &pixel_texture, 1 );
    _draw_mesh_instanced( &pixel_shader, M, gfx_ss_quad_mesh.vao, gfx_ss_quad_mesh.n_vertices, &pixel_texture, 1, n_cps, instanced_buffers, 2 );
    gfx_alpha_testing( false );

    gfx_swap_buffer();
  }

  free( cps_ptr );
  free( spacing_ptr );
  gfx_buffer_delete( &instanced_buffers[0] );
  gfx_buffer_delete( &instanced_buffers[1] );

  gfx_stop();
  return 0;
}
