/**
 * 3D Texture Test
 * Anton Gerdelan, 29 Nov 2025.
 */

#define APG_IMPLEMENTATION
#define APG_NO_BACKTRACES
#include "apg.h"
#include "apg_maths.h"
#include "gfx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

int main() {
  uint32_t seed = time( NULL );
  printf( "seed=%u\n", seed );
  srand( seed );

  gfx_t gfx = gfx_start( 800, 600, "3D Texture Demo" );
  if ( !gfx.started ) { return 1; }

  mesh_t cube = gfx_mesh_cube_create();

  int w = 16, h = 16, d = 16;
  int n            = 3;
  uint8_t* img_ptr = calloc( 1, w * h * d * n );
  for ( int z = 0; z < d; z++ ) {
    for ( int y = 0; y < h; y++ ) {
      for ( int x = 0; x < w; x++ ) {
        int idx              = z * w * h + y * w + x;
        img_ptr[idx * n + 0] = x * ( 256 / w );
        img_ptr[idx * n + 1] = y * ( 256 / h );
        img_ptr[idx * n + 2] = z * ( 256 / d );
      }
    }
  }
  texture_t tex = gfx_texture_create( w, h, d, n, img_ptr );
  free( img_ptr );

  shader_t shader = gfx_shader_create_from_file( "cube.vert", "cube.frag" );

  glClearColor( 0.2f, 0.2f, 0.2f, 1.0f );
  while ( !glfwWindowShouldClose( gfx.window_ptr ) ) {
    glfwPollEvents();

    uint32_t win_w, win_h, fb_w, fb_h;
    glfwGetWindowSize( gfx.window_ptr, &win_w, &win_h );
    glfwGetFramebufferSize( gfx.window_ptr, &fb_w, &fb_h );
    float aspect = (float)fb_w / (float)fb_h;
    glViewport( 0, 0, win_w, win_h );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glEnable( GL_CULL_FACE ); //
    glCullFace( GL_BACK );   // <--------- inside of cube is drawn
    glFrontFace( GL_CCW );     //

    mat4 P = perspective( 66.6f, aspect, 0.1f, 100.0f );
    mat4 V = look_at( (vec3){ 0, 0, 5 }, (vec3){ 0 }, (vec3){ 0, 1, 0 } );

    glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_P" ), 1, GL_FALSE, P.m );
    glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_V" ), 1, GL_FALSE, V.m );
    gfx_draw( cube, tex, shader );

    glfwSwapBuffers( gfx.window_ptr );
  }

  gfx_stop();

  printf( "Normal exit.\n" );

  return 0;
}
