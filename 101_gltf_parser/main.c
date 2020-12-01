#include "cJSON/cJSON.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

typedef struct gltf_node_t {
  char name_str[256];
  int mesh_idx;
} gltf_node_t;

typedef struct gltf_scene_t {
  int n_node_idxs;
  int* node_idxs_ptr; // just a list of indexes into master nodes list
} gltf_scene_t;

typedef struct gltf_t {
  char version_str[16]; // "version"

  int n_scenes;             // NB: scenes are optional
  gltf_scene_t* scenes_ptr; // "scenes"
  int default_scene_idx;    // "scene"

  int n_nodes;            // NB: nodes are optional
  gltf_node_t* nodes_ptr; // "nodes"

} gltf_t;

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
  memset( gltf_ptr, 0, sizeof( gltf_t ) );
  return true;
}

void gltf_print( const gltf_t* gltf_ptr ) {
  printf( "version \t= %s\n", gltf_ptr->version_str );
  printf( "n_scenes \t= %i\n", gltf_ptr->n_scenes );
  printf( "n_nodes \t= %i\n", gltf_ptr->n_nodes );
  printf( "scene \t\t= %i\n", gltf_ptr->default_scene_idx );
  for ( int s = 0; s < gltf_ptr->n_scenes; s++ ) {
    printf( "scene %i:\n", s );
    for ( int n = 0; n < gltf_ptr->scenes_ptr[s].n_node_idxs; n++ ) { printf( "  node %i \t= %i\n", n, gltf_ptr->scenes_ptr[s].node_idxs_ptr[n] ); }
  }
  for ( int n = 0; n < gltf_ptr->n_nodes; n++ ) {
    printf( "node %i:\n", n );
    if ( gltf_ptr->nodes_ptr[n].mesh_idx > -1 ) { printf( "  mesh \t= %i\n", gltf_ptr->nodes_ptr[n].mesh_idx ); }
    if ( gltf_ptr->nodes_ptr[n].name_str[0] != '\0' ) { printf( "  name \t= `%s`\n", gltf_ptr->nodes_ptr[n].name_str ); }
  }
}

int main( int argc, char** argv ) {
  if ( argc < 2 ) {
    printf( "Usage ./a.out MYFILE.gltf\n" );
    return 0;
  }
  printf( "Reading `%s`\n", argv[1] );
  gltf_t gltf = { 0 };
  if ( !gltf_read( argv[1], &gltf ) ) {
    fprintf( stderr, "ERROR: reading gltf `%s`\n", argv[1] );
    return 1;
  }
  gltf_print( &gltf );
  gltf_free( &gltf );

  printf( "done\n" );
  return 0;
}
