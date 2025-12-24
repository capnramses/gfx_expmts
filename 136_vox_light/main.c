/*
 * Voxel Raycast with Grid Rotation/Scale/Translate into World, Normals Derivation, and Lighting.
 * Anton Gerdelan, 23 Dec 2025.
 *
 * RUN
 * =================================

 ./a.out -iz sword2.bmp 14 -iz sword.bmp 15 -iz sword2.bmp 16

 ./a.out -iz fish3.bmp 10 -iz fish3.bmp 11 -iz fish3.bmp 12 -iz fish3.bmp 13 -iz fish3.bmp 14 -iz fish4.bmp 15 \
 -iz fish4.bmp 16 -iz fish.bmp 17 -iz fish2.bmp 18 -iz fish2.bmp 19

 ./a.out -d 16 -iz cobble0.bmp 1 -d 16 -iz cobble1.bmp 0 0 -iz cobble1.bmp 2 -iz

 * KEYS
 * =================================
 *
 * P        switch between colour palettes.
 * F2       save model as model.vox (uncompressed 8-bit raw voxel data).
 * F3       load model from model.vox.
 * space    show bounding box
 * WASDQE   move camera
 * Esc      quit
 *
 * TODO
 * =================================
 *
 * DONE  - save button -> vox format
 * DONE  - load vox format
 * SORTA - better camera controls
 * DONE  - correct render if ray starts inside the bounding cube (needed inside detect & flip cull & change t origin + near clip for regular cam change.)
 * DONE  - support scale/translate matrix so >1 voxel mesh can exist in scene.
 * DONE - think about lighting and shading. - the axis should inform which normal to use for shading.
 * DONE  - support voxel bounding box rotation. does this break the "uniform grid" idea?
 * TODO  - write voxel depth into depth map, not cube sides. and preview depth in a subwindow (otherwise intersections/z fight occur on bounding cube sides).
 * TODO  - mouse click to add/remove voxels.
 *
 */

// Use discrete GPU by default. not sure if it works on other OS. if in C++ must be in extern "C" { } block
#ifdef _WIN32
// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
__declspec( dllexport ) unsigned long int NvOptimusEnablement = 0x00000001;
// https://gpuopen.com/amdpowerxpressrequesthighperformance/
__declspec( dllexport ) int AmdPowerXpressRequestHighPerformance = 1;
#endif

#define APG_IMPLEMENTATION
#define APG_NO_BACKTRACES
#include "apg.h"
#include "apg_bmp.h"
#include "apg_maths.h"
#include "gfx.h"
#include <limits.h>
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
  return ( a[0] - b[0] ) * ( a[0] - b[0] ) + ( a[1] - b[1] ) * ( a[1] - b[1] ) + ( a[2] - b[2] ) * ( a[2] - b[2] );
}

uint8_t closest_pal_idx( const uint8_t* pal, const uint8_t* rgb ) {
  const int pal_n     = 3;
  uint8_t closest_idx = 0;
  // printf("rgb=%u,%u,%u\n",rgb[0],rgb[1],rgb[2]);
  int closest_dist = colour_dist_sq( &pal[0], rgb );
  for ( int i = 1; i < 256; i++ ) {
    int dist = colour_dist_sq( &pal[i * pal_n], rgb );
    if ( dist < closest_dist ) {
      closest_dist = dist;
      closest_idx  = i;
    }
  }
  return closest_idx;
}

static uint8_t* _pal_tex_from_file( const char* filename ) {
  assert( filename );

  uint8_t* pal_ptr = calloc( 256 * 3, 1 );
  assert( pal_ptr );

  FILE* f_ptr = fopen( filename, "rb" );
  assert( f_ptr );

  size_t n = fread( pal_ptr, 1, 256 * 3, f_ptr );
  assert( 256 * 3 == n );
  fclose( f_ptr );

  return pal_ptr;
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

  size_t grid_w = grid_dims, grid_h = grid_dims, grid_d = grid_dims;
  size_t grid_n    = 1;
  uint8_t* img_ptr = calloc( 1, grid_w * grid_h * grid_d * grid_n );
  if ( !img_ptr ) {
    fprintf( stderr, "ERROR: allocating memory\n" );
    return 1;
  }
  bool created_voxels = false;

  // NOTE(Anton) This custom filename thing is kinda dumb - I could just use a .bmp for these.
  uint8_t* my_pal_ptr    = _pal_tex_from_file( "my.pal" );
  uint8_t* reds_pal_ptr  = _pal_tex_from_file( "reds.pal" );
  uint8_t* doom_pal_ptr  = _pal_tex_from_file( "doom.pal" );
  uint8_t* blood_pal_ptr = _pal_tex_from_file( "blood.pal" );
  int palette_idx        = 3;
  int n_palettes         = 4;
  texture_t palettes[4];
  palettes[0] = gfx_texture_create( 256, 0, 0, 3, false, my_pal_ptr ); // 256x0x0 == 1d texture, not 256x1x1
  palettes[1] = gfx_texture_create( 256, 0, 0, 3, false, reds_pal_ptr );
  palettes[2] = gfx_texture_create( 256, 0, 0, 3, false, doom_pal_ptr );
  palettes[3] = gfx_texture_create( 256, 0, 0, 3, false, blood_pal_ptr );

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
      //      memcpy( &img_ptr[z_layer * grid_w * grid_h * grid_n], fimg_ptr, w * h * n );

      for ( int y = 0; y < h; y++ ) {
        for ( int x = 0; x < w; x++ ) {
          int ii = ( y * w + x ) * n;
          uint8_t rgb_ptr[3];
          memcpy( rgb_ptr, &fimg_ptr[ii], 3 );
          uint8_t idx = closest_pal_idx( blood_pal_ptr, rgb_ptr );
          //  printf("ii=%i idx=%i\n",ii, idx);
          img_ptr[z_layer * grid_w * grid_h + y * grid_w + x] = idx;
        }
      }

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
            int idx = z * grid_w * grid_h + y * grid_w + x;
            uint8_t rgb[3];
            rgb[0]                    = rand() % 255 + 1;
            rgb[1]                    = rand() % 255 + 1;
            rgb[2]                    = rand() % 255 + 1;
            img_ptr[idx * grid_n + 0] = closest_pal_idx( blood_pal_ptr, rgb );
            //   img_ptr[idx * grid_n + 1] = rand() % 255 + 1;
            //  img_ptr[idx * grid_n + 2] = rand() % 255 + 1;
          }
        }
      }
    }
  }

  texture_t voxels_tex = gfx_texture_create( grid_w, grid_h, grid_d, grid_n, true, img_ptr );

  shader_t shader = (shader_t){ .program = 0 };
  if ( !gfx_shader_create_from_file( "cube.vert", "cube.frag", &shader ) ) { return 1; }

  vec3 cam_pos    = (vec3){ 0, 0, 3 };
  float cam_speed = 10.0f;
  float cam_dist = 5.0f, cam_height = 1.1f;
  bool space_lock = false, show_bounding_cube = false, palette_swap_lock = false, f2_lock = false, f3_lock = false;

  glfwSwapInterval( 0 );

  int fullbrights[256] = { 0 };
  fullbrights[16*9+8] = 1; // Test concept with rubies on sword.

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
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_P ) ) {
      if ( !palette_swap_lock ) {
        palette_idx       = ( palette_idx + 1 ) % n_palettes;
        palette_swap_lock = true;
      }
    } else {
      palette_swap_lock = false;
    }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_F2 ) ) {
      if ( !f2_lock ) {
        printf( "Saving model.vox...\n" );
        f2_lock = true;

        FILE* f_ptr = fopen( "model.vox", "wb" );
        assert( f_ptr );
        char magic[4] = "VOX"; // VOX0 (include null term).
        fwrite( magic, 1, 4, f_ptr );
        uint32_t w = grid_w, h = grid_h, d = grid_d, bpp = 256;
        fwrite( &w, 4, 1, f_ptr );
        fwrite( &h, 4, 1, f_ptr );
        fwrite( &d, 4, 1, f_ptr );
        fwrite( &bpp, 4, 1, f_ptr );
        fwrite( img_ptr, 1, w * h * d, f_ptr );
        fclose( f_ptr );
      }
    } else {
      f2_lock = false;
    }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_F3 ) ) {
      if ( !f3_lock ) {
        printf( "Loading model.vox...\n" );
        f3_lock = true;

        FILE* f_ptr = fopen( "model.vox", "rb" );
        assert( f_ptr );
        char magic[4]; // VOX0 (include null term).
        fread( magic, 1, 4, f_ptr );
        assert( magic[0] == 'V' && magic[1] == 'O' && magic[2] == 'X' && magic[3] == 0 );
        uint32_t w = 0, h = 0, d = 0, bpp = 0;
        fread( &w, 4, 1, f_ptr );
        fread( &h, 4, 1, f_ptr );
        fread( &d, 4, 1, f_ptr );
        fread( &bpp, 4, 1, f_ptr );
        assert( w == grid_w && h == grid_h && d == grid_d && 256 == bpp );
        fread( img_ptr, 1, w * h * d, f_ptr );
        fclose( f_ptr );

        gfx_texture_update( w, h, d, 1, true, img_ptr, &voxels_tex );
      }
    } else {
      f3_lock = false;
    }
    // Orbit camera.
    //   cam_pos = (vec3){ cam_dist * cosf( curr_s * 0.5f ), cam_height, cam_dist * sinf( curr_s * 0.5f ) };

    uint32_t win_w, win_h, fb_w, fb_h;
    glfwGetWindowSize( gfx.window_ptr, &win_w, &win_h );
    glfwGetFramebufferSize( gfx.window_ptr, &fb_w, &fb_h );
    float aspect = (float)fb_w / (float)fb_h;
    glViewport( 0, 0, win_w, win_h );

    glDepthFunc( GL_LESS );
    glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );

    glClearColor( 0.3f, 0.3f, 0.4f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    mat4 P = perspective( 66.6f, aspect, 0.0001f, 100.0f );
    mat4 V = look_at( cam_pos, (vec3){ 0 }, (vec3){ 0, 1, 0 } );

    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW ); // NB Cube mesh used is inside-out.

    glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_P" ), 1, GL_FALSE, P.m );
    glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_V" ), 1, GL_FALSE, V.m );
    glProgramUniform3fv( shader.program, glGetUniformLocation( shader.program, "u_cam_pos_wor" ), 1, &cam_pos.x );
    glProgramUniform1i( shader.program, glGetUniformLocation( shader.program, "u_n_cells" ), grid_w );
    glProgramUniform1i( shader.program, glGetUniformLocation( shader.program, "u_show_bounding_cube" ), (int)show_bounding_cube );
    glProgramUniform1i( shader.program, glGetUniformLocation( shader.program, "u_vol_tex" ), 0 );
    glProgramUniform1i( shader.program, glGetUniformLocation( shader.program, "u_pal_tex" ), 1 );
    glProgramUniform1iv( shader.program, glGetUniformLocation( shader.program, "u_fullbrights" ), 256, fullbrights );

    const vec3 grid_max = (vec3){ 1, 1, 1 };         // In local grid coord space.
    const vec3 grid_min = (vec3){ -1, -1, -1 };      // In local grid coord space.
    {                                                // Draw first voxel cube.
      mat4 S      = scale_mat4( (vec3){ 1, 1, 1 } ); //((vec3){.5,.5,.5});
      mat4 T      = identity_mat4();//translate_mat4( (vec3){ sinf( curr_s * .5 ), 0, 4 + sinf( curr_s * 2.5 ) } );
      mat4 R      = rot_y_deg_mat4( curr_s * 50.0 );
      mat4 M      = mul_mat4_mat4( T, mul_mat4_mat4( R, S ) ); // Local grid coord space->world coords.
      mat4 M_inv  = inverse_mat4( M );                         // World coords->local grid coord space.
      vec4 cp_loc = mul_mat4_vec4( M_inv, vec4_from_vec3f( cam_pos, 1.0f ) );
      // Still want to render when inside bounding cube area, so flip to rendering inside out. Can't do both at once or it will look wonky.
      if ( cp_loc.x < grid_max.x && cp_loc.x > grid_min.x && cp_loc.y < grid_max.y && cp_loc.y > grid_min.y && cp_loc.z < grid_max.z && cp_loc.z > grid_min.z ) {
        glCullFace( GL_FRONT );
      } else {
        glCullFace( GL_BACK );
      }

      glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_M" ), 1, GL_FALSE, M.m );
      glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_M_inv" ), 1, GL_FALSE, M_inv.m );

      const texture_t* textures[] = { &voxels_tex, &palettes[palette_idx] };
      gfx_draw( shader, cube, textures, 2 );
    } /////////////////////////////////////////////////////

    { // Draw second voxel cube.
      mat4 T      = translate_mat4( (vec3){ 2.1, 0, 0 } );
      mat4 M      = T;
      mat4 M_inv  = inverse_mat4( M );
      vec4 cp_loc = mul_mat4_vec4( M_inv, vec4_from_vec3f( cam_pos, 1.0f ) );

      // Still want to render when inside bounding cube area, so flip to rendering inside out. Can't do both at once or it will look wonky.
      if ( cp_loc.x < grid_max.x && cp_loc.x > grid_min.x && cp_loc.y < grid_max.y && cp_loc.y > grid_min.y && cp_loc.z < grid_max.z && cp_loc.z > grid_min.z ) {
        glCullFace( GL_FRONT );
      } else {
        glCullFace( GL_BACK );
      }
      glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_M" ), 1, GL_FALSE, M.m );
      glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_M_inv" ), 1, GL_FALSE, M_inv.m );
      //    gfx_draw( cube, tex, shader );
    }

    glfwSwapBuffers( gfx.window_ptr );
  }

  gfx_stop();
  free( img_ptr );
  free( my_pal_ptr );
  free( reds_pal_ptr );
  free( blood_pal_ptr );

  printf( "Normal exit.\n" );

  return 0;
}
