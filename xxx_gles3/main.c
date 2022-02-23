/*
 * Goal:   Desktop (Windows AND Linux AND MacOS) trivial demo using OpenGL ES 3 back-end.
 * Reason: Better multi-platform parity/porting and an idea about using a desktop R&D environment for mobile applications.
 * Older forum discussion: https://discourse.glfw.org/t/opengl-es3-0-with-glfw-on-pc/186/9
 *
 * TODO
 * ----
 * * Check ANGLE support on Windows
 * * See what happens on MacOS
 * * Check linking details on Ubuntu. DONE - nothing required.
 * * Test same code on an Android device.
 *
 *
 * CHANGES GL4->GLES3
 * ----------------
 * * Add API and version number window hints to context creation.
 * * Shaders use GLSL #version 320
 * * `precision highp float;` in fragment shader after #version tag.
 * * default shaders in gfx won't work -> they are 420. .: don't use default draw textured quad calls.
 * * calls to glProgramUniform...() will crash, replace with glUniform...().
 */

#include "gfx.h"
#include "apg_maths.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main( int argc, char** argv ) {
  printf( "Hello, world!\n" );

  /* NOTE: this function contains the commands to start in ES mode.
  the stdout output should indicate if ES started, and which version. */
  gfx_start( "gl ES 3.0 test", NULL, false );

  // Shaders must use OpenGL ES 3.0 GLSL version tags etc, so not using my internal default:
  gfx_shader_t es_shader = gfx_create_shader_program_from_files( "texquad_es.vert", "texquad_es.frag" );
  if ( !es_shader.loaded ) { return 1; }

  while ( !gfx_should_window_close() ) {
    gfx_poll_events();
    gfx_clear_colour_and_depth_buffers( 0.2f, 0.2f, 0.2f, 1.0f );

    mat4 M = scale_mat4( ( vec3 ){ 0.5f, 0.5f, 0.5f } );
    gfx_draw_mesh( GFX_PT_TRIANGLE_STRIP, &es_shader, identity_mat4(), identity_mat4(), M, gfx_ss_quad_mesh.vao, gfx_ss_quad_mesh.n_vertices, &gfx_checkerboard_texture, 1 );

    gfx_swap_buffer();
  }

  gfx_stop();

  return 0;
}
