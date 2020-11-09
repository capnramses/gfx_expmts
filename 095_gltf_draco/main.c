/*
PLAN:
1. load and display a gltf file
   - using cgltf.h by Johannes Kuhlmann https://github.com/jkuhlmann/cgltf (MIT)
   - using OpenGL
2. load and display a Google Draco-compressed glTF file
   - need to use the Draco lib https://github.com/google/draco
   - cgltf can support this
3. add PBR lighting shaders and use the materials from the gltf file


TODO

* respect indexed rendering
 */

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "gfx.h"
#include "apg_maths.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main( int argc, const char** argv ) {
  printf( "gltf + draco in opengl\n" );
  if ( argc < 2 ) {
    printf( "usage: ./a.out MYFILE.gltf\n" );
    return 0;
  }

  char title[1024];
  snprintf( title, 1023, "anton's gltf demo: %s", argv[1] );
  if ( !gfx_start( title, 1920, 1080, false ) ) {
    fprintf( stderr, "ERROR - starting window\n" );
    return 1;
  }

  gfx_mesh_t mesh = ( gfx_mesh_t ){ .n_vertices = 0 };
  {
    printf( "loading `%s`...\n", argv[1] );
    cgltf_data* data      = NULL; // generally matches format in spec https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
    cgltf_options options = { 0 };
    cgltf_result result   = cgltf_parse_file( &options, argv[1], &data );
    if ( result != cgltf_result_success ) {
      fprintf( stderr, "ERROR: loading GLTF file\n" );
      return 1;
    }

    // now read the data into the data pointers (or it will be NULL)
    result = cgltf_load_buffers( &options, data, argv[1] );
    if ( result != cgltf_result_success ) {
      fprintf( stderr, "Error loading buffers" );
      return 1;
    }

    printf( "loaded %i meshes from GLTF model\n", (int)data->meshes_count );
    for ( int i = 0; i < (int)data->meshes_count; i++ ) {
      if ( data->meshes[i].name ) { printf( "mesh %i) name: `%s`\n", i, data->meshes[i].name ); }
      for ( int j = 0; j < (int)data->meshes[i].primitives_count; j++ ) {
        for ( int k = 0; k < (int)data->meshes[i].primitives[j].attributes_count; k++ ) {
          printf( "mesh %i) primitive %i) `%s`::%i\n", i, j, data->meshes[i].primitives[j].attributes[k].name, data->meshes[i].primitives[j].attributes[k].index );
          if ( data->meshes[i].primitives[j].attributes[k].type == cgltf_attribute_type_position ) {
            //
            cgltf_accessor* acc = data->meshes[i].primitives[j].attributes[k].data;
            int n_vertices      = (int)acc->count; // note gltf position is a vec3 so this is number of vertices, not number of floats

            printf( "n verts = %i\n", n_vertices );

            uint8_t* bytes_ptr = (uint8_t*)acc->buffer_view->buffer->data;
            float* points_ptr  = (float*)&bytes_ptr[acc->buffer_view->offset];

            for ( int v = 0; v < n_vertices; v++ ) {
              printf( " point %i ) { %f,%f,%f }\n", v, points_ptr[3 * v], points_ptr[3 * v + 1], points_ptr[3 * v + 2] );
            }

            // TODO(Anton) add indices
            mesh = gfx_create_mesh_from_mem( points_ptr, 3, NULL, 0, NULL, 0, points_ptr, 3, NULL, 0, 0, n_vertices, false );
          }
        }
      }
    }

    cgltf_free( data );
  }

  while ( !gfx_should_window_close() ) {
    int fb_w = 0, fb_h = 0;
    gfx_framebuffer_dims( &fb_w, &fb_h );
    gfx_viewport( 0, 0, fb_w, fb_h );
    gfx_clear_colour_and_depth_buffers( 0.2, 0.2, 0.2, 1.0 );

    mat4 P = perspective( 67, (float)fb_w / (float)fb_h, 0.1, 1000 );
    mat4 V = look_at( ( vec3 ){ 0, 0, 1 }, ( vec3 ){ 0, 0, 0 }, ( vec3 ){ 0, 1, 0 } );
    mat4 M = identity_mat4();

    gfx_backface_culling( false );

    // gfx_mesh_t mesh, gfx_primitive_type_t pt, gfx_shader_t shader, float* P, float* V, float* M, gfx_texture_t* textures, int n_textures
    gfx_uniform1f( gfx_default_shader, gfx_default_shader.u_alpha, 1.0 );
    gfx_draw_mesh( mesh, GFX_PT_TRIANGLES, gfx_default_shader, P.m, V.m, M.m, NULL, 0 );

    gfx_swap_buffer();
    gfx_poll_events();
  }

  gfx_stop();

  return 0;
}
