/* TODO

* slice view mode
* frustum culling
* world save/load
  save format
  -chunk dims
  -n chunks w,h
  -m voxels in chunk
  -m voxel types
  -m voxels in chunk...
  load
  -deserialise into structs
  -similar to create() function otherwise
  -still need to regen model matrices, textures, etc.
*/

#pragma once

#include "apg_maths.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum block_type_t { BLOCK_TYPE_AIR = 0, BLOCK_TYPE_CRUST, BLOCK_TYPE_GRASS, BLOCK_TYPE_DIRT, BLOCK_TYPE_STONE } block_type_t;

bool chunks_create( uint32_t seed, uint32_t chunks_wide, uint32_t chunks_deep );

bool chunks_free();

/* call before chunks_draw() to depth sort the chunks. should improve performance by reducing overdraw */
void chunks_sort_draw_queue( vec3 cam_pos );

void chunks_draw( vec3 cam_fwd, mat4 P, mat4 V );

/* draw all visible chunks to a colour picking framebuffer. work out the pixel x,y under the mouse.
get its RGBA from the framebuffer at (x,y) then call chunks_picked_colour_to_voxel_idx() to get the voxel index of a particular pixel */
void chunks_draw_colour_picking( mat4 offcentre_P, mat4 V );

int chunks_get_drawn_count();

/* if b channel for face is > 5 then colour was not a voxel and function will return false */
bool chunks_picked_colour_to_voxel_idx( uint8_t r, uint8_t g, uint8_t b, uint8_t a, int* x, int* y, int* z, int* face, int* chunk_id );

/*
block_type must not be NULL and chunk_id must be valid
RETURNS false if xyz is out of bounds */
bool chunks_get_block_type_in_chunk( int chunk_id, int x, int y, int z, block_type_t* block_type );

/*
RETURNS
- true if block was changed
- false if no change was required since type is the same as before
- false and does nothing if coords are out of chunk bounds */
bool chunks_set_block_type_in_chunk( int chunk_id, int x, int y, int z, block_type_t type );

bool chunks_create_block_on_face( int picked_chunk_id, int picked_x, int picked_y, int picked_z, int picked_face, block_type_t type );

/* explicitly update one chunk. sometimes useful during world creation. normally just call chunks_update_dirty_chunk_meshes()
PERFORMANCE WARNING: current impl calls malloc() and free() */
void chunks_update_chunk_mesh( int chunk_id );

/* call once per update tick to regenerate geometry for any chunks that were modified since last call
calls chunks_update_chunk_mesh() */
void chunks_update_dirty_chunk_meshes();

void chunks_slice_view_mode( bool enable );
