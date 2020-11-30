#include "gltf.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

bool gltf_load( const char* filename, gltf_scene_t* gltf_scene_ptr ) {
  if ( !filename || !gltf_scene_ptr ) { return false; }

  // format mirrors glTF spec https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
  cgltf_data* data_ptr  = NULL;
  cgltf_options options = { 0 };

  // parses both gltf and glb according to docs in header
  // must free data pointer if succeeds
  // contents of external files for buffers and images are not loaded. URIs are in `cgltf_data`.
  // cgltf_options - zero initialised structure to trigger default behaviour
  cgltf_result res = cgltf_parse_file( &options, filename, &data_ptr );
  if ( cgltf_result_success != res ) {
    fprintf( stderr, "ERROR: loading GLTF file `%s`\n", filename );
    return false;
  }

  // populate data buffers with external files using FILE* API
  res = cgltf_load_buffers( &options, data_ptr, filename );
  if ( cgltf_result_success != res ) {
    fprintf( stderr, "ERROR: loading external resources for GLTF file `%s`\n", filename );
    cgltf_free( data_ptr );
    return false;
  }

  { // create gfx resources from gltf data
    gltf_scene_ptr->n_meshes   = (uint32_t)data_ptr->meshes_count;
    gltf_scene_ptr->meshes_ptr = calloc( gltf_scene_ptr->n_meshes, sizeof( gfx_mesh_t ) );
    for ( uint32_t i = 0; i < gltf_scene_ptr->n_meshes; i++ ) {}
  }

  cgltf_free( data_ptr );
  return true;
}

bool gltf_free( gltf_scene_t* gltf_scene_ptr ) {
  if ( !gltf_scene_ptr || !gltf_scene_ptr->meshes_ptr ) { return false; }
  free( gltf_scene_ptr->meshes_ptr );
  *gltf_scene_ptr = ( gltf_scene_t ){ .n_meshes = 0 };
  return true;
}
