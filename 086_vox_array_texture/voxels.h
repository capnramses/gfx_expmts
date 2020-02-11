#pragma once

#include "apg_maths.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

// dimensions of chunk in voxels
#define CHUNK_X 16  // 32
#define CHUNK_Y 256 // 256
#define CHUNK_Z 16  // 32

#define VOXEL_FACE_VERTS 6
#define VOXEL_VP_COMPS 3
#define VOXEL_VT_COMPS 2
#define VOXEL_VN_COMPS 4
#define VOXEL_VPICKING_COMPS 3
#define VOXEL_VPALIDX_COMPS 1
#define VOXEL_FACE_VP_FLOATS ( VOXEL_FACE_VERTS * VOXEL_VP_COMPS )
#define VOXEL_FACE_VP_BYTES ( VOXEL_FACE_VP_FLOATS * sizeof( float ) )
#define VOXEL_FACE_VT_FLOATS ( VOXEL_FACE_VERTS * VOXEL_VT_COMPS )
#define VOXEL_FACE_VT_BYTES ( VOXEL_FACE_VT_FLOATS * sizeof( float ) )
#define VOXEL_FACE_VN_FLOATS ( VOXEL_FACE_VERTS * VOXEL_VN_COMPS )
#define VOXEL_FACE_VN_BYTES ( VOXEL_FACE_VN_FLOATS * sizeof( float ) )
#define VOXEL_FACE_VPICKING_FLOATS ( VOXEL_FACE_VERTS * VOXEL_VPICKING_COMPS )
#define VOXEL_FACE_VPICKING_BYTES ( VOXEL_FACE_VPICKING_FLOATS * sizeof( float ) )
#define VOXEL_FACE_VPALIDX_UINTS ( VOXEL_FACE_VERTS * VOXEL_VPALIDX_COMPS )
#define VOXEL_FACE_VPALIDX_BYTES ( VOXEL_FACE_VPALIDX_UINTS * sizeof( uint32_t ) )
#define VOXEL_CUBE_VP_FLOATS ( VOXEL_FACE_VP_FLOATS * 6 )
#define VOXEL_CUBE_VP_BYTES ( VOXEL_FACE_VP_BYTES * 6 )
#define VOXEL_CUBE_VT_FLOATS ( VOXEL_FACE_VT_FLOATS * 6 )
#define VOXEL_CUBE_VT_BYTES ( VOXEL_FACE_VT_BYTES * 6 )
#define VOXEL_CUBE_VN_FLOATS ( VOXEL_FACE_VN_FLOATS * 6 )
#define VOXEL_CUBE_VN_BYTES ( VOXEL_CUBE_VN_FLOATS * sizeof( float ) )
#define VOXEL_CUBE_VPICKING_FLOATS ( VOXEL_FACE_VPICKING_FLOATS * 6 )
#define VOXEL_CUBE_VPICKING_BYTES ( VOXEL_FACE_VPICKING_BYTES * 6 )
#define VOXEL_CUBE_VPALIDX_UINTS ( VOXEL_FACE_VPALIDX_UINTS * 6 )
#define VOXEL_CUBE_VPALIDX_BYTES ( VOXEL_FACE_VPALIDX_BYTES * 6 )

typedef enum block_type_t { BLOCK_TYPE_AIR = 0, BLOCK_TYPE_CRUST, BLOCK_TYPE_GRASS, BLOCK_TYPE_DIRT, BLOCK_TYPE_STONE } block_type_t;

#pragma pack( push, 1 )
typedef struct voxel_t {
  uint8_t type;
} voxel_t;

typedef struct chunk_t {
  voxel_t* voxels;
  int heightmap[CHUNK_X * CHUNK_Z];
  uint32_t n_non_air_voxels;
} chunk_t;
#pragma pack( pop )

typedef struct chunk_vertex_data_t {
  float* positions_ptr;
	float* texcoords_ptr;
  float* picking_ptr; // only needs 3 bytes
  float* normals_ptr;
  uint32_t* palette_indices_ptr;
  size_t n_vertices;
  size_t n_vp_comps;
  size_t n_vt_comps;
  size_t n_vpalidx_comps;
  size_t n_vpicking_comps;
  size_t n_vn_comps;
  size_t vp_buffer_sz;
  size_t vt_buffer_sz;
  size_t vn_buffer_sz;
  size_t vpalidx_buffer_sz;
  size_t vpicking_buffer_sz;
} chunk_vertex_data_t;

chunk_t chunk_generate( const uint8_t* heightmap, int hm_w, int hm_h, int x_offset, int z_offset );
chunk_t chunk_generate_flat();

// warning: if chunk was already loaded or generated, then call chunk_free() first!
bool chunk_load_from_file( const char* filename, chunk_t* chunk );

bool chunk_save_to_file( const char* filename, const chunk_t* chunk );

void chunk_free( chunk_t* chunk );

bool chunk_write_heightmap( const char* filename, const chunk_t* chunk );

// as chunk_gen_vertex_data but for just 1 vertical level in the chunk. y range params can default to: 0, CHUNK_Y for an entire chunk
chunk_vertex_data_t chunk_gen_vertex_data( const chunk_t* chunk, int from_y_inclusive, int to_y_exclusive );

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
