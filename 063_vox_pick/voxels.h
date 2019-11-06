#pragma once

#include "apg_maths.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define CHUNK_X 16 // 32
#define CHUNK_Y 16 // 256
#define CHUNK_Z 16 // 32
#define N_CUBE_FACE_VERTS 6
#define N_CUBE_FACE_VERT_COMPS 3
#define N_CUBE_FACE_FLOATS ( N_CUBE_FACE_VERTS * N_CUBE_FACE_VERT_COMPS )
#define N_CUBE_FACE_BYTES ( N_CUBE_FACE_FLOATS * sizeof( float ) )
#define N_FULL_CUBE_BYTES ( N_CUBE_FACE_BYTES * 6 )

typedef enum block_type_t { BLOCK_TYPE_AIR = 0, BLOCK_TYPE_CRUST, BLOCK_TYPE_GRASS, BLOCK_TYPE_DIRT, BLOCK_TYPE_STONE } block_type_t;

#pragma pack( push, 1 )
typedef struct voxel_t {
  uint8_t type;
} voxel_t;

typedef struct chunk_t {
  voxel_t* voxels;
  int heightmap[CHUNK_X * CHUNK_Z];
  int n_non_air_voxels;
} chunk_t;
#pragma pack( pop )

typedef struct chunk_vertex_data_t {
  float* positions_ptr;
  float* colours_ptr;
  size_t sz;
  size_t n_vertices;
  size_t n_vp_comps;
  size_t n_vc_comps;
} chunk_vertex_data_t;

chunk_t chunk_generate();
bool chunk_load_from_file( const char* filename, chunk_t* chunk );
bool chunk_save_to_file( const char* filename, const chunk_t* chunk );
void chunk_free( chunk_t* chunk );
bool chunk_write_heightmap( const char* filename, const chunk_t* chunk );
chunk_vertex_data_t chunk_gen_vertex_data( const chunk_t* chunk );
void chunk_free_vertex_data( chunk_vertex_data_t* chunk_vertex_data );
bool get_block_type_in_chunk( const chunk_t* chunk, int x, int y, int z, block_type_t* block_type );

/* creates vertex data for a cube made from 6 individual faces */
chunk_vertex_data_t _test_cube_from_faces();
