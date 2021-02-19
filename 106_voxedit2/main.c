#include "camera.h"
#include "gfx.h"
#include "input.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* create a 16x256x16 array of blocks */
#define CHUNK_X 16
#define CHUNK_Y 32
#define CHUNK_Z 16
#define N_VOXELS ( CHUNK_X * CHUNK_Y * CHUNK_Z )
uint8_t voxels[N_VOXELS];
float positions[N_VOXELS * 3];
int types[N_VOXELS];
int n_positions       = 0;
int selected_type_idx = 0;

gfx_buffer_t voxel_buffers[2];
void update_buffers() {
  memset( types, 0, sizeof( int ) * N_VOXELS );
  memset( positions, 0, sizeof( float ) * N_VOXELS * 3 );
  n_positions = 0;
  for ( int y = 0; y < CHUNK_Y; y++ ) {
    for ( int z = 0; z < CHUNK_Z; z++ ) {
      for ( int x = 0; x < CHUNK_X; x++ ) {
        int idx = y * ( CHUNK_X * CHUNK_Z ) + z * CHUNK_Z + x;
        if ( 0 == voxels[idx] ) { continue; }
        positions[n_positions * 3 + 0] = x;
        positions[n_positions * 3 + 1] = y;
        positions[n_positions * 3 + 2] = z;
        types[n_positions]             = voxels[idx];
        n_positions++;
      }
    }
  }
  gfx_buffer_update( &voxel_buffers[0], positions, 3, n_positions );
  gfx_buffer_update( &voxel_buffers[1], types, 1, n_positions );
}

bool save_vox( const char* filename ) {
  FILE* f_ptr = fopen( filename, "wb" );
  fwrite( voxels, sizeof( uint8_t ), N_VOXELS, f_ptr );
  fclose( f_ptr );
  return true;
}

bool load_vox( const char* filename ) {
  FILE* f_ptr = fopen( filename, "rb" );
  fread( voxels, sizeof( uint8_t ), N_VOXELS, f_ptr );
  fclose( f_ptr );
  update_buffers();
  return true;
}

bool save_ply( const char* filename ) {
  // alloc memory
  // TODO based on n_positions
  // create a list of triangles
  for ( int y = 0; y < CHUNK_Y; y++ ) {
    for ( int z = 0; y < CHUNK_Z; z++ ) {
      for ( int x = 0; y < CHUNK_X; x++ ) {
        int idx = y * ( CHUNK_X * CHUNK_Z ) + z * CHUNK_Z + x;
        if ( voxels[idx] == 0 || voxels[idx] == 255 ) { continue; }
//       TODO calc required faces -- this should really be a separate function
//
      }
    }
  }

  return true;
}

vec3 ray_d_wor_from_mouse( mat4 inv_P, mat4 inv_V ) {
  int win_x = 0, win_y = 0;
  gfx_window_dims( &win_x, &win_y );
  vec4 ray_eye = mult_mat4_vec4( inv_P, ( vec4 ){ ( 2.0f * input_mouse_x_win ) / win_x - 1.0f, 1.0f - ( 2.0f * input_mouse_y_win ) / win_y, -1.0, 1.0 } );
  vec4 tmp_wor = mult_mat4_vec4( inv_V, ( vec4 ){ ray_eye.x, ray_eye.y, -1.0, 0.0 } );
  vec3 ray_wor = normalise_vec3( v3_v4( tmp_wor ) );
  return ray_wor;
}

// if we transform the ray from world to local space we can use the same function for any chunk
int xx, yy, zz;
int face_num = 0;
bool raycast_voxel( vec3 ray_o, vec3 ray_d, mat4 inv_M ) {
  vec3 ray_d_loc = normalise_vec3( v3_v4( mult_mat4_vec4( inv_M, v4_v3f( ray_d, 0.0f ) ) ) );
  vec3 ray_o_loc = v3_v4( mult_mat4_vec4( inv_M, v4_v3f( ray_o, 1.0f ) ) );
  print_vec3( ray_d_loc );
  print_vec3( ray_o );
  print_vec3( ray_o_loc );

  // check entire chunk
  bool hit = ray_aabb( ray_o_loc, ray_d_loc, ( vec3 ){ -0.5, -0.5, -0.5 }, ( vec3 ){ CHUNK_X - 1 + 0.5, CHUNK_Y - 1 + 0.5, CHUNK_Z - 1 + 0.5 }, 0.1, 1000 );
  if ( !hit ) { return false; }
  // check 8 subdivisions
  // hit = ray_obb( box, ray_o, ray_d, &t, &face_num );

  // HACK
  float d_closest = 0.0f;
  int idx_closest = -1;
  for ( int y = CHUNK_Y - 1; y >= 0; y-- ) {
    for ( int z = 0; z < CHUNK_Z; z++ ) {
      for ( int x = 0; x < CHUNK_X; x++ ) {
        int idx = y * ( CHUNK_X * CHUNK_Z ) + z * CHUNK_Z + x;
        if ( 0 == voxels[idx] ) { continue; }

        bool minihit = ray_aabb( ray_o_loc, ray_d_loc, ( vec3 ){ -0.5 + x, -0.5 + y, -0.5 + z }, ( vec3 ){ x + 0.5, y + 0.5, z + 0.5 }, 1, 100 );
        if ( minihit ) {
          float d = length2_vec3( ( vec3 ){ x - ray_o_loc.x, y - ray_o_loc.y, z - ray_o_loc.z } );
          if ( -1 == idx_closest || d < d_closest ) {
            idx_closest = idx;
            d_closest   = d;
            xx          = x;
            yy          = y;
            zz          = z;
          }
        }
      }
    }
  }
  if ( d_closest != -1 ) {
    printf( "hit x %i y %i z %i\n", xx, yy, zz );

    // TODO(Anton) slow AABB ray to pick face -- store the face because we'll build on that, not just on top

    float t = 0.0;
    ray_obb(
      ( obb_t ){
        .centre        = ( vec3 ){ xx, yy, zz },                                           //
        .half_lengths  = { 0.5, 0.5, 0.5 },                                                //
        .norm_side_dir = { ( vec3 ){ 1, 0, 0 }, ( vec3 ){ 0, 1, 0 }, ( vec3 ){ 0, 0, 1 } } //
      },
      ray_o_loc, ray_d_loc, &t, &face_num //
    );

    return true;
  }

  return hit;
}

void reset_chunk() {
  memset( voxels, 0, sizeof( uint8_t ) * N_VOXELS );
  memset( types, 0, sizeof( int ) * N_VOXELS );
  memset( positions, 0, sizeof( float ) * N_VOXELS * 3 );
  n_positions = 0;
  // floor has type 1
  for ( int z = 0; z < CHUNK_Z; z++ ) {
    for ( int x = 0; x < CHUNK_X; x++ ) {
      int y                          = 0;
      int idx                        = y * ( CHUNK_X * CHUNK_Z ) + z * CHUNK_Z + x;
      voxels[idx]                    = 1;
      positions[n_positions * 3 + 0] = x;
      positions[n_positions * 3 + 1] = y;
      positions[n_positions * 3 + 2] = z;
      types[n_positions]             = 1;
      n_positions++;
    }
  }
}

int my_argc;
char** my_argv;

static int _find_arg( char* str ) {
  for ( int i = 0; i < my_argc; i++ ) {
    if ( strcmp( str, my_argv[i] ) == 0 ) { return i; }
  }
  return -1;
}

int main( int argc, char** argv ) {
  my_argc    = argc;
  my_argv    = argv;
  int dash_i = _find_arg( "-i" );
  int dash_o = _find_arg( "-o" );
  int dash_t = _find_arg( "-t" );
  if ( _find_arg( "--help" ) > -1 ) {
    printf( "Usage: %s [-i INPUT_FILE] [-o OUTPUT_FILE] [-t TILE_ATLAS_IMAGE]\n", argv[0] );
    return 0;
  }
  char input_file[128], output_file[128], texture_file[128];
  sprintf( input_file, "my.vox" );
  sprintf( output_file, "my.vox" );
  sprintf( texture_file, "texture.png" );
  if ( dash_i >= 0 && dash_i < my_argc - 1 ) { sprintf( input_file, argv[dash_i + 1] ); }
  if ( dash_o >= 0 && dash_o < my_argc - 1 ) { sprintf( output_file, argv[dash_o + 1] ); }
  if ( dash_t >= 0 && dash_t < my_argc - 1 ) { sprintf( texture_file, argv[dash_t + 1] ); }
  gfx_start( "Voxedit2 by Anton Gerdelan", NULL, false );
  printf( "Voxedit2 by Anton Gerdelan\ninput file=`%s`\noutput file=`%s`\ntexture=`%s`\n", input_file, output_file, texture_file );

  input_init();

  int fb_w = 0, fb_h = 0;
  gfx_framebuffer_dims( &fb_w, &fb_h );
  camera_t cam = create_cam( (float)fb_w / (float)fb_h );
  cam.pos      = ( vec3 ){ .x = 25, .y = 8, .z = 25 };
  cam.x_degs   = 0; //-45;
  cam.y_degs   = 45;
  cam.speed *= 4.0;

  gfx_shader_t shader = gfx_create_shader_program_from_files( "instanced.vert", "instanced.frag" );
  if ( shader.program_gl == 0 ) { return 1; }
  gfx_texture_t texture     = gfx_texture_create_from_file( texture_file, ( gfx_texture_properties_t ){ .bilinear = 0, .is_srgb = false } );
  gfx_texture_t sel_texture = gfx_texture_create_from_file( "selected.png", ( gfx_texture_properties_t ){ .bilinear = true, .is_srgb = false } );
  gfx_mesh_t mesh           = gfx_mesh_create_from_ply( "unit_cube.ply" );
  if ( mesh.n_vertices == 0 ) { return 1; }
  reset_chunk();
  voxel_buffers[0] = gfx_buffer_create( positions, 3, n_positions, true, false );
  voxel_buffers[1] = gfx_buffer_create( types, 1, n_positions, true, true );

  mat4 M     = scale_mat4( ( vec3 ){ 1, 1, 1 } );
  mat4 inv_M = inverse_mat4( M );

  double prev_s = gfx_get_time_s();
  while ( !gfx_should_window_close() ) {
    double curr_s    = gfx_get_time_s();
    double elapsed_s = curr_s - prev_s;
    prev_s           = curr_s;

    int win_x = 0, win_y = 0;
    gfx_window_dims( &win_x, &win_y );
    gfx_framebuffer_dims( &fb_w, &fb_h );
    gfx_viewport( 0, 0, fb_w, fb_h );
    gfx_clear_colour_and_depth_buffers( 0.2f, 0.2f, 0.2f, 1.0f );

    recalc_cam_V( &cam );
    recalc_cam_P( &cam, (float)fb_w / (float)fb_h );
    mat4 inv_P = inverse_mat4( cam.P );
    mat4 inv_V = inverse_mat4( cam.V );

    {
      gfx_uniform4f( &shader, shader.u_tint, 1, 1, 1, 1 );

      /* render a single cube for each, with instancing */
      int n_instances = n_positions;
      gfx_draw_mesh_instanced( &shader, cam.P, cam.V, M, mesh.vao, mesh.n_vertices, &texture, 1, n_instances, voxel_buffers, 2 );
    }

    vec2 picker_scale = ( vec2 ){ 1024 / (float)fb_w * 0.5f, 1024 / (float)fb_h * 0.5f };
    vec2 picker_pos   = ( vec2 ){ -1.0 + picker_scale.x, 1.0 - picker_scale.y };
    { /* pop up tile chooser */
      gfx_depth_testing( false );
      gfx_draw_textured_quad( texture, picker_scale, picker_pos, ( vec2 ){ 1, 1 }, ( vec4 ){ 1, 1, 1, 1 } );

      gfx_alpha_testing( true );
      // and show currently selected type by drawing a box over the image
      vec2 sel_scale = ( vec2 ){ picker_scale.x / 16, picker_scale.y / 16 };
      vec2 sel_pos = ( vec2 ){ -1.0 + sel_scale.x * ( selected_type_idx % 16 ) * 2 + sel_scale.x, 1.0 - sel_scale.y * ( selected_type_idx / 16 ) * 2 - sel_scale.y };
      float v      = fabsf( sinf( curr_s * 4 ) );
      gfx_draw_textured_quad( sel_texture, sel_scale, sel_pos, ( vec2 ){ 1, 1 }, ( vec4 ){ v, v, v, 1 } );
      gfx_alpha_testing( false );

      gfx_depth_testing( true );
    }

    gfx_swap_buffer();
    input_reset_last_polled_input_states();
    gfx_poll_events();

    /* button to save in either ply or a voxel format (just dims and tile types) */
    if ( input_was_key_pressed( input_save_key ) ) {
      printf( "saving...\n" );
      save_vox( output_file );
    }
    /* button to load from voxel format */
    if ( input_was_key_pressed( input_load_key ) ) {
      printf( "loading...\n" );
      load_vox( input_file );
      // TODO validate
      // TODO update buffer used for palettes
    }
    if ( input_was_key_pressed( input_quicksave_key ) ) {
      printf( "saving out.ply" );
      save_ply( "out.ply" );
    }
    if ( input_was_key_pressed( input_screenshot_key ) ) {
      printf( "screenshot...\n" );
      gfx_screenshot();
    }

    cam_update_keyboard_controls( &cam, elapsed_s );

    bool over_picker = false;
    float mmx        = input_mouse_x_win / (float)win_x * 2.0f - 1.0f;
    float mmy        = -( input_mouse_y_win / (float)win_y * 2.0f - 1.0f );
    if ( mmx < -1.0 + picker_scale.x * 2 && mmy > 1.0 - picker_scale.y * 2 ) { over_picker = true; }

    /* oct-tree raycast function within chunk. ignore air tiles. */
    if ( input_lmb_clicked() ) {
      if ( over_picker ) {
        float xp     = ( mmx + 1.0 ) / ( picker_scale.x * 2 );
        float top    = 1.0;
        float bottom = 1.0 - picker_scale.y * 2.0;
        float yrange = top - bottom;
        float ypn    = ( mmy - bottom ) / yrange;
        float yp     = 1.0 - ypn;
        //        float yp          = ( mmy + 1.0 ) / ( picker_scale.y * 2 );
        int xx            = (int)( xp / ( 1.0 / 16.0 ) );
        int yy            = (int)( yp / ( 1.0 / 16.0 ) );
        selected_type_idx = yy * 16 + xx;
        printf( "selected type = %i\n", selected_type_idx );
      } else {
        vec3 ray_d_wor = ray_d_wor_from_mouse( inv_P, inv_V );
        if ( raycast_voxel( cam.pos, ray_d_wor, inv_M ) ) {
          printf( "HIT. face %i\n", face_num );
          switch ( face_num ) {
          case 1: {
            if ( xx < CHUNK_X - 1 ) {
              int idx     = yy * ( CHUNK_X * CHUNK_Z ) + zz * CHUNK_Z + xx + 1;
              voxels[idx] = (uint8_t)selected_type_idx;
              update_buffers();
            }
          } break;
          case 2: {
            if ( yy < CHUNK_Y - 1 ) {
              int idx     = ( yy + 1 ) * ( CHUNK_X * CHUNK_Z ) + zz * CHUNK_Z + xx;
              voxels[idx] = (uint8_t)selected_type_idx;
              update_buffers();
            }
          } break;
          case 3: {
            if ( zz < CHUNK_Z - 1 ) {
              int idx     = yy * ( CHUNK_X * CHUNK_Z ) + ( zz + 1 ) * CHUNK_Z + xx;
              voxels[idx] = (uint8_t)selected_type_idx;
              update_buffers();
            }
          } break;
          case -1: {
            if ( xx > 0 ) {
              int idx     = yy * ( CHUNK_X * CHUNK_Z ) + zz * CHUNK_Z + xx - 1;
              voxels[idx] = (uint8_t)selected_type_idx;
              update_buffers();
            }
          } break;
          case -2: {
            if ( yy > 0 ) {
              int idx     = ( yy - 1 ) * ( CHUNK_X * CHUNK_Z ) + zz * CHUNK_Z + xx;
              voxels[idx] = (uint8_t)selected_type_idx;
              update_buffers();
            }
          } break;
          case -3: {
            if ( zz > 0 ) {
              int idx     = yy * ( CHUNK_X * CHUNK_Z ) + ( zz - 1 ) * CHUNK_Z + xx;
              voxels[idx] = (uint8_t)selected_type_idx;
              update_buffers();
            }
          } break;
          } // endsw
        } else {
          printf( "MISS\n" );
        }
      } // endif over picker
    }   // endif lmb

    if ( input_rmb_clicked() && !over_picker ) {
      vec3 ray_d_wor = ray_d_wor_from_mouse( inv_P, inv_V );
      if ( raycast_voxel( cam.pos, ray_d_wor, inv_M ) ) {
        int idx     = yy * ( CHUNK_X * CHUNK_Z ) + zz * CHUNK_Z + xx;
        voxels[idx] = 0;
        update_buffers();
      }
    }

  } // endwhile

  gfx_delete_shader_program( &shader );
  gfx_delete_mesh( &mesh );
  gfx_delete_texture( &texture );
  gfx_buffer_delete( &voxel_buffers[0] );
  gfx_buffer_delete( &voxel_buffers[1] );
  gfx_stop();

  printf( "normal exit\n" );
  return 0;
}
