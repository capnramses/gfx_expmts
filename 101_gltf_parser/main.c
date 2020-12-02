#include "gfx_gltf.h"
#include "input.h"
#include "apg_maths.h"
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

// resolution of IBL map -- make it higher to test.
#define IBL_DIMS 32 // 1024 // 32

gfx_texture_t cubemap_from_images() {
  gfx_texture_t tex = ( gfx_texture_t ){ .handle_gl = 0 };
  // to match OpenGL constants order is: posx,negx,posy,negy,posz,negz
  const char* img_paths[6] = { "textures/kegs_right.png", "textures/kegs_left.png", "textures/kegs_up.png", "textures/kegs_down.png",
    "textures/kegs_forward.png", "textures/kegs_back.png" };
  uint8_t* img_ptr[6]      = { NULL };
  int x = 0, y = 0, comp = 0;
  for ( int i = 0; i < 6; i++ ) {
    img_ptr[i] = stbi_load( img_paths[i], &x, &y, &comp, 0 );
    if ( !img_ptr[i] ) { fprintf( stderr, "ERROR loading `%s`\n", img_paths[i] ); }
  }
  tex = gfx_create_cube_texture_from_mem(
    img_ptr, x, y, comp, ( gfx_texture_properties_t ){ .bilinear = true, .has_mips = false, .is_cube = true, .is_srgb = true, .is_hdr = false } );
  for ( int i = 0; i < 6; i++ ) { free( img_ptr[i] ); }
  return tex;
}

int main( int argc, char** argv ) {
  if ( argc < 2 ) {
    printf( "Usage ./a.out MYFILE.gltf\n" );
    return 0;
  }

  if ( !gfx_start( "glTF loader test", 1024, 1024, false ) ) { return 1; }
  input_init();
  gfx_cubemap_seamless( true );
  gfx_shader_t pbr_shader    = gfx_create_shader_program_from_files( "pbr.vert", "pbr.frag" );
  gfx_shader_t cube_shader   = gfx_create_shader_program_from_files( "cube.vert", "cube.frag" );
  gfx_texture_t cube_texture = cubemap_from_images();
  // TODO(Anton) does not need mips.
  gfx_texture_t irradiance_texture =
    gfx_create_cube_texture_from_mem( NULL, IBL_DIMS, IBL_DIMS, 3, ( gfx_texture_properties_t ){ .bilinear = true, .has_mips = false, .is_cube = true } );
  gfx_framebuffer_t convoluting_framebuffer = gfx_create_framebuffer( IBL_DIMS, IBL_DIMS, true );
  gfx_shader_t convoluting_shader           = gfx_create_shader_program_from_files( "cube.vert", "cube_convolution.frag" );

  { // pass to create irradiance map from cube map

    gfx_viewport( 0, 0, IBL_DIMS, IBL_DIMS );
    mat4 I     = identity_mat4();
    mat4 P     = perspective( 90.0f, 1.0f, 0.1f, 100.0f );
    mat4 Vs[6] = {
      look_at( ( vec3 ){ 0 }, ( vec3 ){ 1, 0, 0 }, ( vec3 ){ 0, -1, 0 } ),  //
      look_at( ( vec3 ){ 0 }, ( vec3 ){ -1, 0, 0 }, ( vec3 ){ 0, -1, 0 } ), //
      look_at( ( vec3 ){ 0 }, ( vec3 ){ 0, 1, 0 }, ( vec3 ){ 0, 0, 1 } ),   //
      look_at( ( vec3 ){ 0 }, ( vec3 ){ 0, -1, 0 }, ( vec3 ){ 0, 0, -1 } ), //
      look_at( ( vec3 ){ 0 }, ( vec3 ){ 0, 0, 1 }, ( vec3 ){ 0, -1, 0 } ),  //
      look_at( ( vec3 ){ 0 }, ( vec3 ){ 0, 0, -1 }, ( vec3 ){ 0, -1, 0 } )  //
    };
    for ( int i = 0; i < 6; i++ ) {
      gfx_framebuffer_bind_cube_face( convoluting_framebuffer, irradiance_texture, i, 0 );
      bool ret = gfx_framebuffer_status( convoluting_framebuffer );
      if ( !ret ) { fprintf( stderr, "ERROR: convoluting_framebuffer\n" ); }
      gfx_bind_framebuffer( &convoluting_framebuffer );
      gfx_clear_colour_and_depth_buffers( 0.0f, 0.0f, 0.0f, 0.0f );
      gfx_draw_mesh( gfx_cube_mesh, GFX_PT_TRIANGLES, convoluting_shader, P.m, Vs[i].m, I.m, &cube_texture, 1 );
    }
    gfx_bind_framebuffer( NULL );
  }
  // new stuff
  // NOTE: viewing the prefilter map at LODs shows distinct colour differences between cube sides but its otherwise okay. limitation of source LDR image?
  int prefilter_map_dims              = 128; // can be higher for more detailed surfaces
  int prefilter_max_mip_levels        = 5;
  gfx_texture_t prefilter_map_texture = gfx_create_cube_texture_from_mem( NULL, prefilter_map_dims, prefilter_map_dims, 3,
    ( gfx_texture_properties_t ){ .bilinear = true, .has_mips = true, .is_cube = true, .is_hdr = true, .is_srgb = true, .cube_max_mip_level = prefilter_max_mip_levels } );

  gfx_shader_t prefilter_shader           = gfx_create_shader_program_from_files( "cube.vert", "cube_prefilter_hdr.frag" );
  gfx_framebuffer_t prefilter_framebuffer = gfx_create_framebuffer( prefilter_map_dims, prefilter_map_dims, true );

  { // create prefilter map
    mat4 I     = identity_mat4();
    mat4 P     = perspective( 90.0f, 1.0f, 0.1f, 100.0f );
    mat4 Vs[6] = {
      look_at( ( vec3 ){ 0 }, ( vec3 ){ 1, 0, 0 }, ( vec3 ){ 0, -1, 0 } ),  //
      look_at( ( vec3 ){ 0 }, ( vec3 ){ -1, 0, 0 }, ( vec3 ){ 0, -1, 0 } ), //
      look_at( ( vec3 ){ 0 }, ( vec3 ){ 0, 1, 0 }, ( vec3 ){ 0, 0, 1 } ),   //
      look_at( ( vec3 ){ 0 }, ( vec3 ){ 0, -1, 0 }, ( vec3 ){ 0, 0, -1 } ), //
      look_at( ( vec3 ){ 0 }, ( vec3 ){ 0, 0, 1 }, ( vec3 ){ 0, -1, 0 } ),  //
      look_at( ( vec3 ){ 0 }, ( vec3 ){ 0, 0, -1 }, ( vec3 ){ 0, -1, 0 } )  //
    };
    gfx_bind_framebuffer( &prefilter_framebuffer );
    for ( int mip_level = 0; mip_level < prefilter_max_mip_levels; mip_level++ ) {
      float roughness = (float)mip_level / (float)( prefilter_max_mip_levels - 1.0f );
      gfx_uniform1f( prefilter_shader, prefilter_shader.u_roughness_factor, roughness );

      int mip_w = prefilter_map_dims * pow( 0.5, (double)mip_level );
      int mip_h = prefilter_map_dims * pow( 0.5, (double)mip_level );
      gfx_framebuffer_update_depth_texture_dims( prefilter_framebuffer, mip_w, mip_h );
      gfx_viewport( 0, 0, mip_w, mip_h );

      for ( int i = 0; i < 6; i++ ) {
        gfx_framebuffer_bind_cube_face( prefilter_framebuffer, prefilter_map_texture, i, mip_level );

        bool ret = gfx_framebuffer_status( prefilter_framebuffer );
        if ( !ret ) { fprintf( stderr, "ERROR: prefilter_framebuffer. mip level %i, face %i, w %i h %i\n", mip_level, i, mip_w, mip_h ); }

        gfx_bind_framebuffer( &prefilter_framebuffer );
        gfx_clear_colour_and_depth_buffers( 0.0f, 0.0f, 0.0f, 0.0f );
        gfx_draw_mesh( gfx_cube_mesh, GFX_PT_TRIANGLES, prefilter_shader, P.m, Vs[i].m, I.m, &cube_texture, 1 );
      }
    }
    gfx_bind_framebuffer( NULL );
  }

  // newer stuff
  // TODO(Anton) add a way to set the framebuffer texture to HDR RG16F
  gfx_texture_t brdf_lut_texture = gfx_create_texture_from_mem( NULL, 512, 512, 2, ( gfx_texture_properties_t ){ .bilinear = true, .is_hdr = true } );
  {
    gfx_shader_t gfx_brdf_lut_shader = gfx_create_shader_program_from_files( "quad.vert", "brdf_lut.frag" );
    gfx_framebuffer_t brdf_lut_fb    = gfx_create_framebuffer( 512, 512, false );
    gfx_framebuffer_bind_texture( brdf_lut_fb, brdf_lut_texture );
    gfx_bind_framebuffer( &brdf_lut_fb );
    gfx_viewport( 0, 0, 512, 512 );
    gfx_clear_colour_and_depth_buffers( 0.0f, 0.0f, 0.0f, 0.0f );
    mat4 I = identity_mat4();
    gfx_draw_mesh( gfx_ss_quad_mesh, GFX_PT_TRIANGLE_STRIP, gfx_brdf_lut_shader, I.m, I.m, I.m, NULL, 0 );
    gfx_bind_framebuffer( NULL );
  }
  gfx_shader_t quad_shader = gfx_create_shader_program_from_files( "quad.vert", "quad.frag" );
  {
    gfx_viewport( 0, 0, 512, 512 );
    gfx_clear_colour_and_depth_buffers( 0.0f, 0.0f, 0.0f, 0.0f );
    mat4 I = identity_mat4();
    gfx_draw_mesh( gfx_ss_quad_mesh, GFX_PT_TRIANGLE_STRIP, quad_shader, I.m, I.m, I.m, &brdf_lut_texture, 1 );
    gfx_swap_buffer();

    uint8_t* img_ptr = malloc( 512 * 512 * 2 );
    gfx_read_pixels( 0, 0, 512, 512, 2, img_ptr );
    stbi_write_png( "lut.png", 512, 512, 2, img_ptr, 512 * 2 );
    free( img_ptr );
  }
  vec3 light_pos_wor_initial = ( vec3 ){ 0, 5, 10 };

  printf( "Reading `%s`\n", argv[1] );
  gfx_gltf_t gltf = { 0 };
  if ( !gfx_gltf_load( argv[1], &gltf ) ) { return 1; }

  float cam_x_deg = 0.0f, cam_y_deg = 0.0f;
  double prev_s = gfx_get_time_s();

  int background_image_mode = 0;
  bool debug_lut            = false;

  while ( !gfx_should_window_close() ) {
    double curr_s    = gfx_get_time_s();
    double elapsed_s = curr_s - prev_s;
    prev_s           = curr_s;

    if ( input_is_key_held( input_turn_up_key ) ) { cam_x_deg += 90.0f * elapsed_s; }
    if ( input_is_key_held( input_turn_down_key ) ) { cam_x_deg -= 90.0f * elapsed_s; }
    if ( input_is_key_held( input_turn_left_key ) ) { cam_y_deg += 90.0f * elapsed_s; }
    if ( input_is_key_held( input_turn_right_key ) ) { cam_y_deg -= 90.0f * elapsed_s; }
    if ( input_was_key_pressed( 'I' ) ) { background_image_mode = ( background_image_mode + 1 ) % 3; }
    if ( input_was_key_pressed( 'L' ) ) { debug_lut = !debug_lut; }

    mat4 cam_R_x     = rot_x_deg_mat4( -cam_x_deg );
    mat4 cam_R_y     = rot_y_deg_mat4( -cam_y_deg );
    mat4 cam_R       = mult_mat4_mat4( cam_R_x, cam_R_y );
    vec3 cam_pos_wor = ( vec3 ){ 0, 0, 15.0f };
    mat4 cam_T       = translate_mat4( ( vec3 ){ .x = 0, .y = 0, .z = -cam_pos_wor.z } );
    mat4 V           = mult_mat4_mat4( cam_R, cam_T );

    int fb_w = 0, fb_h = 0;
    gfx_framebuffer_dims( &fb_w, &fb_h );
    float aspect = (float)fb_w / (float)fb_h;
    mat4 P       = perspective( 67, aspect, 0.1, 1000.0 );

    gfx_viewport( 0, 0, fb_w, fb_h );
    gfx_clear_colour_and_depth_buffers( 0.2, 0.2, 0.2, 1.0 );

    { // skybox
      mat4 V_envmap = cam_R;
      mat4 M_envmap = scale_mat4( ( vec3 ){ 10, 10, 10 } );
      gfx_depth_mask( false );
      // SWITCH TO irradiance_texture to test
      switch ( background_image_mode ) {
      case 0: {
        gfx_draw_mesh( gfx_cube_mesh, GFX_PT_TRIANGLES, cube_shader, P.m, V_envmap.m, M_envmap.m, &cube_texture, 1 );
      } break;
      case 1: {
        gfx_draw_mesh( gfx_cube_mesh, GFX_PT_TRIANGLES, cube_shader, P.m, V_envmap.m, M_envmap.m, &irradiance_texture, 1 );
      } break;
      case 2: {
        gfx_draw_mesh( gfx_cube_mesh, GFX_PT_TRIANGLES, cube_shader, P.m, V_envmap.m, M_envmap.m, &prefilter_map_texture, 1 );
      } break;
      }
      gfx_depth_mask( true );
    }

    // rotate light around z axis
    mat4 R_light = rot_z_deg_mat4( gfx_get_time_s() * 100.0 );

    vec4 light_pos_curr_wor_xyzw = mult_mat4_vec4( R_light, v4_v3f( light_pos_wor_initial, 1.0 ) );

    gfx_texture_t pbr_textures[GFX_TEXTURE_UNIT_MAX];
    pbr_textures[GFX_TEXTURE_UNIT_ENVIRONMENT] = cube_texture;
    pbr_textures[GFX_TEXTURE_UNIT_IRRADIANCE]  = irradiance_texture;
    pbr_textures[GFX_TEXTURE_UNIT_PREFILTER]   = prefilter_map_texture;
    pbr_textures[GFX_TEXTURE_UNIT_BRDF_LUT]    = brdf_lut_texture;

    {
      float scale_mod = 5.0f;
      mat4 S          = scale_mat4( ( vec3 ){ scale_mod, scale_mod, scale_mod } );
			mat4 R = rot_x_deg_mat4( 90.0f );
			mat4 M = mult_mat4_mat4( R, S );

      for ( int i = 0; i < gltf.n_meshes; i++ ) {
        gfx_uniform1f( pbr_shader, pbr_shader.u_roughness_factor, 1.0f );
        gfx_uniform1f( pbr_shader, pbr_shader.u_metallic_factor, 1.0f );
        gfx_uniform4f( pbr_shader, pbr_shader.u_base_colour_rgba, 1.0f, 1.0f, 1.0f, 1.0f );

        int mat_idx     = gltf.mat_idx_for_mesh_idx_ptr[i];
        int albedo_tidx = gltf.gltf.materials_ptr[mat_idx].pbr_metallic_roughness.base_colour_texture_idx;
        int mr_tidx     = gltf.gltf.materials_ptr[mat_idx].pbr_metallic_roughness.metallic_roughness_texture_idx;
        int ao_tidx     = gltf.gltf.materials_ptr[mat_idx].occlusion_texture_idx;
        int nm_tidx     = gltf.gltf.materials_ptr[mat_idx].normal_texture_idx;
        int em_tidx     = gltf.gltf.materials_ptr[mat_idx].emissive_texture_idx;
        if ( albedo_tidx > -1 ) {
          int albedo_iidx                       = gltf.gltf.textures_ptr[albedo_tidx].source_idx;
          pbr_textures[GFX_TEXTURE_UNIT_ALBEDO] = gltf.textures_ptr[albedo_iidx];
        }
        if ( mr_tidx > -1 ) {
          int mr_iidx                                    = gltf.gltf.textures_ptr[mr_tidx].source_idx;
          pbr_textures[GFX_TEXTURE_UNIT_METAL_ROUGHNESS] = gltf.textures_ptr[mr_iidx];
        }
        if ( ao_tidx > -1 ) {
          int ao_iidx                                      = gltf.gltf.textures_ptr[ao_tidx].source_idx;
          pbr_textures[GFX_TEXTURE_UNIT_AMBIENT_OCCLUSION] = gltf.textures_ptr[ao_iidx];
        }
        if ( nm_tidx > -1 ) {
          int nm_iidx                           = gltf.gltf.textures_ptr[nm_tidx].source_idx;
          pbr_textures[GFX_TEXTURE_UNIT_NORMAL] = gltf.textures_ptr[nm_iidx];
        }
        if ( em_tidx > -1 ) {
          int em_iidx                             = gltf.gltf.textures_ptr[em_tidx].source_idx;
          pbr_textures[GFX_TEXTURE_UNIT_EMISSIVE] = gltf.textures_ptr[em_iidx];
        }

        // TODO set up materials
        gfx_draw_mesh( gltf.meshes_ptr[i], GFX_PT_TRIANGLES, pbr_shader, P.m, V.m, M.m, pbr_textures, GFX_TEXTURE_UNIT_MAX );
      }
    }
#if 0
    { // render the glTF scene
      float curr_s = (float)gfx_get_time_s();
      mat4 S       = scale_mat4( ( vec3 ){ scale_mod, scale_mod, scale_mod } );
      mat4 T       = translate_mat4( ( vec3 ){ .x = sinf( curr_s ), .y = 0, .z = 0 } );
      mat4 Rx      = rot_x_deg_mat4( curr_s * 5.5f );
      mat4 Ry      = rot_y_deg_mat4( curr_s * 10.0f );
      mat4 R       = mult_mat4_mat4( Ry, Rx );
      mat4 TR      = mult_mat4_mat4( T, R );
      mat4 M       = mult_mat4_mat4( TR, S );
      gfx_uniform1f( pbr_shader, pbr_shader.u_roughness_factor, gltf_scene.material.roughness_f );
      gfx_uniform1f( pbr_shader, pbr_shader.u_metallic_factor, gltf_scene.material.metallic_f );
      gfx_uniform4f( pbr_shader, pbr_shader.u_base_colour_rgba, gltf_scene.material.base_colour_rgba[0], gltf_scene.material.base_colour_rgba[1],
        gltf_scene.material.base_colour_rgba[2], gltf_scene.material.base_colour_rgba[3] );
      // TODO(Anton) emissive RGB uniform
      gfx_uniform3f( pbr_shader, pbr_shader.u_light_pos_wor, light_pos_curr_wor_xyzw.x, light_pos_curr_wor_xyzw.y, light_pos_curr_wor_xyzw.z );
      gfx_uniform3f( pbr_shader, pbr_shader.u_cam_pos_wor, cam_pos_wor.x, cam_pos_wor.y, cam_pos_wor.z );

      if ( gltf_scene.material.base_colour_texture_idx > -1 ) {
        pbr_textures[GFX_TEXTURE_UNIT_ALBEDO] = gltf_scene.textures_ptr[gltf_scene.material.base_colour_texture_idx];
      }
      if ( gltf_scene.material.metal_roughness_texture_idx > -1 ) {
        pbr_textures[GFX_TEXTURE_UNIT_METAL_ROUGHNESS] = gltf_scene.textures_ptr[gltf_scene.material.metal_roughness_texture_idx];
      }
      if ( gltf_scene.material.emissive_texture_idx > -1 ) {
        pbr_textures[GFX_TEXTURE_UNIT_EMISSIVE] = gltf_scene.textures_ptr[gltf_scene.material.emissive_texture_idx];
      }
      if ( gltf_scene.material.ambient_occlusion_texture_idx > -1 ) {
        pbr_textures[GFX_TEXTURE_UNIT_AMBIENT_OCCLUSION] = gltf_scene.textures_ptr[gltf_scene.material.ambient_occlusion_texture_idx];
      }
      if ( gltf_scene.material.normal_texture_idx > -1 ) {
        pbr_textures[GFX_TEXTURE_UNIT_NORMAL] = gltf_scene.textures_ptr[gltf_scene.material.normal_texture_idx];
      }

      for ( uint32_t mi = 0; mi < gltf_scene.n_meshes; mi++ ) {
        gfx_draw_mesh( gltf_scene.meshes_ptr[mi], GFX_PT_TRIANGLES, pbr_shader, P.m, V.m, M.m, pbr_textures, GFX_TEXTURE_UNIT_MAX );
      }
    }
#endif
    if ( debug_lut ) {
      mat4 I = identity_mat4();
      gfx_draw_mesh( gfx_ss_quad_mesh, GFX_PT_TRIANGLE_STRIP, quad_shader, I.m, I.m, I.m, &brdf_lut_texture, 1 );
    }

    gfx_swap_buffer();
    input_reset_last_polled_input_states();
    gfx_poll_events();
  }

  gfx_gltf_free( &gltf );

  gfx_stop();
  printf( "done\n" );
  return 0;
}
