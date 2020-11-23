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

#include "gfx.h"
#include "apg_maths.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

gfx_texture_t cubemap_set_up() {
  gfx_texture_t tex = ( gfx_texture_t ){ .handle_gl = 0 };
  // to match OpenGL constants order is: posx,negx,posy,negy,posz,negz
  const char* img_paths[6] = { "textures/kegs_right.png", "textures/kegs_left.png", "textures/kegs_up.png", "textures/kegs_down.png",
    "textures/kegs_forward.png", "textures/kegs_back.png" };
  uint8_t* img_ptr[6]      = { NULL };
  int x = 0, y = 0, comp = 0;
  for ( int i = 0; i < 6; i++ ) {
    img_ptr[i] = stbi_load( img_paths[i], &x, &y, &comp, 0 );
    if ( !img_paths ) { fprintf( stderr, "ERROR loading `%s`\n", img_paths[i] ); }
  }
  tex = gfx_create_cube_texture_from_mem( img_ptr, x, y, comp, ( gfx_texture_properties_t ){ .bilinear = true, .has_mips = true, .is_cube = true, .is_srgb = true } );
  for ( int i = 0; i < 6; i++ ) { free( img_ptr[i] ); }
  return tex;
}

int main() {
  // load a sphere mesh
  vec3 verts[4096];
  uint32_t indices[4096];
  int n_verts = 0, n_indices = 0;
  _gen_uv_sphere( 16, 16, 1.0f, verts, &n_verts, indices, &n_indices );
  printf( "UV sphere generated: %u verts, %u indices\n", n_verts, n_indices );
  _dump_obj( verts, indices, n_verts, n_indices );

  // load a shader with a UBO for PBR stuff

  // loop over nxm spheres with varying inputs

  gfx_start( "PBR spheres direct lighting demo", 1024, 1024, false );

  gfx_mesh_t sphere_mesh =
    gfx_create_mesh_from_mem( &verts[0].x, 3, NULL, 0, NULL, 0, NULL, 0, indices, sizeof( uint32_t ) * n_indices, GFX_INDICES_TYPE_UINT32, n_verts, false );
  gfx_shader_t sphere_shader = gfx_create_shader_program_from_files( "sphere.vert", "sphere_gltf.frag" );
  gfx_shader_t cube_shader   = gfx_create_shader_program_from_files( "cube.vert", "cube.frag" );
  gfx_texture_t cube_texture = cubemap_set_up();

  vec3 light_pos_wor_initial = ( vec3 ){ 0, 5, 10 };

  // gfx_wireframe_mode();
  // gfx_backface_culling( false );

  while ( !gfx_should_window_close() ) {
    int fb_w = 0, fb_h = 0;
    gfx_framebuffer_dims( &fb_w, &fb_h );
    float aspect     = (float)fb_w / (float)fb_h;
    vec3 cam_pos_wor = ( vec3 ){ 0, 0, 15.0f };
    mat4 V           = look_at( cam_pos_wor, ( vec3 ){ 0, 0, 0 }, ( vec3 ){ 0, 1, 0 } );
    mat4 P           = perspective( 67, aspect, 0.1, 1000.0 );

    gfx_viewport( 0, 0, fb_w, fb_h );
    gfx_clear_colour_and_depth_buffers( 0.2, 0.2, 0.2, 1.0 );

    {                                  // skybox
      mat4 V_envmap = identity_mat4(); // rot_y_deg_mat4( -cam_heading );
      mat4 M_envmap = scale_mat4( ( vec3 ){ 10, 10, 10 } );
      gfx_depth_mask( false );
      gfx_draw_mesh( gfx_cube_mesh, GFX_PT_TRIANGLES, cube_shader, P.m, V_envmap.m, M_envmap.m, &cube_texture, 1 );
      gfx_depth_mask( true );
    }

    // rotate light around z axis
    mat4 R_light = rot_z_deg_mat4( gfx_get_time_s() * 100.0 );

    vec4 light_pos_curr_wor_xyzw = mult_mat4_vec4( R_light, v4_v3f( light_pos_wor_initial, 1.0 ) );

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

        gfx_draw_mesh( sphere_mesh, GFX_PT_TRIANGLES, sphere_shader, P.m, V.m, M.m, NULL, 0 );
      }
    }

    gfx_swap_buffer();
    gfx_poll_events();
  }

  gfx_stop();

  //

  return 0;
}
