/*
PLAN:
1. load and display a gltf file
   - using cgltf.h by Johannes Kuhlmann https://github.com/jkuhlmann/cgltf (MIT)
   - using OpenGL
2. load and display a Google Draco-compressed glTF file
   - need to use the Draco lib https://github.com/google/draco
   - cgltf can support this
3. add PBR lighting shaders and use the materials from the gltf file


GLTF reference card here: https://www.slideshare.net/Khronos_Group/gltf-20-reference-guide


* NOTE: in glTF the default material model is Metallic-Roughness-Model of PBR. with values for rough/metallic either as constant values for whole model or from
textures.
  - materials->pbrMetallicRoughness->baseColorTexture->index    (eg 1 to use second texture)
                                                     ->texCoord (eg 0 to use first set of texture coordinates)
                                   ->baseColorFactor: [r,g,b,a]
                                   ->metallicRoughnessTexture....
                                   ->metallicFactor...
                                   ->roughnessFactor...
                                   ->normalTexture...
                                   ->occlusionTexture... (bits in the model that are internally shadowed and factored darker)
                                   ->emissiveTexture...
                                   ->emissiveFactor...
 - textures->....set of textures of type:
                ->source: #   - number of image asset
                ->sampler: #  - number of sampler
 - images->uri eg. filename or mime type
        OR
         ->bufferView: #
         ->mimeType: image/jpeg
 - samplers->magFilter    - set to OpenGL constant
           ->minFilter    - set to OpenGL constant
           ->wrapS        - set to OpenGL constant
           ->wrapT        - set to OpenGL constant



           this is a nice one:
            ..\..\glTF-Sample-Models\2.0\BarramundiFish\glTF\BarramundiFish.gltf





            TODO
            * blur reflection map and ~lower res
            *
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "gfx.h"
#include "apg_maths.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

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

int main( int argc, const char** argv ) {
  printf( "gltf + draco in opengl\n" );
  char gltf_path[1024];
  gltf_path[0] = '\0';
  if ( argc < 2 ) {
    printf( "usage: ./a.out MYFILE.gltf\n" );
    strcpy( gltf_path, "cube.gltf" );
  } else {
    strncat( gltf_path, argv[1], 1024 );
  }

  char asset_path[2048];
  asset_path[0]  = '\0';
  int last_slash = -1;
  int len        = strlen( gltf_path );
  for ( int i = 0; i < len; i++ ) {
    if ( gltf_path[i] == '\\' || gltf_path[i] == '/' ) { last_slash = i; }
  }
  if ( last_slash > -1 ) {
    strncpy( asset_path, gltf_path, last_slash + 1 );
    asset_path[last_slash + 1] = '\0';
    printf( "using asset path: `%s`\n", asset_path );
    for ( int i = 0; i < last_slash + 1; i++ ) {
      if ( asset_path[i] == '\\' ) { asset_path[i] = '/'; }
    }
  }

  char title[2048];
  snprintf( title, 2048, "anton's gltf demo: %s", gltf_path );
  if ( !gfx_start( title, 1920, 1080, false ) ) {
    fprintf( stderr, "ERROR - starting window\n" );
    return 1;
  }

  gfx_shader_t shader      = gfx_create_shader_program_from_files( "shader.vert", "shader.frag" );
  gfx_shader_t cube_shader = gfx_create_shader_program_from_files( "cube.vert", "cube.frag" );

  /*
  loaded image 0: `../../glTF-Sample-Models/2.0/DamagedHelmet/glTF/Default_albedo.jpg`
  loaded image 1: `../../glTF-Sample-Models/2.0/DamagedHelmet/glTF/Default_metalRoughness.jpg`
  loaded image 2: `../../glTF-Sample-Models/2.0/DamagedHelmet/glTF/Default_emissive.jpg`
  loaded image 3: `../../glTF-Sample-Models/2.0/DamagedHelmet/glTF/Default_AO.jpg`
  loaded image 4: `../../glTF-Sample-Models/2.0/DamagedHelmet/glTF/Default_normal.jpg`
  */
  gfx_texture_t textures[16];
  int n_textures = 0;

  gfx_texture_t cube_texture = cubemap_set_up();

  float scale_to_fit        = 1.0f;
  float base_colour_rgba[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
  gfx_mesh_t mesh           = ( gfx_mesh_t ){ .n_vertices = 0 };
  {
    printf( "loading `%s`...\n", gltf_path );
    cgltf_data* data      = NULL; // generally matches format in spec https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
    cgltf_options options = { 0 };
    cgltf_result result   = cgltf_parse_file( &options, gltf_path, &data );
    if ( result != cgltf_result_success ) {
      fprintf( stderr, "ERROR: loading GLTF file\n" );
      return 1;
    }

    // now read the data into the data pointers (or it will be NULL)
    result = cgltf_load_buffers( &options, data, gltf_path );
    if ( result != cgltf_result_success ) {
      fprintf( stderr, "Error loading buffers" );
      return 1;
    }

    printf( "loaded %i meshes from GLTF model\n", (int)data->meshes_count );
    for ( int i = 0; i < (int)data->meshes_count; i++ ) {
      if ( data->meshes[i].name ) { printf( "mesh %i) name: `%s`\n", i, data->meshes[i].name ); }
      for ( int j = 0; j < (int)data->meshes[i].primitives_count; j++ ) {
        // get indices here
        cgltf_accessor* indices_acc_ptr = data->meshes[i].primitives[j].indices;
        int n_indices                   = (int)indices_acc_ptr->count;
        size_t indices_sz               = n_indices * sizeof( uint16_t );
        int indices_type                = 1; // uint16_t

        /* https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_005_BuffersBufferViewsAccessors.md
        The first accessor refers to the bufferView with index 0, which defines the part of the buffer data that contains the indices.
        Its type is "SCALAR", and its componentType is 5123 (UNSIGNED_SHORT). This means that the indices are stored as scalar unsigned short values.
        */
        printf( "n_indices = %i\n", n_indices );
        uint8_t* indices_bytes_ptr = (uint8_t*)indices_acc_ptr->buffer_view->buffer->data;
        uint16_t* indices_ptr      = (uint16_t*)&indices_bytes_ptr[indices_acc_ptr->buffer_view->offset];
        float* points_ptr          = NULL;
        float* texcoords_ptr       = NULL;
        float* colours_ptr         = NULL;
        float* normals_ptr         = NULL;
        int n_colour_comps         = 3;
        int n_vertices             = 0;

        for ( int k = 0; k < (int)data->meshes[i].primitives[j].attributes_count; k++ ) {
          printf( "mesh %i) primitive %i) `%s`::%i\n", i, j, data->meshes[i].primitives[j].attributes[k].name, data->meshes[i].primitives[j].attributes[k].index );
          if ( data->meshes[i].primitives[j].attributes[k].type == cgltf_attribute_type_position ) {
            cgltf_accessor* acc = data->meshes[i].primitives[j].attributes[k].data;
            n_vertices          = (int)acc->count; // note gltf position is a vec3 so this is number of vertices, not number of floats
            printf( "n verts = %i\n", n_vertices );
            uint8_t* bytes_ptr = (uint8_t*)acc->buffer_view->buffer->data;
            points_ptr         = (float*)&bytes_ptr[acc->buffer_view->offset];

            // TODO first validate hasmin hasmax
            float min_x   = acc->min[0];
            float min_y   = acc->min[1];
            float min_z   = acc->min[2];
            float max_x   = acc->max[0];
            float max_y   = acc->max[1];
            float max_z   = acc->max[2];
            float biggest = fabs( min_x );
            if ( fabs( min_y ) > biggest ) { biggest = fabs( min_y ); }
            if ( fabs( min_z ) > biggest ) { biggest = fabs( min_z ); }
            if ( fabs( max_x ) > biggest ) { biggest = fabs( max_x ); }
            if ( fabs( max_y ) > biggest ) { biggest = fabs( max_y ); }
            if ( fabs( max_z ) > biggest ) { biggest = fabs( max_z ); }
            scale_to_fit = 1.0 / biggest;

          } else if ( data->meshes[i].primitives[j].attributes[k].type == cgltf_attribute_type_texcoord ) {
            cgltf_accessor* acc = data->meshes[i].primitives[j].attributes[k].data;
            uint8_t* bytes_ptr  = (uint8_t*)acc->buffer_view->buffer->data;
            texcoords_ptr       = (float*)&bytes_ptr[acc->buffer_view->offset];
          } else if ( data->meshes[i].primitives[j].attributes[k].type == cgltf_attribute_type_color ) {
            cgltf_accessor* acc = data->meshes[i].primitives[j].attributes[k].data;
            uint8_t* bytes_ptr  = (uint8_t*)acc->buffer_view->buffer->data;
            colours_ptr         = (float*)&bytes_ptr[acc->buffer_view->offset];
          } else if ( data->meshes[i].primitives[j].attributes[k].type == cgltf_attribute_type_normal ) {
            cgltf_accessor* acc = data->meshes[i].primitives[j].attributes[k].data;
            uint8_t* bytes_ptr  = (uint8_t*)acc->buffer_view->buffer->data;
            normals_ptr         = (float*)&bytes_ptr[acc->buffer_view->offset];
          } // endif

          mesh = gfx_create_mesh_from_mem( points_ptr, 3, texcoords_ptr, 2, normals_ptr, 3, colours_ptr, 3, indices_ptr, indices_sz, indices_type, n_vertices, false );
        }
      }
    }

    // load images
    n_textures = (int)data->images_count;
    printf( "%i textures\n", n_textures );
    for ( int i = 0; i < (int)data->images_count; i++ ) {
      char full_path[2048];
      strcpy( full_path, asset_path );
      strcat( full_path, data->images[i].uri );
      int x = 0, y = 0, comp = 0;
      unsigned char* img_ptr = stbi_load( full_path, &x, &y, &comp, 0 );
      if ( !img_ptr ) {
        fprintf( stderr, "ERROR loading image: `%s`\n", full_path );
        return 1;
      }
      textures[i] =
        gfx_create_texture_from_mem( img_ptr, x, y, comp, ( gfx_texture_properties_t ){ .bilinear = true, .has_mips = true, .is_srgb = false, .repeats = true } );
      free( img_ptr );

      printf( "loaded image %i: `%s`\n", i, full_path );
    }

    // load materials
    for ( int i = 0; i < (int)data->materials_count; i++ ) {
      printf( "material %i: `%s`. has_pbr_metallic_roughness: %i. has_pbr_specular_glossiness: %i\n", i, data->materials[i].name,
        data->materials[i].has_pbr_metallic_roughness, data->materials[i].has_pbr_specular_glossiness );
      if ( data->materials[i].has_pbr_metallic_roughness ) {
        memcpy( base_colour_rgba, data->materials[i].pbr_metallic_roughness.base_color_factor, sizeof( float ) * 4 );
      }
    }

    cgltf_free( data );
  }

  textures[GFX_TEXTURE_UNIT_ENVIRONMENT] = cube_texture; // #5

  float cam_heading = 0.0f; // y-rotation in degrees

  while ( !gfx_should_window_close() ) {
    int fb_w = 0, fb_h = 0;
    gfx_framebuffer_dims( &fb_w, &fb_h );
    gfx_viewport( 0, 0, fb_w, fb_h );
    gfx_clear_colour_and_depth_buffers( 0.2, 0.2, 0.2, 1.0 );

    cam_heading = gfx_get_time_s() * 5.0;

    mat4 P = perspective( 67, (float)fb_w / (float)fb_h, 0.1, 1000 );

    // mat4 V_t = translate_mat4( ( vec3 ){ 0, 0, -2 } );
    // mat4 V_r = rot_y_deg_mat4( -cam_heading );
    // mat4 V   = mult_mat4_mat4( V_t, V_r );
    vec3 cam_pos = ( vec3 ){ 0, 0, 2 };
    mat4 V       = look_at( cam_pos, ( vec3 ){ 0, 0, 0 }, ( vec3 ){ 0, 1, 0 } );

    // TODO scale mesh to fit viewport
    mat4 Rx = rot_x_deg_mat4( 90 + gfx_get_time_s() * 10.0 );
    mat4 Ry = rot_y_deg_mat4( gfx_get_time_s() * 10.0 );
    mat4 R  = mult_mat4_mat4( Ry, Rx );
    mat4 S  = scale_mat4( ( vec3 ){ scale_to_fit, scale_to_fit, scale_to_fit } );
    mat4 M  = mult_mat4_mat4( R, S );

    gfx_backface_culling( true );

    mat4 V_envmap = identity_mat4(); // rot_y_deg_mat4( -cam_heading );
    mat4 M_envmap = scale_mat4( ( vec3 ){ 10, 10, 10 } );
    gfx_depth_mask( false );
    gfx_draw_mesh( gfx_cube_mesh, GFX_PT_TRIANGLES, cube_shader, P.m, V_envmap.m, M_envmap.m, &cube_texture, 1 );
    gfx_depth_mask( true );

    // gfx_mesh_t mesh, gfx_primitive_type_t pt, gfx_shader_t shader, float* P, float* V, float* M, gfx_texture_t* textures, int n_textures
    gfx_uniform1f( shader, shader.u_alpha, 1.0 );
    gfx_uniform4f( shader, shader.u_base_colour_rgba, base_colour_rgba[0], base_colour_rgba[1], base_colour_rgba[2], base_colour_rgba[3] );
    gfx_uniform3f( shader, shader.u_cam_pos_wor, cam_pos.x, cam_pos.y, cam_pos.z );
    gfx_draw_mesh( mesh, GFX_PT_TRIANGLES, shader, P.m, V.m, M.m, textures, GFX_TEXTURE_UNIT_MAX );

    gfx_swap_buffer();
    gfx_poll_events();
  }

  gfx_stop();

  return 0;
}
