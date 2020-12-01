#include "gltf.h"
#include "cJSON/cJSON.h"
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
static bool _read_entire_file( const char* filename, _entire_file_t* record ) {
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
  bool ret              = _read_entire_file( filename, &record );
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

  // "scene"
  cJSON* scene_ptr = cJSON_GetObjectItem( json_ptr, "scene" );
  if ( scene_ptr ) { gltf_ptr->default_scene_idx = scene_ptr->valueint; }

  // "scenes"
  cJSON* scenes_ptr = cJSON_GetObjectItem( json_ptr, "scenes" );
  if ( scenes_ptr && cJSON_IsArray( scenes_ptr ) ) { gltf_ptr->n_scenes = cJSON_GetArraySize( scenes_ptr ); }
  gltf_ptr->scenes_ptr = calloc( sizeof( gltf_scene_t ), gltf_ptr->n_scenes );
  if ( !gltf_ptr->scenes_ptr ) {
    fprintf( stderr, "ERROR: gltf. OOM scenes_ptr\n" );
    cJSON_Delete( json_ptr );
    free( record.data );
    return false;
  }
  for ( int s = 0; s < gltf_ptr->n_scenes; s++ ) {
    cJSON* scene_ptr      = cJSON_GetArrayItem( scenes_ptr, s );
    cJSON* nodes_list_ptr = cJSON_GetObjectItem( scene_ptr, "nodes" );
    if ( nodes_list_ptr && cJSON_IsArray( nodes_list_ptr ) ) {
      gltf_ptr->scenes_ptr[s].n_node_idxs = cJSON_GetArraySize( nodes_list_ptr );
      printf( "gltf_ptr->scenes_ptr[%i].n_node_idxs=%i\n", s, gltf_ptr->scenes_ptr[s].n_node_idxs );
      if ( gltf_ptr->scenes_ptr[s].n_node_idxs > 0 ) {
        gltf_ptr->scenes_ptr[s].node_idxs_ptr = calloc( sizeof( int ), gltf_ptr->scenes_ptr[s].n_node_idxs );
        if ( !gltf_ptr->scenes_ptr[s].node_idxs_ptr ) {
          fprintf( stderr, "ERROR: gltf. OOM node_idxs_ptr\n" );
          cJSON_Delete( json_ptr );
          free( record.data );
          return false;
        }
        for ( int n = 0; n < gltf_ptr->scenes_ptr[s].n_node_idxs; n++ ) {
          gltf_ptr->scenes_ptr[s].node_idxs_ptr[n] = cJSON_GetArrayItem( nodes_list_ptr, n )->valueint;
        }
      }
    }
  }

  // "nodes"
  cJSON* nodes_ptr = cJSON_GetObjectItem( json_ptr, "nodes" );
  if ( nodes_ptr && cJSON_IsArray( nodes_ptr ) ) {
    gltf_ptr->n_nodes = cJSON_GetArraySize( nodes_ptr );
    if ( gltf_ptr->n_nodes > 0 ) {
      gltf_ptr->nodes_ptr = calloc( sizeof( gltf_node_t ), gltf_ptr->n_nodes );
      if ( !gltf_ptr->nodes_ptr ) {
        fprintf( stderr, "ERROR: gltf. OOM nodes_ptr\n" );
        cJSON_Delete( json_ptr );
        free( record.data );
        return false;
      }
      for ( int n = 0; n < gltf_ptr->n_nodes; n++ ) {
        gltf_ptr->nodes_ptr[n].mesh_idx = -1;
        cJSON* node_ptr                 = cJSON_GetArrayItem( nodes_ptr, n );
        cJSON* mesh_ptr                 = cJSON_GetObjectItem( node_ptr, "mesh" );
        if ( mesh_ptr ) { gltf_ptr->nodes_ptr[n].mesh_idx = mesh_ptr->valueint; }
        cJSON* name_ptr = cJSON_GetObjectItem( node_ptr, "name" );
        if ( name_ptr ) { strncat( gltf_ptr->nodes_ptr[n].name_str, name_ptr->valuestring, 255 ); }
      }
    }
  }

  // "meshes"
  cJSON* meshes_ptr = cJSON_GetObjectItem( json_ptr, "meshes" );
  if ( meshes_ptr && cJSON_IsArray( meshes_ptr ) ) {
    gltf_ptr->n_meshes = cJSON_GetArraySize( meshes_ptr );
    if ( gltf_ptr->n_meshes > 0 ) {
      gltf_ptr->meshes_ptr = calloc( sizeof( gltf_mesh_t ), gltf_ptr->n_meshes );
      if ( !gltf_ptr->meshes_ptr ) {
        fprintf( stderr, "ERROR: gltf. OOM meshes_ptr\n" );
        cJSON_Delete( json_ptr );
        free( record.data );
        return false;
      }
      for ( int m = 0; m < gltf_ptr->n_meshes; m++ ) {
        cJSON* mesh_ptr = cJSON_GetArrayItem( meshes_ptr, m );
        cJSON* name_ptr = cJSON_GetObjectItem( mesh_ptr, "name" );
        if ( name_ptr ) { strncat( gltf_ptr->meshes_ptr[m].name_str, name_ptr->valuestring, 255 ); }
        cJSON* primitives_ptr = cJSON_GetObjectItem( mesh_ptr, "primitives" );
        if ( primitives_ptr && cJSON_IsArray( primitives_ptr ) ) {
          gltf_ptr->meshes_ptr[m].n_primitives   = cJSON_GetArraySize( primitives_ptr );
          gltf_ptr->meshes_ptr[m].primitives_ptr = calloc( sizeof( gltf_primitive_t ), gltf_ptr->meshes_ptr[m].n_primitives );
          if ( !gltf_ptr->meshes_ptr[m].primitives_ptr ) {
            fprintf( stderr, "ERROR: gltf. OOM primitives_ptr\n" );
            cJSON_Delete( json_ptr );
            free( record.data );
            return false;
          }
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
        }   // endif primitives_ptr
      }     // endfor n_meshes
    }       // endif n_meshes > 0
  }         // endif meshes_ptr

  // "accessors"
  cJSON* accessors_ptr = cJSON_GetObjectItem( json_ptr, "accessors" );
  if ( accessors_ptr && cJSON_IsArray( accessors_ptr ) ) {
    gltf_ptr->n_accessors   = cJSON_GetArraySize( accessors_ptr );
    gltf_ptr->accessors_ptr = calloc( sizeof( gltf_accessor_t ), gltf_ptr->n_accessors );
    if ( !gltf_ptr->accessors_ptr ) {
      fprintf( stderr, "ERROR: gltf. OOM accessors_ptr\n" );
      cJSON_Delete( json_ptr );
      free( record.data );
      return false;
    }
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
      if ( name_ptr ) { strncat( gltf_ptr->accessors_ptr[a].name_str, name_ptr->valuestring, 255 ); }
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
  }   // endif accessors_ptr

  cJSON_Delete( json_ptr );
  free( record.data );
  return true;
}

bool gltf_free( gltf_t* gltf_ptr ) {
  if ( !gltf_ptr ) { return false; }
  if ( gltf_ptr->scenes_ptr ) {
    for ( int s = 0; s < gltf_ptr->n_scenes; s++ ) {
      if ( gltf_ptr->scenes_ptr[s].node_idxs_ptr ) { free( gltf_ptr->scenes_ptr[s].node_idxs_ptr ); }
    }
    free( gltf_ptr->scenes_ptr );
  }
  if ( gltf_ptr->nodes_ptr ) { free( gltf_ptr->nodes_ptr ); }
  if ( gltf_ptr->meshes_ptr ) {
    for ( int m = 0; m < gltf_ptr->n_meshes; m++ ) {
      if ( gltf_ptr->meshes_ptr[m].primitives_ptr ) { free( gltf_ptr->meshes_ptr[m].primitives_ptr ); }
    }
    free( gltf_ptr->meshes_ptr );
  }
  if ( gltf_ptr->accessors_ptr ) { free( gltf_ptr->accessors_ptr ); }
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
}
