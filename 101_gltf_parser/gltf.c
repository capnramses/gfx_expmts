#include "gltf.h"
#include "cJSON/cJSON.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

// NB If I wrote my own JSON parser it could directly copy into these structs... (I end up with like 3 versions of file data in mem)
bool gltf_read( const char* filename, gltf_t* gltf_ptr ) {
  _entire_file_t record = { 0 };
  bool ret              = gltf_read_entire_file( filename, &record );
  if ( !ret ) { return false; }

  char* json_char_ptr = (char*)record.data;

  cJSON* json_ptr = cJSON_ParseWithLength( json_char_ptr, record.sz );
  if ( !json_ptr ) {
    fprintf( stderr, "ERROR: gltf. cJSON_ParseWithLength\n" );
    free( record.data );
    return false;
  }

  // "asset"
  cJSON* asset_ptr = cJSON_GetObjectItem( json_ptr, "asset" );
  if ( asset_ptr ) {
    cJSON* version_ptr = cJSON_GetObjectItem( asset_ptr, "version" );
    if ( version_ptr ) { strncat( gltf_ptr->version_str, version_ptr->valuestring, 15 ); }
  }

  // get top-level objects
  cJSON* scene_ptr = cJSON_GetObjectItem( json_ptr, "scene" );
  // and arrays
  cJSON* accessors_ptr    = cJSON_GetObjectItem( json_ptr, "accessors" );
  cJSON* buffers_ptr      = cJSON_GetObjectItem( json_ptr, "buffers" );
  cJSON* buffer_views_ptr = cJSON_GetObjectItem( json_ptr, "bufferViews" );
  cJSON* images_ptr       = cJSON_GetObjectItem( json_ptr, "images" );
  cJSON* materials_ptr    = cJSON_GetObjectItem( json_ptr, "materials" );
  cJSON* meshes_ptr       = cJSON_GetObjectItem( json_ptr, "meshes" );
  cJSON* nodes_ptr        = cJSON_GetObjectItem( json_ptr, "nodes" );
  cJSON* samplers_ptr     = cJSON_GetObjectItem( json_ptr, "samplers" );
  cJSON* scenes_ptr       = cJSON_GetObjectItem( json_ptr, "scenes" );
  cJSON* textures_ptr     = cJSON_GetObjectItem( json_ptr, "textures" );

  //
  // count top-level array elements
  //
  if ( accessors_ptr && cJSON_IsArray( accessors_ptr ) ) { gltf_ptr->n_accessors = cJSON_GetArraySize( accessors_ptr ); }
  if ( buffers_ptr && cJSON_IsArray( buffers_ptr ) ) { gltf_ptr->n_buffers = cJSON_GetArraySize( buffers_ptr ); }
  if ( buffer_views_ptr && cJSON_IsArray( buffer_views_ptr ) ) { gltf_ptr->n_buffer_views = cJSON_GetArraySize( buffer_views_ptr ); }
  if ( images_ptr && cJSON_IsArray( images_ptr ) ) { gltf_ptr->n_images = cJSON_GetArraySize( images_ptr ); }
  if ( materials_ptr && cJSON_IsArray( materials_ptr ) ) { gltf_ptr->n_materials = cJSON_GetArraySize( materials_ptr ); }
  if ( meshes_ptr && cJSON_IsArray( meshes_ptr ) ) { gltf_ptr->n_meshes = cJSON_GetArraySize( meshes_ptr ); }
  if ( nodes_ptr && cJSON_IsArray( nodes_ptr ) ) { gltf_ptr->n_nodes = cJSON_GetArraySize( nodes_ptr ); }
  if ( samplers_ptr && cJSON_IsArray( samplers_ptr ) ) { gltf_ptr->n_samplers = cJSON_GetArraySize( samplers_ptr ); }
  if ( scenes_ptr && cJSON_IsArray( scenes_ptr ) ) { gltf_ptr->n_scenes = cJSON_GetArraySize( scenes_ptr ); }
  if ( textures_ptr && cJSON_IsArray( textures_ptr ) ) { gltf_ptr->n_textures = cJSON_GetArraySize( textures_ptr ); }

  //
  // alloc structs
  //
  if ( gltf_ptr->n_accessors > 0 ) {
    gltf_ptr->accessors_ptr = calloc( sizeof( gltf_accessor_t ), gltf_ptr->n_accessors );
    assert( gltf_ptr->accessors_ptr );
  }
  if ( gltf_ptr->n_buffers > 0 ) {
    gltf_ptr->buffers_ptr = calloc( sizeof( gltf_buffer_t ), gltf_ptr->n_buffers );
    assert( gltf_ptr->buffers_ptr );
  }
  if ( gltf_ptr->n_buffer_views > 0 ) {
    gltf_ptr->buffer_views_ptr = calloc( sizeof( gltf_buffer_view_t ), gltf_ptr->n_buffer_views );
    assert( gltf_ptr->buffer_views_ptr );
  }
  if ( gltf_ptr->n_images > 0 ) {
    gltf_ptr->images_ptr = calloc( sizeof( gltf_image_t ), gltf_ptr->n_images );
    assert( gltf_ptr->images_ptr );
  }
  if ( gltf_ptr->n_materials > 0 ) {
    gltf_ptr->materials_ptr = calloc( sizeof( gltf_material_t ), gltf_ptr->n_materials );
    assert( gltf_ptr->materials_ptr );
  }
  if ( gltf_ptr->n_meshes > 0 ) {
    gltf_ptr->meshes_ptr = calloc( sizeof( gltf_mesh_t ), gltf_ptr->n_meshes );
    assert( gltf_ptr->meshes_ptr );
    for ( int m = 0; m < gltf_ptr->n_meshes; m++ ) {
      cJSON* mesh_ptr       = cJSON_GetArrayItem( meshes_ptr, m );
      cJSON* primitives_ptr = cJSON_GetObjectItem( mesh_ptr, "primitives" );
      if ( primitives_ptr && cJSON_IsArray( primitives_ptr ) ) {
        gltf_ptr->meshes_ptr[m].n_primitives   = cJSON_GetArraySize( primitives_ptr );
        gltf_ptr->meshes_ptr[m].primitives_ptr = calloc( sizeof( gltf_primitive_t ), gltf_ptr->meshes_ptr[m].n_primitives );
        assert( gltf_ptr->meshes_ptr[m].primitives_ptr );
      }
    }
  }
  if ( gltf_ptr->n_nodes > 0 ) {
    gltf_ptr->nodes_ptr = calloc( sizeof( gltf_node_t ), gltf_ptr->n_nodes );
    assert( gltf_ptr->nodes_ptr );
  }
  if ( gltf_ptr->n_samplers > 0 ) {
    gltf_ptr->samplers_ptr = calloc( sizeof( gltf_sampler_t ), gltf_ptr->n_samplers );
    assert( gltf_ptr->samplers_ptr );
  }
  if ( gltf_ptr->n_scenes > 0 ) {
    gltf_ptr->scenes_ptr = calloc( sizeof( gltf_scene_t ), gltf_ptr->n_scenes );
    assert( gltf_ptr->scenes_ptr );

    for ( int s = 0; s < gltf_ptr->n_scenes; s++ ) {
      cJSON* scene_ptr      = cJSON_GetArrayItem( scenes_ptr, s );
      cJSON* nodes_list_ptr = cJSON_GetObjectItem( scene_ptr, "nodes" );
      if ( nodes_list_ptr && cJSON_IsArray( nodes_list_ptr ) ) {
        gltf_ptr->scenes_ptr[s].n_node_idxs = cJSON_GetArraySize( nodes_list_ptr );
        if ( gltf_ptr->scenes_ptr[s].n_node_idxs > 0 ) {
          gltf_ptr->scenes_ptr[s].node_idxs_ptr = calloc( sizeof( int ), gltf_ptr->scenes_ptr[s].n_node_idxs );
          assert( gltf_ptr->scenes_ptr[s].node_idxs_ptr );
        }
      }
    }
  }
  if ( gltf_ptr->n_textures > 0 ) {
    gltf_ptr->textures_ptr = calloc( sizeof( gltf_texture_t ), gltf_ptr->n_textures );
    assert( gltf_ptr->textures_ptr );
  }

  //
  // parse into structs
  //

  // "scene"
  if ( scene_ptr ) { gltf_ptr->default_scene_idx = scene_ptr->valueint; }

  // "accessors"
  for ( int a = 0; a < gltf_ptr->n_accessors; a++ ) {
    gltf_ptr->accessors_ptr[a].buffer_view_idx = -1;

    cJSON* accessor_ptr = cJSON_GetArrayItem( accessors_ptr, a );

    cJSON* buffer_view_ptr    = cJSON_GetObjectItem( accessor_ptr, "bufferView" );
    cJSON* byte_offset_ptr    = cJSON_GetObjectItem( accessor_ptr, "byteOffset" );
    cJSON* component_type_ptr = cJSON_GetObjectItem( accessor_ptr, "componentType" );
    cJSON* count_ptr          = cJSON_GetObjectItem( accessor_ptr, "count" );
    cJSON* type_ptr           = cJSON_GetObjectItem( accessor_ptr, "type" );
    cJSON* name_ptr           = cJSON_GetObjectItem( accessor_ptr, "name" );
    cJSON* max_ptr            = cJSON_GetObjectItem( accessor_ptr, "max" );
    cJSON* min_ptr            = cJSON_GetObjectItem( accessor_ptr, "min" );
    if ( buffer_view_ptr ) { gltf_ptr->accessors_ptr[a].buffer_view_idx = buffer_view_ptr->valueint; }
    if ( byte_offset_ptr ) { gltf_ptr->accessors_ptr[a].byte_offset = byte_offset_ptr->valueint; }
    if ( component_type_ptr ) { gltf_ptr->accessors_ptr[a].component_type = (gltf_component_type_t)component_type_ptr->valueint; }
    if ( count_ptr ) { gltf_ptr->accessors_ptr[a].count = count_ptr->valueint; }
    if ( max_ptr && cJSON_IsArray( max_ptr ) && ( 3 == cJSON_GetArraySize( max_ptr ) ) ) {
      gltf_ptr->accessors_ptr[a].has_max = true;
      gltf_ptr->accessors_ptr[a].max[0]  = (float)cJSON_GetArrayItem( max_ptr, 0 )->valuedouble;
      gltf_ptr->accessors_ptr[a].max[1]  = (float)cJSON_GetArrayItem( max_ptr, 1 )->valuedouble;
      gltf_ptr->accessors_ptr[a].max[2]  = (float)cJSON_GetArrayItem( max_ptr, 2 )->valuedouble;
    }
    if ( min_ptr && cJSON_IsArray( min_ptr ) && ( 3 == cJSON_GetArraySize( min_ptr ) ) ) {
      gltf_ptr->accessors_ptr[a].has_min = true;
      gltf_ptr->accessors_ptr[a].min[0]  = (float)cJSON_GetArrayItem( min_ptr, 0 )->valuedouble;
      gltf_ptr->accessors_ptr[a].min[1]  = (float)cJSON_GetArrayItem( min_ptr, 1 )->valuedouble;
      gltf_ptr->accessors_ptr[a].min[2]  = (float)cJSON_GetArrayItem( min_ptr, 2 )->valuedouble;
    }
    if ( name_ptr ) { strncat( gltf_ptr->accessors_ptr[a].name_str, name_ptr->valuestring, GLTF_NAME_MAX - 1 ); }
    if ( type_ptr ) {
      if ( strncmp( type_ptr->valuestring, "SCALAR", 10 ) == 0 ) {
        gltf_ptr->accessors_ptr[a].type = GLTF_SCALAR;
      } else if ( strncmp( type_ptr->valuestring, "VEC2", 10 ) == 0 ) {
        gltf_ptr->accessors_ptr[a].type = GLTF_VEC2;
      } else if ( strncmp( type_ptr->valuestring, "VEC3", 10 ) == 0 ) {
        gltf_ptr->accessors_ptr[a].type = GLTF_VEC3;
      } else if ( strncmp( type_ptr->valuestring, "VEC4", 10 ) == 0 ) {
        gltf_ptr->accessors_ptr[a].type = GLTF_VEC4;
      } else if ( strncmp( type_ptr->valuestring, "MAT2", 10 ) == 0 ) {
        gltf_ptr->accessors_ptr[a].type = GLTF_MAT2;
      } else if ( strncmp( type_ptr->valuestring, "MAT3", 10 ) == 0 ) {
        gltf_ptr->accessors_ptr[a].type = GLTF_MAT3;
      } else if ( strncmp( type_ptr->valuestring, "MAT4", 10 ) == 0 ) {
        gltf_ptr->accessors_ptr[a].type = GLTF_MAT4;
      }
    }
  } // endfor accessors

  // "buffers"
  for ( int b = 0; b < gltf_ptr->n_buffers; b++ ) {
    cJSON* b_ptr          = cJSON_GetArrayItem( buffers_ptr, b );
    cJSON* byteLength_ptr = cJSON_GetObjectItem( b_ptr, "byteLength" );
    cJSON* uri_ptr        = cJSON_GetObjectItem( b_ptr, "uri" );
    if ( byteLength_ptr ) { gltf_ptr->buffers_ptr[b].byte_length = byteLength_ptr->valueint; }
    if ( uri_ptr ) { strncat( gltf_ptr->buffers_ptr[b].uri_str, uri_ptr->valuestring, GLTF_URI_MAX - 1 ); }
  } // endfor n_buffers

  // "bufferViews"
  for ( int bv = 0; bv < gltf_ptr->n_buffer_views; bv++ ) {
    cJSON* bv_ptr = cJSON_GetArrayItem( buffer_views_ptr, bv );

    cJSON* buffer_ptr     = cJSON_GetObjectItem( bv_ptr, "buffer" );
    cJSON* byteOffset_ptr = cJSON_GetObjectItem( bv_ptr, "byteOffset" );
    cJSON* byteLength_ptr = cJSON_GetObjectItem( bv_ptr, "byteLength" );
    cJSON* byteStride_ptr = cJSON_GetObjectItem( bv_ptr, "byteStride" );
    cJSON* name_ptr       = cJSON_GetObjectItem( bv_ptr, "name" );

    if ( buffer_ptr ) { gltf_ptr->buffer_views_ptr[bv].buffer_idx = buffer_ptr->valueint; }
    if ( byteOffset_ptr ) { gltf_ptr->buffer_views_ptr[bv].byte_offset = byteOffset_ptr->valueint; }
    if ( byteLength_ptr ) { gltf_ptr->buffer_views_ptr[bv].byte_length = byteLength_ptr->valueint; }
    if ( byteStride_ptr ) { gltf_ptr->buffer_views_ptr[bv].byte_stride = byteStride_ptr->valueint; }
    if ( name_ptr ) { strncat( gltf_ptr->buffer_views_ptr[bv].name_str, name_ptr->valuestring, GLTF_NAME_MAX - 1 ); }
  } // endfor n_buffer_views

  // "images"
  for ( int i = 0; i < gltf_ptr->n_images; i++ ) {
    cJSON* i_ptr   = cJSON_GetArrayItem( images_ptr, i );
    cJSON* uri_ptr = cJSON_GetObjectItem( i_ptr, "uri" );
    if ( uri_ptr ) { strncat( gltf_ptr->images_ptr[i].uri_str, uri_ptr->valuestring, GLTF_URI_MAX - 1 ); }
  }

  // "materials"
  for ( int m = 0; m < gltf_ptr->n_materials; m++ ) {
    cJSON* m_ptr                = cJSON_GetArrayItem( materials_ptr, m );
    cJSON* name_ptr             = cJSON_GetObjectItem( m_ptr, "name" );
    cJSON* normalTexture_ptr    = cJSON_GetObjectItem( m_ptr, "normalTexture" );
    cJSON* occlusionTexture_ptr = cJSON_GetObjectItem( m_ptr, "occlusionTexture" );
    cJSON* alphaMode_ptr        = cJSON_GetObjectItem( m_ptr, "alphaMode" );
    cJSON* doubleSided_ptr      = cJSON_GetObjectItem( m_ptr, "doubleSided" );
    cJSON* pbr_ptr              = cJSON_GetObjectItem( m_ptr, "pbrMetallicRoughness" );

    gltf_ptr->materials_ptr[m].normal_texture_idx                                    = -1;
    gltf_ptr->materials_ptr[m].occlusion_texture_idx                                 = -1;
    gltf_ptr->materials_ptr[m].pbr_metallic_roughness.base_colour_texture_idx        = -1;
    gltf_ptr->materials_ptr[m].pbr_metallic_roughness.metallic_roughness_texture_idx = -1;
    if ( normalTexture_ptr ) { gltf_ptr->materials_ptr[m].normal_texture_idx = normalTexture_ptr->valueint; }
    if ( occlusionTexture_ptr ) { gltf_ptr->materials_ptr[m].occlusion_texture_idx = occlusionTexture_ptr->valueint; }
    if ( alphaMode_ptr ) { gltf_ptr->materials_ptr[m].alpha_blend = true; }
    if ( doubleSided_ptr ) { gltf_ptr->materials_ptr[m].is_doubled_sided = true; }
    if ( name_ptr ) { strncat( gltf_ptr->materials_ptr[m].name_str, name_ptr->valuestring, GLTF_NAME_MAX - 1 ); }
    if ( pbr_ptr ) {
      cJSON* baseColorTexture_ptr         = cJSON_GetObjectItem( pbr_ptr, "baseColorTexture" );
      cJSON* metallicRoughnessTexture_ptr = cJSON_GetObjectItem( pbr_ptr, "metallicRoughnessTexture" );
      if ( baseColorTexture_ptr ) { gltf_ptr->materials_ptr[m].pbr_metallic_roughness.base_colour_texture_idx = baseColorTexture_ptr->valueint; }
      if ( metallicRoughnessTexture_ptr ) {
        gltf_ptr->materials_ptr[m].pbr_metallic_roughness.metallic_roughness_texture_idx = metallicRoughnessTexture_ptr->valueint;
      }
    }
  }

  // "meshes"
  for ( int m = 0; m < gltf_ptr->n_meshes; m++ ) {
    cJSON* mesh_ptr = cJSON_GetArrayItem( meshes_ptr, m );

    cJSON* name_ptr = cJSON_GetObjectItem( mesh_ptr, "name" );
    if ( name_ptr ) { strncat( gltf_ptr->meshes_ptr[m].name_str, name_ptr->valuestring, GLTF_NAME_MAX - 1 ); }

    cJSON* primitives_ptr = cJSON_GetObjectItem( mesh_ptr, "primitives" );
    for ( int p = 0; p < gltf_ptr->meshes_ptr[m].n_primitives; p++ ) {
      cJSON* primitive_ptr = cJSON_GetArrayItem( primitives_ptr, p );

      gltf_ptr->meshes_ptr[m].primitives_ptr[p].attributes.position_idx   = -1;
      gltf_ptr->meshes_ptr[m].primitives_ptr[p].attributes.tangent_idx    = -1;
      gltf_ptr->meshes_ptr[m].primitives_ptr[p].attributes.normal_idx     = -1;
      gltf_ptr->meshes_ptr[m].primitives_ptr[p].attributes.texcoord_0_idx = -1;
      gltf_ptr->meshes_ptr[m].primitives_ptr[p].material_idx              = -1;
      gltf_ptr->meshes_ptr[m].primitives_ptr[p].indices_idx               = -1;

      cJSON* attributes_ptr = cJSON_GetObjectItem( primitive_ptr, "attributes" );
      if ( attributes_ptr ) {
        cJSON* attribute_ptr = cJSON_GetObjectItem( attributes_ptr, "POSITION" );
        if ( attribute_ptr ) { gltf_ptr->meshes_ptr[m].primitives_ptr[p].attributes.position_idx = attribute_ptr->valueint; }
        attribute_ptr = cJSON_GetObjectItem( attributes_ptr, "TANGENT" );
        if ( attribute_ptr ) { gltf_ptr->meshes_ptr[m].primitives_ptr[p].attributes.tangent_idx = attribute_ptr->valueint; }
        attribute_ptr = cJSON_GetObjectItem( attributes_ptr, "NORMAL" );
        if ( attribute_ptr ) { gltf_ptr->meshes_ptr[m].primitives_ptr[p].attributes.normal_idx = attribute_ptr->valueint; }
        attribute_ptr = cJSON_GetObjectItem( attributes_ptr, "TEXCOORD_0" );
        if ( attribute_ptr ) { gltf_ptr->meshes_ptr[m].primitives_ptr[p].attributes.texcoord_0_idx = attribute_ptr->valueint; }
      }
      cJSON* material_ptr = cJSON_GetObjectItem( primitive_ptr, "material" );
      if ( material_ptr ) { gltf_ptr->meshes_ptr[m].primitives_ptr[p].material_idx = material_ptr->valueint; }
      cJSON* indices_ptr = cJSON_GetObjectItem( primitive_ptr, "indices" );
      if ( indices_ptr ) { gltf_ptr->meshes_ptr[m].primitives_ptr[p].indices_idx = indices_ptr->valueint; }

    } // endfor n_primitives
  }   // endfor n_meshes

  // "nodes"
  for ( int n = 0; n < gltf_ptr->n_nodes; n++ ) {
    gltf_ptr->nodes_ptr[n].mesh_idx = -1;
    cJSON* node_ptr                 = cJSON_GetArrayItem( nodes_ptr, n );
    cJSON* mesh_ptr                 = cJSON_GetObjectItem( node_ptr, "mesh" );
    if ( mesh_ptr ) { gltf_ptr->nodes_ptr[n].mesh_idx = mesh_ptr->valueint; }
    cJSON* name_ptr = cJSON_GetObjectItem( node_ptr, "name" );
    if ( name_ptr ) { strncat( gltf_ptr->nodes_ptr[n].name_str, name_ptr->valuestring, GLTF_NAME_MAX - 1 ); }
  }

  // samplers
  for ( int s = 0; s < gltf_ptr->n_samplers; s++ ) {
    cJSON* s_ptr         = cJSON_GetArrayItem( samplers_ptr, s );
    cJSON* magFilter_ptr = cJSON_GetObjectItem( s_ptr, "magFilter" );
    cJSON* minFilter_ptr = cJSON_GetObjectItem( s_ptr, "minFilter" );
    if ( magFilter_ptr ) {
      gltf_ptr->samplers_ptr[s].mag_filter     = samplers_ptr->valueint;
      gltf_ptr->samplers_ptr[s].has_mag_filter = true;
    }
    if ( minFilter_ptr ) {
      gltf_ptr->samplers_ptr[s].min_filter     = samplers_ptr->valueint;
      gltf_ptr->samplers_ptr[s].has_min_filter = true;
    }
  }

  // "scenes"
  for ( int s = 0; s < gltf_ptr->n_scenes; s++ ) {
    cJSON* scene_ptr      = cJSON_GetArrayItem( scenes_ptr, s );
    cJSON* nodes_list_ptr = cJSON_GetObjectItem( scene_ptr, "nodes" );
    for ( int n = 0; n < gltf_ptr->scenes_ptr[s].n_node_idxs; n++ ) {
      gltf_ptr->scenes_ptr[s].node_idxs_ptr[n] = cJSON_GetArrayItem( nodes_list_ptr, n )->valueint;
    }
  }

  // "textures"
  for ( int t = 0; t < gltf_ptr->n_textures; t++ ) {
    cJSON* t_ptr       = cJSON_GetArrayItem( textures_ptr, t );
    cJSON* sampler_ptr = cJSON_GetObjectItem( t_ptr, "sampler" );
    cJSON* source_ptr  = cJSON_GetObjectItem( t_ptr, "source" );
    cJSON* name_ptr    = cJSON_GetObjectItem( t_ptr, "name" );
    if ( sampler_ptr ) { gltf_ptr->textures_ptr[t].sampler_idx = sampler_ptr->valueint; }
    if ( source_ptr ) { gltf_ptr->textures_ptr[t].source_idx = source_ptr->valueint; }
    if ( name_ptr ) { strncat( gltf_ptr->textures_ptr[t].name_str, name_ptr->valuestring, GLTF_NAME_MAX - 1 ); }
  }

  cJSON_Delete( json_ptr );
  free( record.data );
  return true;
}

bool gltf_free( gltf_t* gltf_ptr ) {
  if ( !gltf_ptr ) { return false; }
  if ( gltf_ptr->accessors_ptr ) { free( gltf_ptr->accessors_ptr ); }
  if ( gltf_ptr->buffers_ptr ) { free( gltf_ptr->buffers_ptr ); }
  if ( gltf_ptr->buffer_views_ptr ) { free( gltf_ptr->buffer_views_ptr ); }
  if ( gltf_ptr->images_ptr ) { free( gltf_ptr->images_ptr ); }
  if ( gltf_ptr->materials_ptr ) { free( gltf_ptr->materials_ptr ); }
  if ( gltf_ptr->meshes_ptr ) {
    for ( int m = 0; m < gltf_ptr->n_meshes; m++ ) {
      if ( gltf_ptr->meshes_ptr[m].primitives_ptr ) { free( gltf_ptr->meshes_ptr[m].primitives_ptr ); }
    }
    free( gltf_ptr->meshes_ptr );
  }
  if ( gltf_ptr->nodes_ptr ) { free( gltf_ptr->nodes_ptr ); }
  if ( gltf_ptr->samplers_ptr ) { free( gltf_ptr->samplers_ptr ); }
  if ( gltf_ptr->scenes_ptr ) {
    for ( int s = 0; s < gltf_ptr->n_scenes; s++ ) {
      if ( gltf_ptr->scenes_ptr[s].node_idxs_ptr ) { free( gltf_ptr->scenes_ptr[s].node_idxs_ptr ); }
    }
    free( gltf_ptr->scenes_ptr );
  }
  if ( gltf_ptr->textures_ptr ) { free( gltf_ptr->textures_ptr ); }

  memset( gltf_ptr, 0, sizeof( gltf_t ) );
  return true;
}

void gltf_print( const gltf_t* gltf_ptr ) {
  printf( "version \t= %s\n", gltf_ptr->version_str );
  printf( "n_scenes \t= %i\n", gltf_ptr->n_scenes );
  printf( "n_nodes \t= %i\n", gltf_ptr->n_nodes );
  printf( "scene \t\t= %i\n", gltf_ptr->default_scene_idx );
  printf( "\nscenes:\n" );
  for ( int s = 0; s < gltf_ptr->n_scenes; s++ ) {
    printf( "  scene %i:\n", s );
    for ( int n = 0; n < gltf_ptr->scenes_ptr[s].n_node_idxs; n++ ) { printf( "   node %i \t= %i\n", n, gltf_ptr->scenes_ptr[s].node_idxs_ptr[n] ); }
  }
  printf( "\nnodes:\n" );
  for ( int n = 0; n < gltf_ptr->n_nodes; n++ ) {
    printf( "node %i:\n", n );
    if ( gltf_ptr->nodes_ptr[n].name_str[0] != '\0' ) { printf( "    name \t= `%s`\n", gltf_ptr->nodes_ptr[n].name_str ); }
    if ( gltf_ptr->nodes_ptr[n].mesh_idx > -1 ) { printf( "    mesh \t= %i\n", gltf_ptr->nodes_ptr[n].mesh_idx ); }
  }
  printf( "\nmeshes:\n" );
  for ( int m = 0; m < gltf_ptr->n_meshes; m++ ) {
    printf( "  mesh %i:\n", m );
    if ( gltf_ptr->meshes_ptr[m].name_str[0] != '\0' ) { printf( "    name \t\t= `%s`\n", gltf_ptr->meshes_ptr[m].name_str ); }
    printf( "    n_primitives \t= %i\n", gltf_ptr->meshes_ptr[m].n_primitives );
    for ( int p = 0; p < gltf_ptr->meshes_ptr[m].n_primitives; p++ ) {
      printf( "      primitive \t= %i\n", p );
      printf( "        POSITION \t= %i\n", gltf_ptr->meshes_ptr[m].primitives_ptr[p].attributes.position_idx );
      printf( "        TANGENT \t= %i\n", gltf_ptr->meshes_ptr[m].primitives_ptr[p].attributes.tangent_idx );
      printf( "        NORMAL \t\t= %i\n", gltf_ptr->meshes_ptr[m].primitives_ptr[p].attributes.normal_idx );
      printf( "        TEXCOORD_0 \t= %i\n", gltf_ptr->meshes_ptr[m].primitives_ptr[p].attributes.texcoord_0_idx );
      printf( "        indices \t= %i\n", gltf_ptr->meshes_ptr[m].primitives_ptr[p].indices_idx );
      printf( "        material \t= %i\n", gltf_ptr->meshes_ptr[m].primitives_ptr[p].material_idx );
    }
    //
    //
  }
  printf( "\naccessors:\n" );
  for ( int a = 0; a < gltf_ptr->n_accessors; a++ ) {
    printf( "  accessor %i:\n", a );
    printf( "    buffer view \t= %i\n", gltf_ptr->accessors_ptr[a].buffer_view_idx );
    printf( "    buffer offset \t= %i\n", gltf_ptr->accessors_ptr[a].byte_offset );
    switch ( gltf_ptr->accessors_ptr[a].component_type ) {
    case GLTF_BYTE: printf( "    component type \t= BYTE\n" ); break;
    case GLTF_UNSIGNED_BYTE: printf( "    component type \t= UNSIGNED_BYTE\n" ); break;
    case GLTF_SHORT: printf( "    component type \t= SHORT\n" ); break;
    case GLTF_UNSIGNED_SHORT: printf( "    component type \t= UNSIGNED_SHORT\n" ); break;
    case GLTF_UNSIGNED_INT: printf( "    component type \t= UNSIGNED_INT\n" ); break;
    case GLTF_FLOAT: printf( "    component type \t= FLOAT\n" ); break;
    default: printf( "    component type \t= UNKNOWN\n" ); break;
    } // endswitch
    printf( "    count \t\t= %i\n", gltf_ptr->accessors_ptr[a].count );
    switch ( gltf_ptr->accessors_ptr[a].type ) {
    case GLTF_SCALAR: printf( "    type \t\t= SCALAR\n" ); break;
    case GLTF_VEC2: printf( "    type \t\t= VEC2\n" ); break;
    case GLTF_VEC3: printf( "    type \t\t= VEC3\n" ); break;
    case GLTF_VEC4: printf( "    type \t\t= VEC4\n" ); break;
    case GLTF_MAT2: printf( "    type \t\t= MAT2\n" ); break;
    case GLTF_MAT3: printf( "    type \t\t= MAT3\n" ); break;
    case GLTF_MAT4: printf( "    type \t\t= MAT4\n" ); break;
    default: printf( "    type \t\t= UNKNOWN\n" ); break;
    } // endswitch
    if ( gltf_ptr->accessors_ptr[a].has_max ) {
      printf( "    max \t\t= (%.4f, %.4f, %.4f)\n", gltf_ptr->accessors_ptr[a].max[0], gltf_ptr->accessors_ptr[a].max[1], gltf_ptr->accessors_ptr[a].max[2] );
    }
    if ( gltf_ptr->accessors_ptr[a].has_min ) {
      printf( "    min \t\t= (%.4f, %.4f, %.4f)\n", gltf_ptr->accessors_ptr[a].min[0], gltf_ptr->accessors_ptr[a].min[1], gltf_ptr->accessors_ptr[a].min[2] );
    }
    printf( "    name \t\t= %s\n", gltf_ptr->accessors_ptr[a].name_str );
  }

  printf( "\nbuffer views:\n" );
  for ( int bv = 0; bv < gltf_ptr->n_buffer_views; bv++ ) {
    printf( "  buffer views %i:\n", bv );
    printf( "    buffer \t\t= %i\n", gltf_ptr->buffer_views_ptr[bv].buffer_idx );
    printf( "    byte offset \t= %i\n", gltf_ptr->buffer_views_ptr[bv].byte_offset );
    printf( "    byte length \t= %i\n", gltf_ptr->buffer_views_ptr[bv].byte_length );
    printf( "    byte stride \t= %i\n", gltf_ptr->buffer_views_ptr[bv].byte_stride );
    printf( "    name \t\t= %s\n", gltf_ptr->buffer_views_ptr[bv].name_str );
  }

  printf( "\nbuffers:\n" );
  for ( int b = 0; b < gltf_ptr->n_buffers; b++ ) {
    printf( "    byte length \t= %i\n", gltf_ptr->buffers_ptr[b].byte_length );
    printf( "    uri \t\t= %s\n", gltf_ptr->buffers_ptr[b].uri_str );
  } //
}

int gltf_bytes_for_component( gltf_component_type_t comp_type ) {
  switch ( comp_type ) {
  case GLTF_BYTE: return 1;
  case GLTF_UNSIGNED_BYTE: return 1;
  case GLTF_SHORT: return 2;
  case GLTF_UNSIGNED_SHORT: return 2;
  case GLTF_UNSIGNED_INT: return 4;
  case GLTF_FLOAT: return 4;
  default: break;
  }
  return 0;
}

int gltf_comps_in_type( gltf_type_t type ) {
  switch ( type ) {
  case GLTF_SCALAR: return 1;
  case GLTF_VEC2: return 2;
  case GLTF_VEC3: return 3;
  case GLTF_VEC4: return 4;
  case GLTF_MAT2: return 4;
  case GLTF_MAT3: return 9;
  case GLTF_MAT4: return 16;
  default: break;
  }
  return 0;
}
