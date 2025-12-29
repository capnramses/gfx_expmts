#pragma once

#include <stdbool.h>
#include "apg_maths.h"

//bool ray_aabb();

//bool ray_obb();

bool ray_voxel_grid();

vec3 ray_wor_from_mouse( float mouse_x, float mouse_y, int w, int h, mat4 inv_P, mat4 inv_V );
