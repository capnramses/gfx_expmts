#pragma once

#include "apg_maths.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum block_type_t { BLOCK_TYPE_AIR = 0, BLOCK_TYPE_CRUST, BLOCK_TYPE_GRASS, BLOCK_TYPE_DIRT, BLOCK_TYPE_STONE } block_type_t;

bool chunks_create( uint32_t seed );

bool chunks_free();

void chunks_draw( vec3 cam_fwd, mat4 P, mat4 V );

void chunks_draw_colour_picking( mat4 offcentre_P, mat4 V );

/* if b channel for face is > 5 then colour was not a voxel and function will return false */
bool chunks_picked_colour_to_voxel_idx( uint8_t r, uint8_t g, uint8_t b, uint8_t a, int* x, int* y, int* z, int* face, int* chunk_id );

/*
returns true if block was changed
returns false if no change was required since type is the same as before
returns false does nothing if coords are out of chunk bounds
*/
bool chunks_set_block_type_in_chunk( int chunk_id, int x, int y, int z, block_type_t type );

bool chunks_create_block_on_face( int picked_chunk_id, int picked_x, int picked_y, int picked_z, int picked_face,  block_type_t type );

/* PERFORMANCE: current impl calls malloc() and free() */
void chunks_update_chunk_mesh( int chunk_id );
