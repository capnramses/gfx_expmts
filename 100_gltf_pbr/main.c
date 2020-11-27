/*
Diffuse Irradiance
- cubemaps used for lighting in PBR are pre-computed because an integral needs to be solved by convolution
- this lets us represent the full hemisphere of angles of radiance
- they assume the centre of the cube is the surface being lit, which doesn't work well for things spread throughout an interior environment
- to improve this several cubes can be generated - i.e. 'reflection probes' are placed about the room
- i believe engines must compile these as part of a scene build process, rather than have them re-evaluate at run-time
- to represent a range light energy properly we need values outside 0-1 and .: an HDR encoding of the image.

HDR map and equirectangular faffery
- .hdr radiance file formats store a full cubemap in one "equirectangular" image of floats (its a sphere projected onto a rectangle).
  (8 bits per channel + exponent in alpha). requires parsing into floats on read.
  examples here http://www.hdrlabs.com/sibl/archive.html
  stb_image.h can load HDR via stbi_loadf() in 32-bit float 3 channel.
  load as a GL_TEXTURE_2D but format GL_RGB16F, GL_RGB, FL_FLOAT --> why not 32F ?
- it's still a rectangle after loading
- need a pre-pass to create a cube by projecting it
THIS SEEMS LIKE A VERY UNNECESSARY EXTRA STEP -> make a program to do this separately or don't use these equirectangular images

Note to self
-> i think an example of building our own reflection probe cube would be more interesting that using an external hdr thing
-> could actually write equirectangular maps if we wanted to using the same formula as smapling them:

  inv_atan = vec2( 0.1591, 0.3183 );
  uv = vec2( atan( v.z, v.x ), asin( v.y ) ) * inv_atan + 0.5;

Convolution around a hemisphere ( full y-rot of 0-360 in little dϕ stepeens with a up-down rot of 0-90 degrees in dθ stepeens /at each y-rot stepeen/ )
Lo(p,ϕo,θo) = kdc/π ∫2π,ϕ=0 ∫1/2π,θ=0 Li(p,ϕi,θi)cos(θ)sin(θ)dϕdθ
Riemann sum translates to discrete integral:
Lo(p,ϕo,θo)=k_d * (cπ)/(n1*n2) ∑ϕ=0n1 ∑θ=0n2 Li(p,ϕi,θi)cos(θ)sin(θ)dϕdθ
* because sample areas get smaller near the top of a sphere we scale the equation by sin( theta )

GLSL from https://learnopengl.com/PBR/IBL/Diffuse-irradiance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    vec3 irradiance   = vec3( 0.0 ); // L_o
    vec3 up           = vec3( 0.0, 1.0, 0.0 );
    vec3 right        = cross( up, normal );
    up                = cross( normal, right );
    float sampleDelta = 0.025;                                                                           // decrease delta to increase accuracy
    float nrSamples   = 0.0;
    for( float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta ) {                                         // y-rot
      for( float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta ) {                                 // up-and-down quarter rot
        vec3 tangentSample = vec3( sin( theta ) * cos( phi ), sin( theta ) * sin( phi ), cos( theta ) ); // spherical to cartesian (in tangent space)
        vec3 sampleVec     = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;       // tangent space to world, oriented around normal
        // Scaled by cos( thea ) to compensate for weaker light at larger angles, and by sin( theta ) for smaller areas in higher hemisphere.
        irradiance        += texture( environmentMap, sampleVec ).rgb * cos( theta ) * sin( theta );     // L_i.
        nrSamples++;
      }
    }
    irradiance = PI * irradiance * ( 1.0 / float( nrSamples ) );                                         // average result by number of samples
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
NOTE - can store the result in 32x32 images and use linear filtering to help balance

-> replace ambient lighting constant with this IBL

Plan
- just load my regular cubemap as a background and not worry about HDR scales for now
- get a camera working
-> convolve the environment map to produce a blurry boi

-> if we generate these for the building with a white background behind windows/skylights we should get some neat lighting
*/

// TODO
// - put a light-coloured sphere on the light position

#include "gfx.h"
#include "apg_maths.h"
#include "input.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// uncomment if source cubemap is hdr
#define ENVMAP_HDR
// resolution of IBL map -- make it higher to test.
#define IBL_DIMS 32 // 1024 // 32

// see http://www.songho.ca/opengl/gl_sphere.html -- also has code for verts, normals
static void _gen_uv_sphere( int stackCount, int sectorCount, float radius, vec3* vertices_ptr, uint32_t* n_verts_ptr, uint32_t* indices_ptr, uint32_t* n_indices_ptr ) {
  uint32_t n_verts = 0;
  for ( int i = 0; i <= stackCount; ++i ) {
    float stackAngle = APG_M_PI / 2.0f - (float)( i * APG_M_PI / (float)stackCount ); // starting from pi/2 to -pi/2
    float xy         = radius * cosf( stackAngle );                                   // r * cos(u)
    float z          = radius * sinf( stackAngle );                                   // r * sin(u)
    // add (sectorCount+1) vertices per stack
    // the first and last vertices have same position and normal, but different tex coords
    for ( int j = 0; j <= sectorCount; ++j ) {
      float sectorStep        = 2.0f * APG_M_PI / (float)sectorCount;
      vertices_ptr[n_verts++] = ( vec3 ){ .x = xy * cosf( j * sectorStep ), .y = xy * sinf( j * sectorStep ), .z = z };
    }
  }
  *n_verts_ptr = n_verts;

  // generate CCW index list of sphere triangles
  uint32_t n_indices = 0;
  for ( int i = 0; i < stackCount; ++i ) {
    int k1 = i * ( sectorCount + 1 ); // beginning of current stack
    int k2 = k1 + sectorCount + 1;    // beginning of next stack

    for ( int j = 0; j < sectorCount; ++j, ++k1, ++k2 ) {
      // 2 triangles per sector excluding first and last stacks
      // k1 => k2 => k1+1
      if ( i != 0 ) {
        indices_ptr[n_indices++] = k1;
        indices_ptr[n_indices++] = k2;
        indices_ptr[n_indices++] = k1 + 1;
      }

      // k1+1 => k2 => k2+1
      if ( i != ( stackCount - 1 ) ) {
        indices_ptr[n_indices++] = k1 + 1;
        indices_ptr[n_indices++] = k2;
        indices_ptr[n_indices++] = k2 + 1;
      }
    }
  }
  *n_indices_ptr = n_indices;
}

static void _dump_obj( const vec3* verts_ptr, const uint32_t* indices_ptr, uint32_t n_verts, uint32_t n_indices ) {
  FILE* f_ptr = fopen( "out.obj", "w" );
  assert( f_ptr );

  fprintf( f_ptr, "#Wavefront .obj generated by _dump_obj() util by Anton Gerdelan\n" );
  for ( uint32_t v = 0; v < n_verts; v++ ) { fprintf( f_ptr, "v %f %f %f\n", verts_ptr[v].x, verts_ptr[v].y, verts_ptr[v].z ); }
  for ( uint32_t i = 0; i < n_indices; i += 3 ) { fprintf( f_ptr, "f %u %u %u\n", indices_ptr[i + 0] + 1, indices_ptr[i + 1] + 1, indices_ptr[i + 2] + 1 ); }
  fclose( f_ptr );
}

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

int main() {
  // load a sphere mesh
  vec3 verts[4096];
  uint32_t indices[4096];
  uint32_t n_verts = 0, n_indices = 0;
  _gen_uv_sphere( 16, 16, 1.0f, verts, &n_verts, indices, &n_indices );
  printf( "UV sphere generated: %u verts, %u indices\n", n_verts, n_indices );
  _dump_obj( verts, indices, n_verts, n_indices );

  // load a shader with a UBO for PBR stuff

  // loop over nxm spheres with varying inputs

  gfx_start( "PBR - Image-Based-Lighting demo", 1024, 1024, false );
  input_init();
  gfx_cubemap_seamless( true );

  gfx_mesh_t sphere_mesh =
    gfx_create_mesh_from_mem( &verts[0].x, 3, NULL, 0, NULL, 0, NULL, 0, indices, sizeof( uint32_t ) * n_indices, GFX_INDICES_TYPE_UINT32, n_verts, false );
  gfx_shader_t sphere_shader = gfx_create_shader_program_from_files( "sphere.vert", "sphere_gltf.frag" );
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
#ifdef ENVMAP_HDR
  gfx_shader_t prefilter_shader = gfx_create_shader_program_from_files( "cube.vert", "cube_prefilter_hdr.frag" );
#else
  gfx_shader_t prefilter_shader = gfx_create_shader_program_from_files( "cube.vert", "cube_prefilter_ldr.frag" );
#endif
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

    //	gfx_draw_mesh( quad

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

  // gfx_wireframe_mode();
  // gfx_backface_culling( false );

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

    int n_across = 5;
    int n_down   = 5;
    for ( int yi = 0; yi < n_down; yi++ ) {
      float y = (float)yi * 3.0f - ( ( n_down - 1 ) * 3 * 0.5f );
      for ( int xi = 0; xi < n_across; xi++ ) {
        // u_roughness_factor
        float x = (float)xi * 3.0f - ( ( n_across - 1 ) * 3 * 0.5f );
        mat4 T  = translate_mat4( ( vec3 ){ x, y, 0 } );
        mat4 R  = rot_y_deg_mat4( gfx_get_time_s() * 10.0 );
        mat4 M  = mult_mat4_mat4( T, R );

        float roughness = (float)xi / (float)( n_across - 1 );
        float metallic  = (float)yi / (float)( n_down - 1 );
        // printf( "roughness = %f, metallic = %f\n", roughness, metallic );
        gfx_uniform1f( sphere_shader, sphere_shader.u_roughness_factor, roughness );
        gfx_uniform1f( sphere_shader, sphere_shader.u_metallic_factor, metallic );
        gfx_uniform3f( sphere_shader, sphere_shader.u_light_pos_wor, light_pos_curr_wor_xyzw.x, light_pos_curr_wor_xyzw.y, light_pos_curr_wor_xyzw.z );
        gfx_uniform3f( sphere_shader, sphere_shader.u_cam_pos_wor, cam_pos_wor.x, cam_pos_wor.y, cam_pos_wor.z );

        gfx_draw_mesh( sphere_mesh, GFX_PT_TRIANGLES, sphere_shader, P.m, V.m, M.m, pbr_textures, GFX_TEXTURE_UNIT_MAX );
      }
    }

    if ( debug_lut ) {
      mat4 I = identity_mat4();
      gfx_draw_mesh( gfx_ss_quad_mesh, GFX_PT_TRIANGLE_STRIP, quad_shader, I.m, I.m, I.m, &brdf_lut_texture, 1 );
    }

    gfx_swap_buffer();
    input_reset_last_polled_input_states();
    gfx_poll_events();
  }

  gfx_stop();

  //

  return 0;
}
