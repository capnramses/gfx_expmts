/**
 * 3D Texture with Raycast
 * Anton Gerdelan, 29 Nov 2025.
 *
 * RUN
 *
 * ./a.out -iz sword.bmp 30 -iz sword2.bmp 31 -iz sword2.bmp 29
 *
 * TODO
 *
 * - save button -> vox format
 * - load vox format
 * - better camera controls
 * - correct render if ray starts inside the bounding cube
 * - check if it still works with a model matrix / grid spinning.
 * - think about lighting and shading. - the axis should inform which normal to use for shading.
 */

#define APG_IMPLEMENTATION
#define APG_NO_BACKTRACES
#include "apg.h"
#include "apg_bmp.h"
#include "apg_maths.h"
#include "gfx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define GRID_VOXELS_ACROSS 32 // 256

int arg_pos( const char* str, int argc, char** argv ) {
  for ( int i = 1; i < argc; i++ ) {
    if ( 0 == strcmp( str, argv[i] ) ) { return i; }
  }
  return -1;
}

int main( int argc, char** argv ) {
  gfx_t gfx = gfx_start( 800, 600, "3D Texture Demo" );
  if ( !gfx.started ) { return 1; }

  mesh_t cube = gfx_mesh_cube_create();

  size_t grid_w = GRID_VOXELS_ACROSS, grid_h = GRID_VOXELS_ACROSS, grid_d = GRID_VOXELS_ACROSS, grid_n = 3;
  uint8_t* img_ptr = calloc( 1, grid_w * grid_h * grid_d * grid_n );
  if ( !img_ptr ) {
    fprintf( stderr, "ERROR: allocating memory\n" );
    return 1;
  }
  bool created_voxels = false;

  // -iz sword.bmp 1     replaces z layer 1 with the image in sword.bmp
  for ( int i = 1; i < argc - 2; i++ ) {
    if ( 0 == strcmp( "-iz", argv[i] ) && i < argc - 2 ) {
      const char* img_fn = argv[i + 1];
      int z_layer        = atoi( argv[i + 2] );
      int w = 0, h = 0, n = 0;
      unsigned char* fimg_ptr = apg_bmp_read( img_fn, &w, &h, &n );
      if ( !fimg_ptr ) {
        fprintf( stderr, "ERROR: loading image `%s`\n", img_fn );
        return 1;
      }
      printf( "loaded image `%s` %ix%i@x%i\n", img_fn, w, h, n );
      memcpy( &img_ptr[z_layer * grid_w * grid_h * grid_n], fimg_ptr, w * h * n );
      free( fimg_ptr );

      created_voxels = true;
    }
  }

  if ( !created_voxels ) {
    for ( int z = 0; z < grid_d; z++ ) {
      for ( int y = 0; y < grid_h; y++ ) {
        for ( int x = 0; x < grid_w; x++ ) {
          int pc = rand() % 100;
          if ( pc > 90 ) {
            int idx                   = z * grid_w * grid_h + y * grid_w + x;
            img_ptr[idx * grid_n + 0] = rand() % 255 + 1;
            img_ptr[idx * grid_n + 1] = rand() % 255 + 1;
            img_ptr[idx * grid_n + 2] = rand() % 255 + 1;
          }
        }
      }
    }
  }

  texture_t tex = gfx_texture_create( grid_w, grid_h, grid_d, grid_n, img_ptr );

  shader_t shader = (shader_t){ .program = 0 };
  if ( !gfx_shader_create_from_file( "cube.vert", "cube.frag", &shader ) ) { return 1; }

  vec3 cam_pos    = (vec3){ 0, 0, 5 };
  float cam_speed = 10.0f;

  bool space_lock = false, cull_back = false;

  glfwSwapInterval( 0 );

  double prev_s         = glfwGetTime();
  double update_timer_s = 0.0;
  while ( !glfwWindowShouldClose( gfx.window_ptr ) ) {
    double curr_s    = glfwGetTime();
    double elapsed_s = curr_s - prev_s;
    prev_s           = curr_s;
    update_timer_s += elapsed_s;
    if ( update_timer_s > 0.2 ) {
      update_timer_s = 0.0;
      double fps     = 1.0 / elapsed_s;
      char title_str[512];
      sprintf( title_str, "amanatides-woo @ %.2f FPS", fps );
      glfwSetWindowTitle( gfx.window_ptr, title_str );
    }
    glfwPollEvents();
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_ESCAPE ) ) { glfwSetWindowShouldClose( gfx.window_ptr, 1 ); }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_W ) ) { cam_pos.z -= cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_S ) ) { cam_pos.z += cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_A ) ) { cam_pos.x -= cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_D ) ) { cam_pos.x += cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_Q ) ) { cam_pos.y -= cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_E ) ) { cam_pos.y += cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_SPACE ) ) {
      if ( !space_lock ) {
        cull_back  = !cull_back;
        space_lock = true;
      }
    } else {
      space_lock = false;
    }

    uint32_t win_w, win_h, fb_w, fb_h;
    glfwGetWindowSize( gfx.window_ptr, &win_w, &win_h );
    glfwGetFramebufferSize( gfx.window_ptr, &fb_w, &fb_h );
    float aspect = (float)fb_w / (float)fb_h;
    glViewport( 0, 0, win_w, win_h );
    glClearColor( 0.6f, 0.6f, 0.8f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glEnable( GL_CULL_FACE );
    glCullFace( cull_back ? GL_BACK : GL_FRONT );
    glFrontFace( GL_CCW );

    mat4 P = perspective( 66.6f, aspect, 0.1f, 100.0f );
    mat4 V = look_at( cam_pos, (vec3){ 0 }, (vec3){ 0, 1, 0 } );

    glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_P" ), 1, GL_FALSE, P.m );
    glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_V" ), 1, GL_FALSE, V.m );
    glProgramUniform3fv( shader.program, glGetUniformLocation( shader.program, "u_cam_pos_wor" ), 1, &cam_pos.x );
    glProgramUniform1i( shader.program, glGetUniformLocation( shader.program, "u_n_cells" ), grid_w );
    gfx_draw( cube, tex, shader );

    glfwSwapBuffers( gfx.window_ptr );
  }

  gfx_stop();
  free( img_ptr );

  printf( "Normal exit.\n" );

  return 0;
}
