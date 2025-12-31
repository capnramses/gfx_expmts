#pragma once

#include <stdbool.h>
#include "apg_maths.h"

// bool ray_aabb();

// bool ray_obb();

bool ray_voxel_grid( vec3 ro, vec3 rd, float t_entry, vec3 grid_whd, vec3 grid_min, vec3 grid_max, float* t_end_ptr, vec3* vox_n_ptr );

vec3 ray_wor_from_mouse( float mouse_x, float mouse_y, int w, int h, mat4 inv_P, mat4 inv_V );
