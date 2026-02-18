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

typedef struct vox_info_t {
  uint32_t* n_models_ptr;
  uint32_t* dims_xyz_ptr;
  uint32_t* n_voxels_ptr;
  uint8_t* voxels_ptr; // n_voxels * 4 bytes (x,y,z,colour_index).
  uint32_t* rgba_ptr;  // Palette. 256 * 4 bytes (r,g,b,a).
  bool loaded;
} vox_info_t;

typedef struct chunk_hdr_t {
  char id[4];
  uint32_t content_sz;
  uint32_t children_chunks_sz;
} chunk_hdr_t;

int arg_pos( const char* str, int argc, char** argv ) {
  for ( int i = 1; i < argc; i++ ) {
    if ( 0 == strcmp( str, argv[i] ) ) { return i; }
  }
  return -1;
}

bool vox_fmt_write_file( const char* filename, vox_info_t info ) {
  bool success = false;

  if ( !filename ) { return success; }
  FILE* f_ptr = fopen( filename, "wb" );
  if ( !f_ptr ) { return success; }

  { // HDR
    char mns[4]      = { 'V', 'O', 'X', ' ' };
    uint32_t version = 200;
    size_t n         = fwrite( mns, 1, 4, f_ptr );
    n                = fwrite( &version, 4, 1, f_ptr );
  }
  { // Chunks
    // Work out size info for, and hierarchy of, chunks.
    uint32_t n_voxels          = *info.n_voxels_ptr;
    chunk_hdr_t rgba_chunk_hdr = (chunk_hdr_t){ .id = { 'R', 'G', 'B', 'A' }, .content_sz = 4 * 256, .children_chunks_sz = 0 };
    chunk_hdr_t xyzi_chunk_hdr = (chunk_hdr_t){ .id = { 'X', 'Y', 'Z', 'I' }, .content_sz = 4 + 4 * n_voxels, .children_chunks_sz = 0 };
    chunk_hdr_t size_chunk_hdr = (chunk_hdr_t){ .id = { 'S', 'I', 'Z', 'E' }, .content_sz = 4 * 3, .children_chunks_sz = 0 };
    chunk_hdr_t main_chunk_hdr =
      (chunk_hdr_t){ .id = { 'M', 'A', 'I', 'N' }, .content_sz = 0, .children_chunks_sz = size_chunk_hdr.content_sz + xyzi_chunk_hdr.content_sz + rgba_chunk_hdr.content_sz };

    // MAIN
    size_t n = fwrite( &main_chunk_hdr, sizeof( chunk_hdr_t ), 1, f_ptr );

    // SIZE
    n = fwrite( &size_chunk_hdr, sizeof( chunk_hdr_t ), 1, f_ptr );
    n = fwrite( info.dims_xyz_ptr, size_chunk_hdr.content_sz, 1, f_ptr );

    // VOXELS
    n = fwrite( &xyzi_chunk_hdr, sizeof( chunk_hdr_t ), 1, f_ptr );
    n = fwrite( info.n_voxels_ptr, 4, 1, f_ptr );
    n = fwrite( info.voxels_ptr, 4 * n_voxels, 1, f_ptr );

    // PALETTE
    n = fwrite( &rgba_chunk_hdr, sizeof( chunk_hdr_t ), 1, f_ptr );
    n = fwrite( info.rgba_ptr, rgba_chunk_hdr.content_sz, 1, f_ptr );
  }
  success = true;
_err_vox_fmt_write_file:
  fclose( f_ptr );
  return success;
}

int main( int argc, char** argv ) {
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

  uint32_t dims_xyz[3]          = { grid_w, grid_h, grid_d };
  uint32_t n_models             = 1;
  uint32_t palette_rgba[256]    = { 0x00000000, 0xffffffff };
  uint8_t* paletised_voxels_ptr = calloc( grid_w * grid_h * grid_d * 4, sizeof( uint8_t ) );
  int p_next                    = 2;

  uint8_t test_rgba_array[4] = { 0x11, 0x22, 0x33, 0xFF };
  uint32_t test_rgba;
  memcpy( &test_rgba, test_rgba_array, 4 );
  printf( "test_rgba=0x%x\n", test_rgba );

  // Count non-air voxels, and add colours to palette.
  uint32_t n_voxels = 0;
  for ( int z = 0; z < grid_d; z++ ) {
    for ( int y = 0; y < grid_h; y++ ) {
      for ( int x = 0; x < grid_w; x++ ) {
        int i = z * grid_h * grid_w + y * grid_w + x;
        // z_layer * grid_w * grid_h * grid_n
        assert( grid_n == 3 );
        uint8_t rgba[4] = { img_ptr[i * grid_n + 0], img_ptr[i * grid_n + 1], img_ptr[i * grid_n + 2], 0xFF };

        if ( 0 == rgba[0] && 0 == rgba[1] && 0 == rgba[2] ) { continue; }

        int use_p = 0;
        // Look through palette for exact colour match.
        for ( int p = 0; p < 255; p++ ) {
          if ( 0 == memcmp( &palette_rgba[p], rgba, 4 * sizeof( uint8_t ) ) ) {
            use_p = p;
            break;
          }
        }
        // Then try to add colour to palette if there is space.
        if ( !use_p && p_next < 255 ) {
          memcpy( &palette_rgba[p_next], rgba, 4 * sizeof( uint8_t ) );
          use_p = p_next++;
        }
        // Use another colour - could find closest match.
        if ( !use_p ) { use_p = 1; }
        paletised_voxels_ptr[n_voxels * 4 + 0] = (uint8_t)x;
        paletised_voxels_ptr[n_voxels * 4 + 1] = (uint8_t)z;
        paletised_voxels_ptr[n_voxels * 4 + 2] = grid_h - 1 - (uint8_t)y; // Gravity direction.
        paletised_voxels_ptr[n_voxels * 4 + 3] = (uint8_t)( use_p + 1 );  // color [0-254] are mapped to palette index [1-255]
        n_voxels++;
      }
    }
  }
  printf( "palette0=%x\n", palette_rgba[0] );
  printf( "palette1=%x\n", palette_rgba[1] );

  vox_info_t vox_info = (vox_info_t){
    .dims_xyz_ptr = dims_xyz,            //
    .loaded       = true,                //
    .n_models_ptr = &n_models,           //
    .n_voxels_ptr = &n_voxels,           //
    .n_models_ptr = &n_models,           //
    .rgba_ptr     = palette_rgba,        //
    .voxels_ptr   = paletised_voxels_ptr //
  }; //

  texture_t tex = gfx_texture_create( grid_w, grid_h, grid_d, grid_n, img_ptr );

  shader_t shader = (shader_t){ .program = 0 };
  if ( !gfx_shader_create_from_file( "cube.vert", "cube.frag", &shader ) ) { return 1; }

  vec3 cam_pos    = (vec3){ 0, 0, 5 };
  float cam_speed = 10.0f;
  float cam_dist = 5.0f, cam_height = 1.1f;
  bool space_lock = false, show_bounding_cube = false;

  glfwSwapInterval( 0 );

  double prev_s         = glfwGetTime();
  double update_timer_s = 0.0;
  bool f2_lock          = false;
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
    bool f2_pressed = false;
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_ESCAPE ) ) { glfwSetWindowShouldClose( gfx.window_ptr, 1 ); }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_W ) ) { cam_dist -= cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_S ) ) { cam_dist += cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_Q ) ) { cam_height -= cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_E ) ) { cam_height += cam_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_F2 ) ) {
      if ( !f2_lock ) {
        f2_pressed = f2_lock = true;
        const char* filename = "saved.vox";
        bool ret             = vox_fmt_write_file( filename, vox_info );
        if ( !ret ) {
          fprintf( stderr, "ERROR: Writing file=%s\n", filename );
          return 1;
        }
        printf( "Saved file=%s of %u voxels.\n", filename, *vox_info.n_voxels_ptr );
      }
    } else {
      f2_lock = f2_pressed = false;
    }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_SPACE ) ) {
      if ( !space_lock ) {
        show_bounding_cube = !show_bounding_cube;
        space_lock         = true;
      }
    } else {
      space_lock = false;
    }
    // Orbit camera.
    cam_pos = (vec3){ cam_dist * cosf( curr_s * 0.5f ), cam_height, cam_dist * sinf( curr_s * 0.5f ) };

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
    mat4 V = look_at( cam_pos, (vec3){ 0 }, (vec3){ 0, 1, 0 } );

    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW ); // NB Cube mesh used is inside-out.

    glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_P" ), 1, GL_FALSE, P.m );
    glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_V" ), 1, GL_FALSE, V.m );
    glProgramUniform3fv( shader.program, glGetUniformLocation( shader.program, "u_cam_pos_wor" ), 1, &cam_pos.x );
    glProgramUniform1i( shader.program, glGetUniformLocation( shader.program, "u_n_cells" ), grid_w );
    glProgramUniform1i( shader.program, glGetUniformLocation( shader.program, "u_show_bounding_cube" ), (int)!show_bounding_cube );

    {                                   // Draw first voxel cube.
      mat4 M         = identity_mat4(); //((vec3){.5,.5,.5});
      vec3 grid_max  = (vec3){ 1, 1, 1 };
      vec3 grid_min  = (vec3){ -1, -1, -1 };
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
      mat4 T         = translate_mat4( (vec3){ 2.1, 0, 0 } );
      mat4 M         = T;
      vec3 grid_max  = (vec3){ 1, 1, 1 };
      vec3 grid_min  = (vec3){ -1, -1, -1 };
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

  printf( "Normal exit.\n" );

  return 0;
}
