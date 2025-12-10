/* clang-format off

 * 3D Texture with Raycast
 * Anton Gerdelan, 29 Nov 2025.
 *
 * RUN
 *
 * ./a.out -iz sword2.bmp 14 -iz sword.bmp 15 -iz sword2.bmp 16
 * ./a.out -iz fish3.bmp 10 -iz fish3.bmp 11 -iz fish3.bmp 12 -iz fish3.bmp 13 -iz fish3.bmp 14 -iz fish4.bmp 15 -iz fish4.bmp 16 -iz fish.bmp 17 -iz fish2.bmp 18 -iz fish2.bmp 19
 * ./a.out -d 16 -iz cobble0.bmp 1 -d 16 -iz cobble1.bmp 0 0 -iz cobble1.bmp 2 -iz 
 *
 * TODO
 *
 * - save button -> vox format
 * - load vox format
 * SORTA - better camera controls
 * DONE  - correct render if ray starts inside the bounding cube (needed inside detect & flip cull & change t origin + near clip for regular cam change.)
 * DONE  - support scale/translate matrix so >1 voxel mesh can exist in scene.
 * SORTA - think about lighting and shading. - the axis should inform which normal to use for shading.
 * - support voxel bounding box rotation. does this break the "uniform grid" idea?
 * - write voxel depth into depth map, not cube sides. and preview depth in a subwindow (otherwise intersections/z fight occur on bounding cube sides).
 * 
 * clang-format on
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

int arg_pos( const char* str, int argc, char** argv ) {
  for ( int i = 1; i < argc; i++ ) {
    if ( 0 == strcmp( str, argv[i] ) ) { return i; }
  }
  return -1;
}

int colour_dist_sq( const uint8_t* a, const uint8_t* b ) {
  return ( a[0] * a[0] - b[0] * b[0] ) + ( a[1] * a[1] - b[1] * b[1] ) + ( a[2] * a[2] - b[2] * b[2] );
}

uint8_t closest_pal_idx( const uint8_t* pal, const uint8_t* rgb ) {
  uint8_t closest_idx = 0;
  int closest_dist = 255 * 255 * 3 + 1;
  for ( int i = 0; i < 256; i++ ) {
    int dist = colour_dist_sq( &pal[i * 3], rgb );
    if ( dist < closest_dist ) {
      closest_dist = dist;
      closest_idx = i;
    }
  }
  return closest_idx;
}

int main( int argc, char** argv ) {
  uint8_t* pal_ptr = calloc( 256 * 3, 1 );
  assert( pal_ptr );
  FILE* f_ptr = fopen( "my.pal", "rb" );
  assert( f_ptr );
  fread( pal_ptr, 1, 256 * 3, f_ptr );
  fclose( f_ptr );

  TODO - load palette into 1D texture
  TODO - change 3d texture to 1 byte per voxel
  TODO - random byte per voxel and index into palette texture strip
  TODO - for each loaded image slice, use colour distance to assign palette indices.

  gfx_t gfx = gfx_start( 800, 600, "3D Texture Demo" );
  if ( !gfx.started ) { return 1; }

  size_t grid_dims = 32;
  int n_idx        = arg_pos( "-d", argc, argv );
  if ( n_idx > 0 && n_idx < argc - 1 ) {
    grid_dims = atoi( argv[n_idx + 1] );
    printf( "grid_dims set to %i\n", (int)grid_dims );
  }

  mesh_t cube = gfx_mesh_cube_create();

  size_t grid_w = grid_dims, grid_h = grid_dims, grid_d = grid_dims, grid_n = 3;
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
      assert( w == h && h == grid_dims );
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
          if ( pc >= 90 ) {
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

  shader_t shader = ( shader_t ){ .program = 0 };
  if ( !gfx_shader_create_from_file( "cube.vert", "cube.frag", &shader ) ) { return 1; }

  vec3 cam_pos    = ( vec3 ){ 0, 0, 5 };
  float cam_speed = 10.0f;
  float cam_dist = 5.0f, cam_height = 1.1f;
  bool space_lock = false, show_bounding_cube = false;

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
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_W ) ) { cam_dist -= cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_S ) ) { cam_dist += cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_Q ) ) { cam_height -= cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_E ) ) { cam_height += cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_SPACE ) ) {
      if ( !space_lock ) {
        show_bounding_cube = !show_bounding_cube;
        space_lock         = true;
      }
    } else {
      space_lock = false;
    }
    // Orbit camera.
    cam_pos = ( vec3 ){ cam_dist * cosf( curr_s * 0.5f ), cam_height, cam_dist * sinf( curr_s * 0.5f ) };

    uint32_t win_w, win_h, fb_w, fb_h;
    glfwGetWindowSize( gfx.window_ptr, &win_w, &win_h );
    glfwGetFramebufferSize( gfx.window_ptr, &fb_w, &fb_h );
    float aspect = (float)fb_w / (float)fb_h;
    glViewport( 0, 0, win_w, win_h );

    glDepthFunc( GL_LESS );
    glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );

    glClearColor( 0.6f, 0.6f, 0.8f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    mat4 P = perspective( 66.6f, aspect, 0.0001f, 100.0f );
    mat4 V = look_at( cam_pos, ( vec3 ){ 0 }, ( vec3 ){ 0, 1, 0 } );

    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW ); // NB Cube mesh used is inside-out.

    glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_P" ), 1, GL_FALSE, P.m );
    glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_V" ), 1, GL_FALSE, V.m );
    glProgramUniform3fv( shader.program, glGetUniformLocation( shader.program, "u_cam_pos_wor" ), 1, &cam_pos.x );
    glProgramUniform1i( shader.program, glGetUniformLocation( shader.program, "u_n_cells" ), grid_w );
    glProgramUniform1i( shader.program, glGetUniformLocation( shader.program, "u_show_bounding_cube" ), (int)!show_bounding_cube );

    {                                   // Draw first voxel cube.
      mat4 M         = identity_mat4(); //((vec3){.5,.5,.5});
      vec3 grid_max  = ( vec3 ){ 1, 1, 1 };
      vec3 grid_min  = ( vec3 ){ -1, -1, -1 };
      vec3 grid_maxb = vec3_from_vec4( mul_mat4_vec4( M, vec4_from_vec3f( grid_max, 1.0 ) ) );
      vec3 grid_minb = vec3_from_vec4( mul_mat4_vec4( M, vec4_from_vec3f( grid_min, 1.0 ) ) );
      // TODO - tidy this into a func.
      grid_min.x = APG_M_MIN( grid_minb.x, grid_maxb.x );
      grid_min.y = APG_M_MIN( grid_minb.y, grid_maxb.y );
      grid_min.z = APG_M_MIN( grid_minb.z, grid_maxb.z );
      grid_max.x = APG_M_MAX( grid_minb.x, grid_maxb.x );
      grid_max.y = APG_M_MAX( grid_minb.y, grid_maxb.y );
      grid_max.z = APG_M_MAX( grid_minb.z, grid_maxb.z );
      // Still want to render when inside bounding cube area, so flip to rendering inside out. Can't do both at once or it will look wonky.
      if ( cam_pos.x < grid_max.x && cam_pos.x > grid_min.x && cam_pos.y < grid_max.y && cam_pos.y > grid_min.y && cam_pos.z < grid_max.z &&
           cam_pos.z > grid_min.z ) {
        glCullFace( GL_FRONT );
      } else {
        glCullFace( GL_BACK );
      }

      glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_M" ), 1, GL_FALSE, M.m );
      glProgramUniform3fv( shader.program, glGetUniformLocation( shader.program, "u_grid_max" ), 1, &grid_max.x );
      glProgramUniform3fv( shader.program, glGetUniformLocation( shader.program, "u_grid_min" ), 1, &grid_min.x );
      gfx_draw( cube, tex, shader );
    }

    { // Draw second voxel cube.
      mat4 T         = translate_mat4( ( vec3 ){ 2.1, 0, 0 } );
      mat4 M         = T;
      vec3 grid_max  = ( vec3 ){ 1, 1, 1 };
      vec3 grid_min  = ( vec3 ){ -1, -1, -1 };
      vec3 grid_maxb = vec3_from_vec4( mul_mat4_vec4( M, vec4_from_vec3f( grid_max, 1.0 ) ) );
      vec3 grid_minb = vec3_from_vec4( mul_mat4_vec4( M, vec4_from_vec3f( grid_min, 1.0 ) ) );

      grid_min.x = APG_M_MIN( grid_minb.x, grid_maxb.x );
      grid_min.y = APG_M_MIN( grid_minb.y, grid_maxb.y );
      grid_min.z = APG_M_MIN( grid_minb.z, grid_maxb.z );
      grid_max.x = APG_M_MAX( grid_minb.x, grid_maxb.x );
      grid_max.y = APG_M_MAX( grid_minb.y, grid_maxb.y );
      grid_max.z = APG_M_MAX( grid_minb.z, grid_maxb.z );

      // Still want to render when inside bounding cube area, so flip to rendering inside out. Can't do both at once or it will look wonky.
      if ( cam_pos.x < grid_max.x && cam_pos.x > grid_min.x && cam_pos.y < grid_max.y && cam_pos.y > grid_min.y && cam_pos.z < grid_max.z &&
           cam_pos.z > grid_min.z ) {
        glCullFace( GL_FRONT );
      } else {
        glCullFace( GL_BACK );
      }
      glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_M" ), 1, GL_FALSE, M.m );
      glProgramUniform3fv( shader.program, glGetUniformLocation( shader.program, "u_grid_max" ), 1, &grid_max.x );
      glProgramUniform3fv( shader.program, glGetUniformLocation( shader.program, "u_grid_min" ), 1, &grid_min.x );
      //    gfx_draw( cube, tex, shader );
    }

    glfwSwapBuffers( gfx.window_ptr );
  }

  gfx_stop();
  free( img_ptr );
  free( pal_ptr );

  printf( "Normal exit.\n" );

  return 0;
}
