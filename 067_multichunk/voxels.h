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
#define N_FULL_CUBE_FLOATS ( N_CUBE_FACE_FLOATS * 6 )
#define N_FULL_CUBE_BYTES ( N_CUBE_FACE_BYTES * 6 )

typedef enum block_type_t { BLOCK_TYPE_AIR = 0, BLOCK_TYPE_CRUST, BLOCK_TYPE_GRASS, BLOCK_TYPE_DIRT, BLOCK_TYPE_STONE } block_type_t;

#pragma pack( push, 1 )
typedef struct voxel_t {
  uint8_t type;
} voxel_t;

typedef struct chunk_t {
  voxel_t* voxels;
  int x, z; // index of chunk. multiplies x,z values
  int heightmap[CHUNK_X * CHUNK_Z];
  uint32_t n_non_air_voxels;
} chunk_t;
#pragma pack( pop )

typedef struct chunk_vertex_data_t {
  float* positions_ptr;
  float* colours_ptr;
  float* picking_ptr; // only needs 3 bytes
  float* normals_ptr;
  size_t sz;
  size_t n_vertices;
  size_t n_vp_comps;
  size_t n_vc_comps;
  size_t n_vpicking_comps;
  size_t n_vn_comps;
  size_t vp_buffer_sz;
  size_t vn_buffer_sz;
} chunk_vertex_data_t;

chunk_t chunk_generate( int chunk_x, int chunk_z );
bool chunk_load_from_file( const char* filename, chunk_t* chunk );
bool chunk_save_to_file( const char* filename, const chunk_t* chunk );
void chunk_free( chunk_t* chunk );
bool chunk_write_heightmap( const char* filename, const chunk_t* chunk );
chunk_vertex_data_t chunk_gen_vertex_data( const chunk_t* chunk );
void chunk_free_vertex_data( chunk_vertex_data_t* chunk_vertex_data );
bool get_block_type_in_chunk( const chunk_t* chunk, int x, int y, int z, block_type_t* block_type );

// return true if x,y,z is out of bounds of chunk
// returns true if y > heightmap at (x,z). if equal returns false.
bool is_voxel_above_surface( const chunk_t* chunk, int x, int y, int z );
bool is_voxel_face_exposed_to_sun( const chunk_t* chunk, int x, int y, int z, int face_idx );
/*
returns true if block was changed
returns false if no change was required since type is the same as before
returns false does nothing if coords are out of chunk bounds
*/
bool set_block_type_in_chunk( chunk_t* chunk, int x, int y, int z, block_type_t type );
/* if b channel for face is > 5 then colour was not a voxel and function will return false */
bool picked_colour_to_voxel_idx( uint8_t r, uint8_t g, uint8_t b, uint8_t a, int* x, int* y, int* z, int* face, int* chunk_id );

/* creates vertex data for a cube made from 6 individual faces */
chunk_vertex_data_t _test_cube_from_faces();
