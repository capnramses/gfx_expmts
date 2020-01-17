
// Use discrete GPU by default. not sure if it works on other OS. if in C++ must be in extern "C" { } block
#ifdef _WIN32
// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
__declspec( dllexport ) unsigned long int NvOptimusEnablement = 0x00000001;
// https://gpuopen.com/amdpowerxpressrequesthighperformance/
__declspec( dllexport ) int AmdPowerXpressRequestHighPerformance = 1;
#endif

/* demo motivations

- DONE create/delete a chunk's worth of voxels with the mouse lmb rmb
- DONE use 1-5 keys to choose a block type
- DONE reset button
- DONE export to ply with correct colours
- DONE export to custom vxl type with palette indices
*/

#include "apg_maths.h"
#include "apg_pixfont.h"
#include "apg_ply.h"
#include "apg_tga.h"
#include "camera.h"
#include "diamond_square.h"
#include "glcontext.h"
#include "gl_utils.h"
#include "input.h"
#include "voxels.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../common/include/stb/stb_image.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHUNKS_N 256
static int _chunks_w = 16, _chunks_h = 16;

char images[16][256] = { "textures/floor_grass.png", "textures/floor_stone.png", "textures/side_grass.png", "textures/hersk-export.png" };

int main() {
  if ( !start_gl( "Voxedit by Anton Gerdelan" ) ) { return 1; }
  init_input();

  texture_t array_texture = ( texture_t ){ .handle_gl = 0, .w = 16, .h = 16, .n_channels = 3, .srgb = false, .is_depth = false, .is_array = true };

  {
    GLsizei layerCount    = 4;
    GLsizei mipLevelCount = 5;
    glGenTextures( 1, &array_texture.handle_gl );
    glBindTexture( GL_TEXTURE_2D_ARRAY, array_texture.handle_gl );
    // allocate memory for all layers
    glTexStorage3D( GL_TEXTURE_2D_ARRAY, mipLevelCount, GL_RGB8, array_texture.w, array_texture.h, layerCount ); // 8?
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    for ( int i = 0; i < layerCount; i++ ) {
      int w, h, n;
      uint8_t* img = stbi_load( images[i], &w, &h, &n, 3 );
      if ( !img ) { return 1; }
      assert( img );
      assert( w == array_texture.w && h == array_texture.h );

      glTexSubImage3D( GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, w, h, 1, GL_RGB, GL_UNSIGNED_BYTE, img ); // note that 'depth' param must be 1
      free( img );
    }
    // once all textures loaded at mip 0 - generate other mipmaps
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR ); // nearest but with 2-linear mipmap blend
    glTexParameterf( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f );
    glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
    glBindTexture( GL_TEXTURE_2D_ARRAY, 0 );
  }
  // TODO create array texture here

  mesh_t chunk_meshes[CHUNKS_N];
  chunk_t chunks[CHUNKS_N];
  mat4 chunks_M[CHUNKS_N];

  shader_t colour_picking_shader;

  uint32_t seed = time( NULL );
  printf( "seed = %u\n", seed );
  srand( seed );
  assert( CHUNK_X == CHUNK_Z );
  int default_height     = 63;
  int noise_scale        = 64;
  int feature_spread     = 64;
  int feature_max_height = 64;
  assert( _chunks_w == _chunks_h );
  dsquare_heightmap_t dshm = dsquare_heightmap_alloc( CHUNK_X * _chunks_w, default_height );
  dsquare_heightmap_gen( &dshm, noise_scale, feature_spread, feature_max_height );
  const float voxel_scale = 0.2f;

  for ( int cz = 0; cz < _chunks_h; cz++ ) {
    for ( int cx = 0; cx < _chunks_w; cx++ ) {
      int idx     = cz * _chunks_w + cx;
      chunks[idx] = chunk_generate( dshm.filtered_heightmap, dshm.w, dshm.h, cx * CHUNK_X, cz * CHUNK_Z );
      {
        chunk_vertex_data_t vertex_data = chunk_gen_vertex_data( &chunks[idx] );
        chunk_meshes[idx]               = create_mesh_from_mem( vertex_data.positions_ptr, vertex_data.n_vp_comps, vertex_data.palette_indices_ptr,
          vertex_data.n_vpalidx_comps, vertex_data.picking_ptr, vertex_data.n_vpicking_comps, vertex_data.texcoords_ptr, vertex_data.n_vt_comps,
          vertex_data.normals_ptr, vertex_data.n_vn_comps, vertex_data.n_vertices );
        chunk_free_vertex_data( &vertex_data );
      }
      chunks_M[idx] = translate_mat4( ( vec3 ){ .x = cx * CHUNK_X * voxel_scale, .z = cz * CHUNK_Z * voxel_scale } );
    }
  }

  texture_t text_texture;
  {
    int w, h;
    int n_channels = 1;
    int thickness  = 1;
    bool outlines  = true;
    bool vflip     = false;
    if ( APG_PIXFONT_FAILURE == apg_pixfont_image_size_for_str( "my_string", &w, &h, thickness, outlines ) ) {
      fprintf( stderr, "ERROR apg_pixfont_image_size_for_str\n" );
      return 1;
    }
    unsigned char* img_mem = (unsigned char*)malloc( w * h * n_channels );
    memset( img_mem, 0x00, w * h * n_channels );
    if ( APG_PIXFONT_FAILURE == apg_pixfont_str_into_image( "my_string", img_mem, w, h, n_channels, 0xFF, 0x7F, 0x00, 0xFF, thickness, outlines, vflip ) ) {
      fprintf( stderr, "ERROR apg_pixfont_image_size_for_str\n" );
      return 1;
    }
    text_texture = create_texture_from_mem( img_mem, w, h, n_channels, false, false, false );
    free( img_mem );
  }
  shader_t voxel_shader;
  {
    const char vert_shader_str[] = {
      "#version 410\n"
      "in vec3 a_vp;\n"
      "in vec2 a_vt;\n"
      "in vec4 a_vn;\n"
      "in uint a_vpal_idx;\n"
      "uniform mat4 u_P, u_V, u_M;\n"
      "out vec2 v_st;\n"
      "out vec4 v_n;\n"
      "out vec3 v_p_eye;\n"
      "flat out uint v_vpal_idx;\n"
      "void main () {\n"
      "  v_vpal_idx = a_vpal_idx;\n"
      "  v_st = a_vt;\n"
      "  v_n.xyz = (u_M * vec4( a_vn.xyz, 0.0 )).xyz;\n"
      "  v_n.w = a_vn.w;\n"
      "  v_p_eye =  (u_V * u_M * vec4( a_vp * 0.1, 1.0 )).xyz;\n"
      "  gl_Position = u_P * vec4( v_p_eye, 1.0 );\n"
      "}\n"
    };
    // heightmap 0 or 1 factor is stored in normal's w channel. used to disable sunlight for rooms/caves/overhangs (assumes sun is always _directly_ overhead,
    // pointing down)
    const char frag_shader_str[] = {
      "#version 410\n"
      "in vec2 v_st;\n"
      "in vec4 v_n;\n"
      "in vec3 v_p_eye;\n"
      "flat in uint v_vpal_idx;\n"
      "uniform sampler2DArray u_palette_texture;\n"
      "uniform vec3 u_fwd;\n"
      "out vec4 o_frag_colour;\n"
      "vec3 sun_rgb = vec3( 1.0, 1.0, 1.0 );\n"
      "vec3 fwd_rgb = vec3( 1.0, 1.0, 1.0 );\n"
      "vec3 fog_rgb = vec3( 0.5, 0.5, 0.9 );\n"
      "void main () {\n"
      "  vec3 texel_rgb    = texture( u_palette_texture, vec3( v_st.s, 1.0 - v_st.t, v_vpal_idx ) ).rgb;\n"
      "  float fog_fac      = clamp( v_p_eye.z * v_p_eye.z / 1000.0, 0.0, 1.0 );\n"
      "  vec3 col           = pow( texel_rgb, vec3( 2.2 ) ); \n" // tga image load is linear colour space already w/o gamma
      "  float sun_dp       = clamp( dot( normalize( v_n.xyz ), normalize( -vec3( -0.3, -1.0, 0.2 ) ) ), 0.0 , 1.0 );\n"
      "  float fwd_dp       = clamp( dot( normalize( v_n.xyz ), -u_fwd ), 0.0, 1.0 );\n"
      "  float outdoors_fac = v_n.w;\n"
      "  o_frag_colour      = vec4( sun_rgb * col * sun_dp * outdoors_fac * 0.9 + col * 0.1, 1.0f );\n"
      "  o_frag_colour.rgb  = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );\n"
      "  o_frag_colour.rgb  = mix(o_frag_colour.rgb, fog_rgb, fog_fac);\n"
      "}\n"
    };
    // NOTE(Anton) fog calc after rgb because colour is trying to match gl clear colour
    voxel_shader = create_shader_program_from_strings( vert_shader_str, frag_shader_str );
  }
  {
    const char vert_shader_str[] = {
      "#version 410\n"
      "in vec3 a_vp;\n"
      "in vec3 a_vpicking;\n"
      "uniform mat4 u_P, u_V, u_M;\n"
      "out vec3 v_vpicking;\n"
      "void main () {\n"
      "  v_vpicking = a_vpicking;\n"
      "  gl_Position = u_P * u_V * u_M * vec4( a_vp * 0.1, 1.0 );\n"
      "}\n"
    };
    const char frag_shader_str[] = {
      "#version 410\n"
      "in vec3 v_vpicking;\n"
      "uniform float u_chunk_id;\n"
      "out vec4 o_frag_colour;\n"
      "void main () {\n"
      "  o_frag_colour = vec4( v_vpicking, u_chunk_id );\n"
      "}\n"
    };
    colour_picking_shader = create_shader_program_from_strings( vert_shader_str, frag_shader_str );
  }
  unsigned char* fps_img_mem = NULL;
  int fps_img_w = 256, fps_img_h = 256;
  int fps_n_channels = 4;
  { // allocate memory for FPS image

    fps_img_mem = (unsigned char*)calloc( fps_img_w * fps_img_h * fps_n_channels, 1 );
    if ( !fps_img_mem ) {
      fprintf( stderr, "ERROR: calloc returned NULL\n" );
      return 1;
    }
  }

  int fb_width, fb_height;
  framebuffer_dims( &fb_width, &fb_height );

  framebuffer_t picking_fb = create_framebuffer( fb_width, fb_height );
  assert( picking_fb.built );

  printf( "displaying...\n" );
  double prev_time  = get_time_s();
  double text_timer = 0.2;
  camera_t cam      = create_cam( (float)fb_width / (float)fb_height );
  {
    cam.pos = ( vec3 ){ .x = 1.6f, .y = 5.0f, .z = 5.0f };
    vec3 up = normalise_vec3( ( vec3 ){ .x = 0.5f, .y = 0.5f, .z = -0.5f } );
    cam.V   = look_at( cam.pos, ( vec3 ){ .x = 0, .y = 0, .z = 0 }, up );
  }

  block_type_t block_type_to_create = BLOCK_TYPE_STONE;
  char hovered_voxel_str[256];
  hovered_voxel_str[0] = '\0';
  int picked_x = -1, picked_y = -1, picked_z = -1, picked_face = -1, picked_chunk_id = -1;
  bool picked = false;

  vec2 text_scale = ( vec2 ){ .x = 1, .y = 1 }, text_pos = ( vec2 ){ .x = 0 };
  while ( !should_window_close() ) {
    int fb_width = 0, fb_height = 0, win_width = 0, win_height = 0;
    double mouse_x = 0, mouse_y = 0;
    double curr_time = get_time_s();
    double elapsed_s = curr_time - prev_time;
    prev_time        = curr_time;
    text_timer += elapsed_s;

    reset_last_polled_input_states();
    poll_events();

    { // TODO(anton) only do when actually resized
      framebuffer_dims( &fb_width, &fb_height );
      window_dims( &win_width, &win_height );
      rebuild_framebuffer( &picking_fb, fb_width, fb_height );
      assert( picking_fb.built );
    }

    { // get mouse cursor and controls
      mouse_pos_win( &mouse_x, &mouse_y );

      for ( int i = 1; i <= 4; i++ ) {
        if ( was_key_pressed( g_palette_1_key + i - 1 ) ) {
          printf( "block type set to %i\n", i );
          block_type_to_create = (block_type_t)i;
        }
      }

      if ( picked ) {
        bool changed = false;
        if ( lmb_clicked() ) {
          int xx = picked_x, yy = picked_y, zz = picked_z;
          switch ( picked_face ) {
          case 0: xx--; break;
          case 1: xx++; break;
          case 2: yy--; break;
          case 3: yy++; break;
          case 4: zz--; break;
          case 5: zz++; break;
          default: assert( false ); break;
          }
          assert( picked_chunk_id >= 0 && picked_chunk_id < CHUNKS_N );

          int chunk_x = picked_chunk_id % _chunks_w;
          int chunk_z = picked_chunk_id / _chunks_w;

          if ( xx < 0 ) {
            assert( chunk_x > 0 );
            xx = 15;
            chunk_x--;
          }
          if ( xx > 15 ) {
            assert( chunk_x < _chunks_w );
            xx = 0;
            chunk_x++;
          }
          if ( zz < 0 ) {
            assert( chunk_z > 0 );
            zz = 15;
            chunk_z--;
          }
          if ( zz > 15 ) {
            assert( chunk_z < _chunks_h );
            zz = 0;
            chunk_z++;
          }
          picked_chunk_id = chunk_z * _chunks_w + chunk_x;

          changed = set_block_type_in_chunk( &chunks[picked_chunk_id], xx, yy, zz, block_type_to_create );
        } else if ( rmb_clicked() ) {
          changed = set_block_type_in_chunk( &chunks[picked_chunk_id], picked_x, picked_y, picked_z, BLOCK_TYPE_AIR );
        }
        if ( changed ) {
          // TODO(Anton) reuse a scratch buffer to avoid mallocs
          chunk_vertex_data_t vertex_data = chunk_gen_vertex_data( &chunks[picked_chunk_id] );
          // TODO(Anton) and reuse the previous VBOs
          delete_mesh( &chunk_meshes[picked_chunk_id] );
          chunk_meshes[picked_chunk_id] = create_mesh_from_mem( vertex_data.positions_ptr, vertex_data.n_vp_comps, vertex_data.palette_indices_ptr,
            vertex_data.n_vpalidx_comps, vertex_data.picking_ptr, vertex_data.n_vpicking_comps, vertex_data.texcoords_ptr, vertex_data.n_vt_comps,
            vertex_data.normals_ptr, vertex_data.n_vn_comps, vertex_data.n_vertices );
          chunk_free_vertex_data( &vertex_data );
        }
      }
      bool cam_fwd = false, cam_bk = false, cam_left = false, cam_rgt = false, turn_left = false, turn_right = false;
      {
        static double prev_mouse_x  = 0.0;
        static bool not_first_frame = false;
        if ( not_first_frame && mmb_held() ) {
          const double mouse_turn_sensitivity = 100.0;
          double turn_factor                  = mouse_turn_sensitivity * elapsed_s * fabs( mouse_x - prev_mouse_x ) / fb_width; // scale by resolution
          if ( mouse_x > prev_mouse_x ) {
            turn_cam_right( &cam, turn_factor );
          } else if ( mouse_x < prev_mouse_x ) {
            turn_cam_left( &cam, turn_factor );
          }
        }
        not_first_frame = true;
        prev_mouse_x    = g_mouse_x_win;
      }
      if ( is_key_held( g_forwards_key ) ) { cam_fwd = true; }
      if ( is_key_held( g_backwards_key ) ) { cam_bk = true; }
      if ( is_key_held( g_slide_left_key ) ) { cam_left = true; }
      if ( is_key_held( g_slide_right_key ) ) { cam_rgt = true; }
      if ( is_key_held( g_fly_up_key ) ) {
        move_cam_up( &cam, elapsed_s );
        cam.animating_y = false; // if animating a height change, don't let that fight us
      }
      if ( is_key_held( g_fly_down_key ) ) {
        move_cam_dn( &cam, elapsed_s );
        cam.animating_y = false;
      }
      if ( is_key_held( g_turn_left_key ) ) { turn_left = true; }
      if ( is_key_held( g_turn_right_key ) ) { turn_right = true; }
      if ( is_key_held( g_turn_up_key ) ) { turn_cam_up( &cam, elapsed_s ); }
      if ( is_key_held( g_turn_down_key ) ) { turn_cam_down( &cam, elapsed_s ); }

      if ( cam_fwd ) {
        move_cam_fwd( &cam, elapsed_s );
      } else if ( cam_bk ) {
        move_cam_bk( &cam, elapsed_s );
      }
      if ( cam_left ) {
        move_cam_lft( &cam, elapsed_s );
      } else if ( cam_rgt ) {
        move_cam_rgt( &cam, elapsed_s );
      }
      if ( turn_left ) {
        turn_cam_left( &cam, elapsed_s );
      } else if ( turn_right ) {
        turn_cam_right( &cam, elapsed_s );
      }
      recalc_cam_V( &cam );
    }

    clear_colour_and_depth_buffers( 0.5, 0.5, 0.9, 1.0 );
    viewport( 0, 0, fb_width, fb_height );

    uniform3f( voxel_shader, voxel_shader.u_fwd, cam.forward.x, cam.forward.y, cam.forward.z );
    for ( int i = 0; i < CHUNKS_N; i++ ) {
      draw_mesh( voxel_shader, cam.P, cam.V, chunks_M[i], chunk_meshes[i].vao, chunk_meshes[i].n_vertices, &array_texture, 1 );
    }
    // update FPS image every so often
    if ( text_timer > 0.1 ) {
      int w = 0, h = 0; // actual image writing area can be smaller than img dims
      int thickness = 1;
      bool outlines = true;
      bool vflip    = false;
      char string[256];
      if ( elapsed_s == 0.0 ) { elapsed_s = 0.00001; }
      double fps = 1.0 / elapsed_s;

      text_timer = 0.0;
      memset( fps_img_mem, 0x00, fps_img_w * fps_img_h * fps_n_channels );

      sprintf( string,
        "FPS %.2f\n%s\nwin dims (%i,%i). fb dims (%i,%i)\nmouse xy (%.2f,%.2f)\nhovered voxel: %s\n\nLMB,RMB   edit voxels\n1,2,3...     block type\n", fps,
        gfx_renderer_str(), win_width, win_height, fb_width, fb_height, mouse_x, mouse_y, hovered_voxel_str );

      if ( APG_PIXFONT_FAILURE == apg_pixfont_image_size_for_str( string, &w, &h, thickness, outlines ) ) {
        fprintf( stderr, "ERROR apg_pixfont_image_size_for_str\n" );
        return 1;
      }
      if ( APG_PIXFONT_FAILURE == apg_pixfont_str_into_image( string, fps_img_mem, w, h, fps_n_channels, 0xFF, 0x7F, 0x00, 0xFF, thickness, outlines, vflip ) ) {
        fprintf( stderr, "ERROR apg_pixfont_image_size_for_str\n" );
        return 1;
      }
      text_texture = create_texture_from_mem( fps_img_mem, w, h, fps_n_channels, false, false, false );
      text_scale.x = (float)w / fb_width * 2.0f;
      text_scale.y = (float)h / fb_height * 2.0f;
      text_pos.x   = -1.0f + text_scale.x;
      text_pos.y   = 1.0f - text_scale.y;
    }
    draw_textured_quad( text_texture, text_scale, text_pos );

    { // render colour IDs around mouse cursor
      // subwindow rectangle. NOTE THAT FB BUFFER SHOULD STILL BE ORIGINAL BUFFER SIZE.
      int h            = 3;
      int w            = 3;
      int x            = mouse_x - h / 2;
      int y            = fb_height - ( mouse_y + w / 2 );
      mat4 offcentre_P = perspective_offcentre_viewport( fb_width, fb_height, x, y, w, h, cam.P );

      bind_framebuffer( &picking_fb );
      viewport( x, y, w, h );
      // clear to 1,1,1,1 because 0,0,0,0 is voxel number 0
      clear_colour_and_depth_buffers( 1, 1, 1, 1 );
      // clear_depth_buffer();

      for ( uint32_t i = 0; i < CHUNKS_N; i++ ) {
        uniform1f( colour_picking_shader, colour_picking_shader.u_chunk_id, (float)i / 255.0f );
        draw_mesh( colour_picking_shader, offcentre_P, cam.V, chunks_M[i], chunk_meshes[i].vao, chunk_meshes[i].n_vertices, NULL, 0 );
      }
      uint8_t data[4] = { 0, 0, 0, 0 };
      // TODO(Anton) this is blocking cpu/gpu sync and can stall. to speed up use the 2-PBO method perhaps.
      read_pixels( x + w / 2, y + h / 2, 1, 1, 4, data );
      picked = picked_colour_to_voxel_idx( data[0], data[1], data[2], data[3], &picked_x, &picked_y, &picked_z, &picked_face, &picked_chunk_id );
      if ( picked ) {
        sprintf( hovered_voxel_str, "chunk_id=%i voxel=(%i,%i,%i) face=%i", picked_chunk_id, picked_x, picked_y, picked_z, picked_face );
      } else {
        sprintf( hovered_voxel_str, "none" );
      }

      bind_framebuffer( NULL );
    }
    swap_buffer();
  }

  for ( int i = 0; i < 4; i++ ) {
    chunk_free( &chunks[i] );
    delete_mesh( &chunk_meshes[i] );
  }
  free( fps_img_mem );
  stop_gl();
  printf( "HALT\n" );
  return 0;
}
