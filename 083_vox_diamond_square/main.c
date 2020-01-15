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
#include "gl_utils.h"
#include "input.h"
#include "voxels.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// exports to a PLY, baking the correct colours from the palette
bool export_voxel_ply( const char* filename, const chunk_t* chunk ) {
  assert( filename && chunk );

  // load latest v of palette
  int w, h, n;
  uint8_t* palette_img = apg_tga_read_file( "palette.tga", &w, &h, &n, 0 ); // vertically flip
  if ( !palette_img ) { return false; }

  // fetch buffers of vertex data
  chunk_vertex_data_t vertex_data = chunk_gen_vertex_data( chunk );

  // create colours buffer from palette indices buffer
  float* colours_buffer = malloc( sizeof( float ) * 3 * vertex_data.n_vertices );

  // scale down the size
  const float scale = 0.1f;

  for ( uint32_t v = 0; v < vertex_data.n_vertices; v++ ) {
    uint32_t pal_idx          = vertex_data.palette_indices_ptr[v];
    colours_buffer[v * 3 + 0] = (float)palette_img[pal_idx * 3 + 2] / 255.0f;
    colours_buffer[v * 3 + 1] = (float)palette_img[pal_idx * 3 + 1] / 255.0f;
    colours_buffer[v * 3 + 2] = (float)palette_img[pal_idx * 3 + 0] / 255.0f; // pal in BGR

    vertex_data.positions_ptr[v * 3] *= scale;
    vertex_data.positions_ptr[v * 3 + 1] *= scale;
    vertex_data.positions_ptr[v * 3 + 2] *= scale;
  }

  // TODO(Anton) also write out normals? yes?

  // write
  uint32_t r = apg_ply_write( "out.ply", ( apg_ply_t ){ .n_positions_comps = 3,
                                           .n_colours_comps                = 3,
                                           .n_normals_comps                = vertex_data.n_vn_comps,
                                           .positions_ptr                  = vertex_data.positions_ptr,
                                           .colours_ptr                    = colours_buffer,
                                           .n_vertices                     = vertex_data.n_vertices,
                                           .loaded                         = true } );

  // free mem
  free( colours_buffer );
  chunk_free_vertex_data( &vertex_data );
  free( palette_img );

  if ( r != 1 ) { return false; } // error writing but still free mem first

  return true;
}

int main() {
  texture_t palette_tex;
  if ( !start_gl( "Voxedit by Anton Gerdelan" ) ) { return 1; }
  init_input();
  {
    int w, h, n;
    uint8_t* palette_img = apg_tga_read_file( "palette.tga", &w, &h, &n, 0 );
    if ( !palette_img ) { return 1; }
    palette_tex = create_texture_from_mem( palette_img, w, h, n, false, false, true );
    assert( palette_tex.handle_gl );
    free( palette_img );
  }

  mesh_t chunk_meshes[4];
  shader_t colour_picking_shader;

  uint32_t seed = time( NULL );
  printf( "seed = %u\n", seed );
  srand( seed );
  assert( CHUNK_X == CHUNK_Z );
  int default_height       = 1;
  int noise_scale          = 4;
  int feature_spread       = 8;
  int feature_max_height   = 32;
  dsquare_heightmap_t dshm = dsquare_heightmap_alloc( CHUNK_X * 2, default_height );
  dsquare_heightmap_gen( &dshm, noise_scale, feature_spread, feature_max_height );

  int x_offset = 0;
  int z_offset = 0;
  chunk_t chunks[4];
  chunks[0] = chunk_generate( dshm.heightmap, dshm.w, dshm.h, x_offset, z_offset );
  chunks[1] = chunk_generate( dshm.heightmap, dshm.w, dshm.h, x_offset + CHUNK_X, z_offset );
  chunks[2] = chunk_generate( dshm.heightmap, dshm.w, dshm.h, x_offset, z_offset + CHUNK_Z );
  chunks[3] = chunk_generate( dshm.heightmap, dshm.w, dshm.h, x_offset + CHUNK_X, z_offset + CHUNK_Z );
  for ( int i = 0; i < 4; i++ ) {
    chunk_vertex_data_t vertex_data = chunk_gen_vertex_data( &chunks[i] );
    chunk_meshes[i] = create_mesh_from_mem( vertex_data.positions_ptr, vertex_data.n_vp_comps, vertex_data.palette_indices_ptr, vertex_data.n_vpalidx_comps,
      vertex_data.picking_ptr, vertex_data.n_vpicking_comps, vertex_data.normals_ptr, vertex_data.n_vn_comps, vertex_data.n_vertices );
    chunk_free_vertex_data( &vertex_data );
  }
  const float voxel_scale = 0.2f;
  mat4 chunks_M[4];
  chunks_M[0] = identity_mat4();
  chunks_M[1] = translate_mat4( ( vec3 ){ .x = CHUNK_X * voxel_scale } );
  chunks_M[2] = translate_mat4( ( vec3 ){ .z = CHUNK_Z * voxel_scale } );
  chunks_M[3] = translate_mat4( ( vec3 ){ .x = CHUNK_X * voxel_scale, .z = CHUNK_Z * voxel_scale } );

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
      "in vec4 a_vn;\n"
      "in uint a_vpal_idx;\n"
      "uniform sampler2D u_palette_texture;\n"
      "uniform mat4 u_P, u_V, u_M;\n"
      "out vec3 v_c;\n"
      "out vec4 v_n;\n"
      "void main () {\n"
      "  vec2 pal_st = vec2( float(a_vpal_idx) / 16.0, 1.0 );\n"
      //  "  pal_st.t = 1.0 - pal_st.t;\n"
      "  v_c = texture( u_palette_texture, pal_st ).rgb;\n"
      "  v_n.xyz = (u_M * vec4( a_vn.xyz, 0.0 )).xyz;\n"
      "  v_n.w = a_vn.w;\n"
      "  gl_Position = u_P * u_V * u_M * vec4( a_vp * 0.1, 1.0 );\n"
      "}\n"
    };
    // heightmap 0 or 1 factor is stored in normal's w channel. used to disable sunlight for rooms/caves/overhangs (assumes sun is always _directly_ overhead,
    // pointing down)
    const char frag_shader_str[] = {
      "#version 410\n"
      "in vec3 v_c;\n"
      "in vec4 v_n;\n"
      "uniform vec3 u_fwd;\n"
      "out vec4 o_frag_colour;\n"
      "vec3 sun_rgb = vec3( 1.0, 1.0, 1.0 );"
      "vec3 fwd_rgb = vec3( 1.0, 1.0, 1.0 );"
      "void main () {\n"
      "  vec3 col           = v_c; /*pow( v_c, vec3( 2.2 ) );*/ \n" // tga image load is linear colour space already w/o gamma
      "  float sun_dp       = clamp( dot( normalize( v_n.xyz ), normalize( -vec3( -0.3, -1.0, 0.2 ) ) ), 0.0 , 1.0 );\n"
      "  float fwd_dp       = clamp( dot( normalize( v_n.xyz ), -u_fwd ), 0.0, 1.0 );\n"
      "  float outdoors_fac = v_n.w;\n"
      "  o_frag_colour      = vec4( sun_rgb * col * sun_dp * outdoors_fac * 0.7 + col * 0.2 + fwd_dp * col * 0.1, 1.0f );\n"
      "  o_frag_colour.rgb  = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );\n"
      "}\n"
    };
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
          assert( picked_chunk_id >= 0 && picked_chunk_id < 4 );

          // TODO(Anton) replace with generic formula that scales
          if ( xx < 0 ) {
            if ( picked_chunk_id == 1 ) {
              xx              = 15;
              picked_chunk_id = 0;
            } else if ( picked_chunk_id == 3 ) {
              xx              = 15;
              picked_chunk_id = 2;
            }
          }
          if ( zz < 0 ) {
            if ( picked_chunk_id == 2 ) {
              zz              = 15;
              picked_chunk_id = 0;
            } else if ( picked_chunk_id == 3 ) {
              zz              = 15;
              picked_chunk_id = 1;
            }
          }

          if ( xx > 15 ) {
            if ( picked_chunk_id == 0 ) {
              xx              = 0;
              picked_chunk_id = 1;
            } else if ( picked_chunk_id == 2 ) {
              xx              = 0;
              picked_chunk_id = 3;
            }
          }
          if ( zz > 15 ) {
            if ( picked_chunk_id == 0 ) {
              zz              = 0;
              picked_chunk_id = 2;
            } else if ( picked_chunk_id == 1 ) {
              zz              = 0;
              picked_chunk_id = 3;
            }
          }

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
            vertex_data.n_vpalidx_comps, vertex_data.picking_ptr, vertex_data.n_vpicking_comps, vertex_data.normals_ptr, vertex_data.n_vn_comps, vertex_data.n_vertices );
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
    for ( int i = 0; i < 4; i++ ) { draw_mesh( voxel_shader, cam.P, cam.V, chunks_M[i], chunk_meshes[i].vao, chunk_meshes[i].n_vertices, &palette_tex, 1 ); }
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
        "FPS %.2f\nwin dims (%i,%i). fb dims (%i,%i)\nmouse xy (%.2f,%.2f)\nhovered voxel: %s\n\nLMB,RMB   edit voxels\n1,2,3...     block type\nF2        "
        "export `out.ply`\nF9        reset chunk",
        fps, win_width, win_height, fb_width, fb_height, mouse_x, mouse_y, hovered_voxel_str );

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

      for ( uint32_t i = 0; i < 4; i++ ) {
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
  delete_texture( &palette_tex );
  stop_gl();
  printf( "HALT\n" );
  return 0;
}
