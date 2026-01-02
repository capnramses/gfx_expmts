#pragma once

#include "apg_maths.h"
#include <stdbool.h>

// bool ray_aabb();

// bool ray_obb();

/**
 * @param entry_xyz
 * Start position of line segment passing through the grid.
 *
 * @param exit_xyz
 * End position of line segment passing through the grid.
 *
 * @param cell_side
 * Side dimensions of square/cubic grid cell.
 *
 * @param visit_cell_cb
 * Pointer to user function that does something when a grid cell is visited.
 * The call-back function should return false if the grid iteration should halt e.g. when searching for the first 'solid' voxel.
 * 
 * @param data_ptr
 * The address of any additional data to pass to visit_cell_cb(). May be NULL.
 */
void ray_uniform_3d_grid( vec3 entry_xyz, vec3 exit_xyz, const float cell_side, bool ( *visit_cell_cb )( int i, int j, int k, int face, void* data_ptr ), void* data_ptr );

vec3 ray_wor_from_mouse( float mouse_x, float mouse_y, int w, int h, mat4 inv_P, mat4 inv_V );

/** As per regular ray_obb except returns 2 t values.
 * Note that ray_obb is just ray_aabb but first moving and rotating sphere and ray to origin centred and axis-aligned.
 * 
 * @param t
 * As per t in ray_obb() except set to 0 when origin is inside box.
 * 
 * @param t2
 * Exit intersection. Should always be > t. Takes the value t would take in ray_obb() when origin is inside box.
 */
bool ray_obb2( obb_t box, vec3 ray_o, vec3 ray_d, float* t, int* face_num, float* t2 );
