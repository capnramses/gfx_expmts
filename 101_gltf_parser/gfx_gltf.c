#include "gfx_gltf.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <stdint.h>
#include <stdio.h>

/* convenience struct and file->memory function */
typedef struct _entire_file_t {
  void* data;
  size_t sz;
} _entire_file_t;

/*
RETURNS
- true on success. record->data is allocated memory and must be freed by the caller.
- false on any error. Any allocated memory is freed if false is returned */
static bool gltf_read_entire_file( const char* filename, _entire_file_t* record ) {
  FILE* fp = fopen( filename, "rb" );
  if ( !fp ) { return false; }
  fseek( fp, 0L, SEEK_END );
  record->sz   = (size_t)ftell( fp );
  record->data = malloc( record->sz );
  if ( !record->data ) {
    fclose( fp );
    return false;
  }
  rewind( fp );
  size_t nr = fread( record->data, record->sz, 1, fp );
  fclose( fp );
  if ( 1 != nr ) { return false; }
  return true;
}

// TODO(Anton) validate array acceses
static uint8_t* _byte_pointer_for_attrib( const gltf_t* gltf_ptr, const _entire_file_t* buffers_ptr, int attrib_accessor_idx, int* count ) {
  if ( !gltf_ptr || attrib_accessor_idx < 0 || !buffers_ptr || !count ) { return NULL; }

  // accessor
  int buffer_view_idx             = gltf_ptr->accessors_ptr[attrib_accessor_idx].buffer_view_idx;
  gltf_component_type_t comp_type = gltf_ptr->accessors_ptr[attrib_accessor_idx].component_type; // # bytes per component
  *count                          = gltf_ptr->accessors_ptr[attrib_accessor_idx].count;
  gltf_type_t type                = gltf_ptr->accessors_ptr[attrib_accessor_idx].type;

  // buffer view
  int buffer_idx  = gltf_ptr->buffer_views_ptr[buffer_view_idx].buffer_idx;
  int byte_offset = gltf_ptr->buffer_views_ptr[buffer_view_idx].byte_offset;
  // int byte_length = gltf_ptr->buffer_views_ptr[buffer_view_idx].byte_length;
  // int byte_stride = gltf_ptr->buffer_views_ptr[buffer_view_idx].byte_stride; // NOTE(Anton) ignored until interleaved mesh found

  // access the buffer
  uint8_t* byte_ptr   = (uint8_t*)buffers_ptr[buffer_idx].data;
  int bytes_per_comp  = gltf_bytes_for_component( comp_type );
  int comps_per_vert  = gltf_comps_in_type( type );
  uint8_t* attrib_ptr = &byte_ptr[byte_offset];
  return attrib_ptr;
}

bool gfx_gltf_load( const char* filename, gfx_gltf_t* gfx_gltf_ptr ) {
  if ( !filename || !gfx_gltf_ptr ) { return false; }

  char asset_path[2048];
  { // work out subfolder to find relative path to images etc
    asset_path[0]  = '\0';
    int last_slash = -1;
    int len        = strlen( filename );
    for ( int i = 0; i < len; i++ ) {
      if ( filename[i] == '\\' || filename[i] == '/' ) { last_slash = i; }
    }
    if ( last_slash > -1 ) {
      strncpy( asset_path, filename, last_slash + 1 );
      asset_path[last_slash + 1] = '\0';
      printf( "using asset path: `%s`\n", asset_path );
      for ( int i = 0; i < last_slash + 1; i++ ) {
        if ( asset_path[i] == '\\' ) { asset_path[i] = '/'; }
      }
    }
  }

  //
  // load basic gltf structure json->structs
  //
  if ( !gltf_read( filename, &gfx_gltf_ptr->gltf ) ) {
    fprintf( stderr, "ERROR: reading gltf `%s`\n", filename );
    return false;
  }

  //
  // structs->gfx buffers
  //

  // count primitives == my 'mesh_t
  for ( int i = 0; i < gfx_gltf_ptr->gltf.n_meshes; i++ ) { gfx_gltf_ptr->n_meshes += gfx_gltf_ptr->gltf.meshes_ptr[i].n_primitives; }

  // alloc

  _entire_file_t* buffer_records_ptr = calloc( sizeof( _entire_file_t ), gfx_gltf_ptr->gltf.n_buffers );
  if ( !buffer_records_ptr ) { return false; }

  gfx_gltf_ptr->n_textures   = gfx_gltf_ptr->gltf.n_images;
  gfx_gltf_ptr->textures_ptr = calloc( sizeof( gfx_texture_t ), gfx_gltf_ptr->n_textures );
  if ( !gfx_gltf_ptr->textures_ptr ) { return false; }

  gfx_gltf_ptr->meshes_ptr = calloc( sizeof( gfx_mesh_t ), gfx_gltf_ptr->n_meshes );
  if ( !gfx_gltf_ptr->meshes_ptr ) { return false; }

	gfx_gltf_ptr->mat_idx_for_mesh_idx_ptr = calloc( sizeof( int ), gfx_gltf_ptr->n_meshes );
  if ( !gfx_gltf_ptr->mat_idx_for_mesh_idx_ptr ) { return false; }

  // load images -> textures
  // NOTE: assumes images are all external files here
  for ( int i = 0; i < gfx_gltf_ptr->n_textures; i++ ) {
    int x = 0, y = 0, comp = 0;
    char full_path[2048];
    strcpy( full_path, asset_path );
    strcat( full_path, gfx_gltf_ptr->gltf.images_ptr[i].uri_str );
    printf( "loading image %i: `%s`\n", i, full_path );
    uint8_t* img_ptr = stbi_load( full_path, &x, &y, &comp, 0 );
    if ( !img_ptr ) { return false; } // TODO(Anton) proper cleanup

    // NB: I don't specify sRGB here as it's controlled in the shader.
    gfx_gltf_ptr->textures_ptr[i] =
      gfx_create_texture_from_mem( img_ptr, x, y, comp, ( gfx_texture_properties_t ){ .bilinear = true, .has_mips = true, .repeats = true } );
    free( img_ptr );
    if ( !gfx_gltf_ptr->textures_ptr[i].handle_gl ) { return false; }
  }

  //
  // construct meshes from buffers
  //

  for ( int i = 0; i < gfx_gltf_ptr->gltf.n_buffers; i++ ) {
    char full_path[2048];
    strcpy( full_path, asset_path );
    strcat( full_path, gfx_gltf_ptr->gltf.buffers_ptr[i].uri_str );
    printf( "loading buffer %i: `%s`\n", i, full_path );
    if ( !gltf_read_entire_file( full_path, &buffer_records_ptr[i] ) ) { return false; }
  }

  int gfx_mesh_idx = 0;
  for ( int m = 0; m < gfx_gltf_ptr->gltf.n_meshes; m++ ) {
    for ( int p = 0; p < gfx_gltf_ptr->gltf.meshes_ptr[m].n_primitives; p++ ) {
      float* positions_ptr   = NULL;
      float* texcoords_0_ptr = NULL;
      float* normals_ptr     = NULL;
      float* tangents_ptr    = NULL;
      uint16_t* indices_ptr  = NULL; // TODO(Anton) assuming type here

      int n_vertices = 0;
      int n_indices  = 0;
      // NB can get min/max here and work out a scale
      texcoords_0_ptr = (float*)_byte_pointer_for_attrib(
        &gfx_gltf_ptr->gltf, buffer_records_ptr, gfx_gltf_ptr->gltf.meshes_ptr[m].primitives_ptr[p].attributes.texcoord_0_idx, &n_vertices );
      normals_ptr = (float*)_byte_pointer_for_attrib(
        &gfx_gltf_ptr->gltf, buffer_records_ptr, gfx_gltf_ptr->gltf.meshes_ptr[m].primitives_ptr[p].attributes.normal_idx, &n_vertices );
      tangents_ptr = (float*)_byte_pointer_for_attrib(
        &gfx_gltf_ptr->gltf, buffer_records_ptr, gfx_gltf_ptr->gltf.meshes_ptr[m].primitives_ptr[p].attributes.tangent_idx, &n_vertices );
      positions_ptr = (float*)_byte_pointer_for_attrib(
        &gfx_gltf_ptr->gltf, buffer_records_ptr, gfx_gltf_ptr->gltf.meshes_ptr[m].primitives_ptr[p].attributes.position_idx, &n_vertices );

      if ( gfx_gltf_ptr->gltf.meshes_ptr[m].primitives_ptr[p].indices_idx > -1 ) {
        indices_ptr =
          (uint16_t*)_byte_pointer_for_attrib( &gfx_gltf_ptr->gltf, buffer_records_ptr, gfx_gltf_ptr->gltf.meshes_ptr[m].primitives_ptr[p].indices_idx, &n_indices );
      }


      printf( "mesh %i n_vertices = %i n_indices= %i\n", gfx_mesh_idx, n_vertices, n_indices );
    
			bool calc_tans = true; // TODO(Anton) heap overflow in tan code
		  gfx_gltf_ptr->meshes_ptr[gfx_mesh_idx] = gfx_create_mesh_from_mem( positions_ptr, 3, texcoords_0_ptr, 2, normals_ptr, 3, NULL, 3, indices_ptr,
        sizeof( uint16_t ) * n_indices, GFX_INDICES_TYPE_UINT16, n_vertices, false, calc_tans );

      // material
			int mat_idx = gfx_gltf_ptr->gltf.meshes_ptr[m].primitives_ptr[p].material_idx;
			gfx_gltf_ptr->mat_idx_for_mesh_idx_ptr[m] = mat_idx;

      gfx_mesh_idx++;
    }
  }

  for ( int i = 0; i < gfx_gltf_ptr->gltf.n_buffers; i++ ) {
    if ( buffer_records_ptr[i].data ) { free( buffer_records_ptr[i].data ); }
  }
  free( buffer_records_ptr );

  return true;
}

bool gfx_gltf_free( gfx_gltf_t* gfx_gltf_ptr ) {
  if ( !gfx_gltf_ptr ) { return false; }

  if ( gfx_gltf_ptr->textures_ptr ) {
    for ( int i = 0; i < gfx_gltf_ptr->n_textures; i++ ) { gfx_delete_texture( &gfx_gltf_ptr->textures_ptr[i] ); }
    free( gfx_gltf_ptr->textures_ptr );
  }

  if ( gfx_gltf_ptr->meshes_ptr ) {
    for ( int i = 0; i < gfx_gltf_ptr->n_meshes; i++ ) { gfx_delete_mesh( &gfx_gltf_ptr->meshes_ptr[i] ); }
    free( gfx_gltf_ptr->meshes_ptr );
  }
	if (gfx_gltf_ptr->mat_idx_for_mesh_idx_ptr) { free(gfx_gltf_ptr->mat_idx_for_mesh_idx_ptr);}

  if ( !gltf_free( &gfx_gltf_ptr->gltf ) ) { return false; }
  memset( gfx_gltf_ptr, 0, sizeof( gfx_gltf_t ) );

  return true;
}

void gfx_gltf_draw( const gfx_gltf_t* gfx_gltf_ptr ) {
  if ( !gfx_gltf_ptr ) { return; }
}
