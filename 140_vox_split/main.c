/*
===============================================================================
 Voxel Renderer for Magicavoxel .vox files.
 Anton Gerdelan, 31 Dec 2025.
===============================================================================

RUN
=================================

./a.out -v myfile.vox


KEYS
=================================

WASDQE         move camera
cursor keys    rotate camera
Esc            quit


TODO
=================================

 * DONE - save button -> vox format
 * DONE - load vox format
 * DONE - better camera controls
 * DONE - correct render if ray starts inside the bounding cube (needed inside detect & flip cull & change t origin + near clip for regular cam change.)
 * DONE - support scale/translate matrix so >1 voxel mesh can exist in scene.
 * DONE - think about lighting and shading. - the axis should inform which normal to use for shading.
 * DONE - support voxel bounding box rotation. does this break the "uniform grid" idea?
 * DONE - write voxel depth into depth map, not cube sides. and preview depth in a subwindow (otherwise intersections/z fight occur on bounding cube sides).
 * DONE - read MagicaVoxel format https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt
 * DONE - fix coords (x mirrored.).
 * DONE - scale non-square models.
 * DONE - fix scaling of non-square models
 * DONE - fix palette in magicavoxel models ( color [0-254] mapped to palette index [1-255] ) .: PI 0 means air, but other PI -1 to get colour.
 * DONE - use slabs for entry/exit and more exact voxel traverse from RTCD -> can prob tidy inside/outside code with this too.
 * DONE - mouse click to add/remove voxels.
 * TODO - dither for alpha voxels.
 * TODO - vox_fmt support animation frames/models.
 * TODO - figure out slight offset in bounds during C-side grid pick. maybe needs epsilon offset or so along t entry or a <= should be a < or so.
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
#include "ray.h"
#include "vox_fmt.h"
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

typedef struct magvoxel_t {
  uint8_t x, y, z, colour_idx;
} magvoxel_t;

uint8_t* img_ptr = NULL;

texture_t voxels_tex;

/* for reference - monument 10 palette colours:
80= green
96= orange
120=light brown
121=dark brown
160=teal
161=l blue
162=mid brown
255=black
all others=grey (usually 122)
*/
#define ET_NONE 0
#define ET_CREATE 1
#define ET_DELETE 2
#define ET_PAINT 3

static int edit_type = ET_NONE;

// Count of voxels on each slice; horizontal xz, vertical xy, vertical yz.
int *n_voxels_on_nz_slices_ptr = NULL, *n_voxels_on_ny_slices_ptr = NULL, *n_voxels_on_nx_slices_ptr = NULL;

/* NOTE(Anton)
  may not actually need to create 2 separate grid memories/textures.
  could just offset and keep using the original memory/texture with an offset.

*/
bool split        = false;
uint8_t *img_ptr2 = NULL, *img_ptr3 = NULL;
texture_t voxels_tex2, voxels_tex3;
uint32_t w2, h2, d2;
uint32_t w3, h3, d3;

static void _split_grid( int slice_facing, int at_offset, vox_info_t* vox_info_ptr ) {
  if ( split ) { return; } // Just split once, for now.

  uint32_t w = vox_info_ptr->dims_xyz_ptr[0], h = vox_info_ptr->dims_xyz_ptr[2], d = vox_info_ptr->dims_xyz_ptr[1];
  w2 = w, h2 = h, d2 = d;
  uint32_t s3x = 0, s3y = 0, s3z = 0;

  switch ( slice_facing ) {
  case 0:
    printf( "split grid! NX slice at W=%i now =%i\n", at_offset, n_voxels_on_nx_slices_ptr[at_offset] );
    w2  = at_offset;
    s3x = w2 + 1;
    break;
  case 1:
    printf( "split grid! NY slice at H=%i now =%i\n", at_offset, n_voxels_on_ny_slices_ptr[at_offset] );
    h2  = at_offset;
    s3y = h2 + 1;
    break;
  case 2:
    printf( "split grid! NZ slice at D=%i now =%i\n", at_offset, n_voxels_on_nz_slices_ptr[at_offset] );
    d2  = at_offset;
    s3z = d2 + 1;
    break;
  default: assert( false ); break;
  }
  w3 = w, h3 = h, d3 = d;

  // Inefficient method.
  for ( uint32_t z = 0; z < d2; z++ ) {
    for ( uint32_t y = 0; y < h2; y++ ) {
      for ( uint32_t x = 0; x < w2; x++ ) {
        uint32_t vox_idx   = ( z * w * h ) + ( ( ( h - 1 ) - y ) * w ) + x;
        uint32_t vox2_idx  = ( z * w2 * h2 ) + ( ( ( h2 - 1 ) - y ) * w2 ) + x;
        img_ptr2[vox2_idx] = img_ptr[vox_idx];
      }
    }
  }
  gfx_texture_update( w2, h2, d2, 1, true, img_ptr2, &voxels_tex2 );

  for ( uint32_t z = s3z; z < d; z++ ) {
    for ( uint32_t y = s3y; y < h; y++ ) {
      for ( uint32_t x = s3x; x < w; x++ ) {
        uint32_t vox_idx   = ( z * w * h ) + ( ( ( h - 1 ) - y ) * w ) + x;
        uint32_t vox3_idx  = ( ( z - s3z ) * w3 * h3 ) + ( ( ( h3 - 1 ) - ( y - s3y ) ) * w3 ) + ( x - s3x );
        img_ptr3[vox3_idx] = img_ptr[vox_idx];
      }
    }
  }
  gfx_texture_update( w3, h3, d3, 1, true, img_ptr3, &voxels_tex3 );

  split = true;
}

bool visit_cell_cb( int i, int j, int k, int face, void* user_ptr ) {
  if ( !user_ptr ) { return false; } // Returning false indicates grid search should also halt.

  // printf( "visit %i/%i/%i\n", i, j, k );
  // assert( i >= 0 && j >= 0 && k >= 0 );
  vox_info_t vi = *( (vox_info_t*)user_ptr );

  if ( !vi.dims_xyz_ptr ) { return false; }
  uint32_t w = vi.dims_xyz_ptr[0];
  uint32_t h = vi.dims_xyz_ptr[2]; // Convert to my preferred coords.
  uint32_t d = vi.dims_xyz_ptr[1];
  // printf( "w/h/d %u/%u/%u\n", w, h, d );

  i                = APG_CLAMP( i, 0, w - 1 ); // TODO off by 1 error here with 0-72 shown.
  j                = APG_CLAMP( j, 0, h - 1 );
  k                = APG_CLAMP( k, 0, d - 1 );
  uint32_t vox_idx = ( k * w * h ) + ( ( ( h - 1 ) - j ) * w ) + i;
  uint32_t tot     = w * h * d;

  // model cubes are multiple voxels big...confusingly
  // x seems to go == to top bound which is wrong.
  // y seems to go == to top bound which is wrong.
  // z seems to go == to top bound which is wrong.

  // printf( "vox_idx=%u / %u = %f\n", vox_idx, tot, (float)vox_idx / tot );

  uint8_t pal_idx = img_ptr[vox_idx];
  // uint8_t pal_idx = 1;
  if ( 0 != pal_idx ) {
    //  printf( "found vox_idx=%u pal-1=%u at ijk=%i,%i,%i\n", vox_idx, (uint32_t)pal_idx - 1, i, j, k );

    if ( ET_CREATE == edit_type ) {
      int ii = i, jj = j, kk = k;
      switch ( face ) {
      case 1: ii--; break;
      case -1: ii++; break;
      case 2: jj++; break;
      case -2: jj--; break;
      case 3: kk--; break;
      case -3: kk++; break;
      }
      ii                = APG_CLAMP( ii, 0, w - 1 );
      jj                = APG_CLAMP( jj, 0, h - 1 );
      kk                = APG_CLAMP( kk, 0, d - 1 );
      uint32_t vox_idx2 = ( kk * w * h ) + ( ( ( h - 1 ) - jj ) * w ) + ii;
      if ( 0 == img_ptr[vox_idx2] ) {
        n_voxels_on_nx_slices_ptr[ii]++;
        n_voxels_on_ny_slices_ptr[jj]++;
        n_voxels_on_nz_slices_ptr[kk]++;
      }
      img_ptr[vox_idx2] = pal_idx;
    }
    if ( ET_DELETE == edit_type ) {
      // img_ptr[vox_idx] = 0;
      vec3 extras[7] = { { -1, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, -1 }, { 0, 0, 1 } };
      for ( int it = 1; it < 2; it++ ) {
        int ii            = APG_CLAMP( i + extras[it].x, 0, w - 1 );
        int jj            = APG_CLAMP( j + extras[it].y, 0, h - 1 );
        int kk            = APG_CLAMP( k + extras[it].z, 0, d - 1 );
        uint32_t vox_idx2 = ( kk * w * h ) + ( ( ( h - 1 ) - jj ) * w ) + ii;

        /////////////////////////////////////////////////////////////////////////////
        // Check for axis slice-off
        if ( img_ptr[vox_idx2] != 0 ) {
          img_ptr[vox_idx2] = 0;

          n_voxels_on_nx_slices_ptr[ii]--;
          n_voxels_on_ny_slices_ptr[jj]--;
          n_voxels_on_nz_slices_ptr[kk]--;

          if ( n_voxels_on_nx_slices_ptr[ii] <= 0 ) { _split_grid( 0, ii, &vi ); }
          if ( n_voxels_on_ny_slices_ptr[jj] <= 0 ) { _split_grid( 1, jj, &vi ); }
          if ( n_voxels_on_nz_slices_ptr[kk] <= 0 ) { _split_grid( 2, kk, &vi ); }

          n_voxels_on_nx_slices_ptr[ii] = APG_CLAMP( n_voxels_on_nx_slices_ptr[ii], 0, h * d - 1 );
          n_voxels_on_ny_slices_ptr[jj] = APG_CLAMP( n_voxels_on_ny_slices_ptr[jj], 0, w * d - 1 );
          n_voxels_on_nz_slices_ptr[kk] = APG_CLAMP( n_voxels_on_nz_slices_ptr[kk], 0, w * h - 1 );
          // printf( "NX slice at W=%i now =%i\n", ii, n_voxels_on_nx_slices_ptr[ii] );
        }
      }
    }
    if ( ET_PAINT == edit_type ) { img_ptr[vox_idx] = 97; }

    gfx_texture_update( w, h, d, 1, true, img_ptr, &voxels_tex );

    return false; // Returning false indicates grid search should also halt.
  }

  return true;
}

int main( int argc, char** argv ) {
  if ( argc < 2 ) {
    printf( "Usage: ./a.out MY_FILE.vox\n" );
    return 0;
  }

  gfx_t gfx = gfx_start( 800, 600, "3D Texture Demo" );
  if ( !gfx.started ) { return 1; }

  uint32_t grid_w = 32, grid_h = 32, grid_d = 32;
  uint32_t grid_n_chans = 1;
  bool created_voxels   = false;
  img_ptr               = NULL;
  mesh_t cube           = gfx_mesh_cube_create();

  // Load Magicavoxel VOX model
  ////////////////////////////////////////////////////////////////////////////////////////////
  apg_file_t vox      = (apg_file_t){ .sz = 0 };
  vox_info_t vox_info = (vox_info_t){ .dims_xyz_ptr = NULL };
  texture_t vox_pal;
  {
    if ( !vox_fmt_read_file( argv[1], &vox, &vox_info ) ) {
      fprintf( stderr, "ERROR: Failed to read vox file `%s`.\n", argv[1] );
      if ( vox.data_ptr != NULL ) {
        free( vox.data_ptr );
        vox = (apg_file_t){ .sz = 0 };
      }
      return 1;
    }
    if ( vox_info.n_models ) { printf( "n_models=%u (animation frames).\n", *vox_info.n_models ); }

    // Test loaded palette.
    if ( vox_info.rgba_ptr ) { apg_bmp_write( "voxpal.bmp", vox_info.rgba_ptr, 16, 16, 4 ); }

    grid_w = vox_info.dims_xyz_ptr[0];
    grid_h = vox_info.dims_xyz_ptr[2]; // Convert to my preferred coords.
    grid_d = vox_info.dims_xyz_ptr[1];
    printf( "grid w/h/d=%u/%u/%u\n", grid_w, grid_h, grid_d );
    img_ptr                   = calloc( 1, grid_w * grid_h * grid_d * grid_n_chans );
    n_voxels_on_nx_slices_ptr = calloc( 4, grid_h * grid_d ); // Vertical slices front-to-back slices. Slice normal z.
    n_voxels_on_ny_slices_ptr = calloc( 4, grid_w * grid_d ); // Horizontal slices. Slice normal y.
    n_voxels_on_nz_slices_ptr = calloc( 4, grid_w * grid_h ); // Vertical slices side-to-side slices. Slice normal x.

    // For split meshes
    img_ptr2 = calloc( 1, grid_w * grid_h * grid_d * grid_n_chans );
    img_ptr3 = calloc( 1, grid_w * grid_h * grid_d * grid_n_chans );

    if ( !img_ptr ) {
      fprintf( stderr, "ERROR: allocating memory.\n" );
      free( vox.data_ptr );
      return 1;
    }
    for ( uint32_t i = 0; i < *vox_info.n_voxels; i++ ) {
      magvoxel_t* v_ptr               = (magvoxel_t*)&vox_info.voxels_ptr[i * 4];
      uint32_t x                      = v_ptr->x;
      uint32_t y                      = ( grid_h - 1 ) - v_ptr->z; // Convert to my preferred coords.
      uint32_t z                      = ( grid_d - 1 ) - v_ptr->y;
      int idx                         = z * grid_w * grid_h + y * grid_w + x;
      img_ptr[idx * grid_n_chans + 0] = v_ptr->colour_idx;

      n_voxels_on_nx_slices_ptr[x]++;
      n_voxels_on_ny_slices_ptr[grid_h - 1 - y]++;
      n_voxels_on_nz_slices_ptr[z]++;
    }
    vox_pal = gfx_texture_create( 256, 0, 0, 4, false, vox_info.rgba_ptr );

    for ( int i = 0; i < grid_h; i++ ) { printf( "voxels on n_voxels_on_nx_slices_ptr slice %i=%i\n", i, n_voxels_on_nx_slices_ptr[i] ); }

    created_voxels = true;
  }
  ////////////////////////////////////////////////////////////////////////////////////////////

  voxels_tex  = gfx_texture_create( grid_w, grid_h, grid_d, grid_n_chans, true, img_ptr );
  voxels_tex2 = gfx_texture_create( grid_w, grid_h, grid_d, grid_n_chans, true, img_ptr );
  voxels_tex3 = gfx_texture_create( grid_w, grid_h, grid_d, grid_n_chans, true, img_ptr );

  shader_t shader = (shader_t){ .program = 0 };
  if ( !gfx_shader_create_from_file( "cube.vert", "cube.frag", &shader ) ) { return 1; }

  vec3 cam_pos        = (vec3){ 0, 0, 3 };
  float cam_y_rot_deg = 0.0f;
  float cam_speed     = 10.0f;
  float cam_rot_speed = 100.0f; // deg/s

  glfwSwapInterval( 0 );

  bool lmb_lock = false, rmb_lock = false;
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
    vec3 cam_mov_push = (vec3){ 0.0f };
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_ESCAPE ) ) { glfwSetWindowShouldClose( gfx.window_ptr, 1 ); }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_LEFT ) ) { cam_y_rot_deg += cam_rot_speed * elapsed_s; }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_RIGHT ) ) { cam_y_rot_deg -= cam_rot_speed * elapsed_s; }
    mat4 cam_R          = rot_y_deg_mat4( cam_y_rot_deg );
    mat4 cam_iR         = rot_y_deg_mat4( -cam_y_rot_deg );
    vec3 forward_mv_dir = vec3_from_vec4( mul_mat4_vec4( cam_R, (vec4){ 0, 0, -1, 0 } ) );
    vec3 right_mv_dir   = vec3_from_vec4( mul_mat4_vec4( cam_R, (vec4){ 1, 0, 0, 0 } ) );
    vec3 up_mv_dir      = { 0, 1, 0 };
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_A ) ) {
      cam_mov_push = add_vec3_vec3( cam_mov_push, mul_vec3_f( right_mv_dir, -cam_speed * elapsed_s ) );
    }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_D ) ) {
      cam_mov_push = add_vec3_vec3( cam_mov_push, mul_vec3_f( right_mv_dir, cam_speed * elapsed_s ) );
    }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_Q ) ) {
      cam_mov_push = add_vec3_vec3( cam_mov_push, mul_vec3_f( up_mv_dir, -cam_speed * elapsed_s ) );
    }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_E ) ) {
      cam_mov_push = add_vec3_vec3( cam_mov_push, mul_vec3_f( up_mv_dir, cam_speed * elapsed_s ) );
    }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_W ) ) {
      cam_mov_push = add_vec3_vec3( cam_mov_push, mul_vec3_f( forward_mv_dir, cam_speed * elapsed_s ) );
    }
    if ( GLFW_PRESS == glfwGetKey( gfx.window_ptr, GLFW_KEY_S ) ) {
      cam_mov_push = add_vec3_vec3( cam_mov_push, mul_vec3_f( forward_mv_dir, -cam_speed * elapsed_s ) );
    }
    cam_pos      = add_vec3_vec3( cam_pos, cam_mov_push );
    mat4 cam_T   = translate_mat4( cam_pos );
    mat4 cam_iT  = translate_mat4( (vec3){ -cam_pos.x, -cam_pos.y, -cam_pos.z } );
    mat4 V       = mul_mat4_mat4( cam_iR, cam_iT );
    int lmb_down = glfwGetMouseButton( gfx.window_ptr, 0 );
    int rmb_down = glfwGetMouseButton( gfx.window_ptr, 1 );
    int mmb_down = glfwGetMouseButton( gfx.window_ptr, 2 );

    edit_type = ET_NONE;
    if ( lmb_down && !lmb_lock ) {
      edit_type = ET_CREATE;
      lmb_lock  = true;
    }
    if ( rmb_down /*&& !rmb_lock*/ ) {
      edit_type = ET_DELETE;
      rmb_lock  = true;
    }
    if ( mmb_down ) { edit_type = ET_PAINT; }
    if ( !lmb_down ) { lmb_lock = false; }
    if ( !rmb_down ) { rmb_lock = false; }

    uint32_t win_w, win_h, fb_w, fb_h;
    glfwGetWindowSize( gfx.window_ptr, &win_w, &win_h );
    glfwGetFramebufferSize( gfx.window_ptr, &fb_w, &fb_h );
    float aspect = (float)fb_w / (float)fb_h;
    // NB near clip needs to be like 0.001 otherwise transtion from outside<->inside can cull the whole voxel sprite, making it flicker briefly.
    mat4 P     = perspective( 66.6f, aspect, 0.0001f, 1000.0f );
    mat4 P_inv = inverse_mat4( P );
    mat4 V_inv = inverse_mat4( V );

    vec3 m_ray_wor = (vec3){ 0.0f };
    if ( edit_type != ET_NONE ) {
      double xpos, ypos;
      glfwGetCursorPos( gfx.window_ptr, &xpos, &ypos );
      m_ray_wor = ray_wor_from_mouse( xpos, ypos, win_w, win_h, P_inv, V_inv );
    }

    glViewport( 0, 0, win_w, win_h );

    glDepthFunc( GL_LESS );
    glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );

    glClearColor( 0.2f, 0.2f, 0.3f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW ); // NB Cube mesh used is inside-out.

    glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_P" ), 1, GL_FALSE, P.m );
    glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_V" ), 1, GL_FALSE, V.m );
    glProgramUniform3fv( shader.program, glGetUniformLocation( shader.program, "u_cam_pos_wor" ), 1, &cam_pos.x );
    glProgramUniform1i( shader.program, glGetUniformLocation( shader.program, "u_vol_tex" ), 0 );
    glProgramUniform1i( shader.program, glGetUniformLocation( shader.program, "u_pal_tex" ), 1 );

    // TODO put this into  a function and call it twice for the split sub-meshes.
    // TODO try to reuse the original texture twice, with offsets, rather than create new ones.
    { /////////////////////////////////////////////////////
      // TODO(Anton) try local scale==1 voxel per world unit (integer size) - probably avoid fp precision issues tiling larger scenes.
      const float cell_side   = 1.0 / 16.0; // TODO uniform.
      const float cube_length = 2.0;

      vec3 scale_vec = (vec3){ ( grid_w * cell_side ) / cube_length, ( grid_h * cell_side ) / cube_length, ( grid_d * cell_side ) / cube_length };

      if ( split ) { scale_vec = (vec3){ ( w2 * cell_side ) / cube_length, ( h2 * cell_side ) / cube_length, ( d2 * cell_side ) / cube_length }; }

      vec3 grid_max = mul_vec3_vec3( (vec3){ 1, 1, 1 }, scale_vec );    // In local grid coord space.
      vec3 grid_min = mul_vec3_vec3( (vec3){ -1, -1, -1 }, scale_vec ); // In local grid coord space.                               // Draw first voxel cube.
      mat4 S        = identity_mat4(); // Do not scale the box to fit voxel grid dims using the world matrix! It should be scaled in local space.
      mat4 T        = identity_mat4(); // translate_mat4( (vec3){ sinf( curr_s * .5 ), 0, 4 + sinf( curr_s * 2.5 ) } );
      mat4 R        = identity_mat4(); // rot_x_deg_mat4( 90 );
      mat4 M        = mul_mat4_mat4( T, mul_mat4_mat4( R, S ) ); // Local grid coord space->world coords.
      mat4 M_inv    = inverse_mat4( M );                         // World coords->local grid coord space.
      vec4 cp_loc   = mul_mat4_vec4( M_inv, vec4_from_vec3f( cam_pos, 1.0f ) );
      // Still want to render when inside bounding cube area, so flip to rendering inside out. Can't do both at once or it will look wonky.
      if ( cp_loc.x < grid_max.x && cp_loc.x > grid_min.x && cp_loc.y < grid_max.y && cp_loc.y > grid_min.y && cp_loc.z < grid_max.z && cp_loc.z > grid_min.z ) {
        glCullFace( GL_FRONT );
      } else {
        glCullFace( GL_BACK );
      }

      if ( edit_type != ET_NONE ) {
        vec4 pos = mul_mat4_vec4( M, (vec4){ 0.0f } );
        vec4 u   = mul_mat4_vec4( R, (vec4){ 1.0f, 0.0f, 0.0f, 0.0f } );
        vec4 v   = mul_mat4_vec4( R, (vec4){ 0.0f, 1.0f, 0.0f, 0.0f } );
        vec4 w   = mul_mat4_vec4( R, (vec4){ 0.0f, 0.0f, 1.0f, 0.0f } );

        float t = 0.0f, t2 = 0.0f;
        int face_num = 0;
        bool hit     = ray_obb2(
          (obb_t){
                .centre        = vec3_from_vec4( pos ),                                            //
                .half_lengths  = { scale_vec.x, scale_vec.y, scale_vec.z },                        //
                .norm_side_dir = { vec3_from_vec4( u ), vec3_from_vec4( v ), vec3_from_vec4( w ) } //
          },
          cam_pos, m_ray_wor, &t, &face_num, &t2 );
        if ( hit ) {
          // 1. detect bounding box hit
          vec3 hit_pos = add_vec3_vec3( cam_pos, mul_vec3_f( m_ray_wor, t ) );
          //       printf( "HIT: t=%f t2=%f face_num=%i pos=(%.2f,%.2f,%.2f)\n", t, t2, face_num, hit_pos.x, hit_pos.y, hit_pos.z );

          // 2. ray_voxel_grid find voxel hit. -grid min so that i,j.k start at 0,0,0 and only go positive.
          vec3 entry_xyz = sub_vec3_vec3( add_vec3_vec3( cam_pos, mul_vec3_f( m_ray_wor, t ) ), grid_min );
          vec3 exit_xyz  = sub_vec3_vec3( add_vec3_vec3( cam_pos, mul_vec3_f( m_ray_wor, t2 ) ), grid_min );
          //   print_vec3( entry_xyz );
          //   print_vec3( exit_xyz );
          ray_uniform_3d_grid( entry_xyz, exit_xyz, cell_side, visit_cell_cb, &vox_info );

          // 3. modify voxel texture and update.
        }
      }

      glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_M" ), 1, GL_FALSE, M.m );
      glProgramUniformMatrix4fv( shader.program, glGetUniformLocation( shader.program, "u_M_inv" ), 1, GL_FALSE, M_inv.m );

      glProgramUniform3fv( shader.program, glGetUniformLocation( shader.program, "u_shape" ), 1, &scale_vec.x );

      if ( !split ) {
        glProgramUniform3i( shader.program, glGetUniformLocation( shader.program, "u_n_cells" ), grid_w, grid_h, grid_d );
        const texture_t* textures[] = { &voxels_tex, &vox_pal };
        gfx_draw( shader, cube, textures, 2 );
      } else {
        glProgramUniform3i( shader.program, glGetUniformLocation( shader.program, "u_n_cells" ), w2, h2, d2 );
        const texture_t* textures[] = { &voxels_tex2, &vox_pal };
        gfx_draw( shader, cube, textures, 2 );
      }
    } /////////////////////////////////////////////////////

    glfwSwapBuffers( gfx.window_ptr );
  }

  gfx_stop();
  free( img_ptr );
  if ( vox.data_ptr ) { free( vox.data_ptr ); }

  printf( "Normal exit.\n" );

  return 0;
}
