// WAD Rend - Copyright 2017 Anton Gerdelan <antonofnote@gmail.com>
// C99
#pragma once
#include "gl_utils.h"
#include "linmath.h"

void init_sky( const char* map_name );

void draw_sky(mat4 view_mat, mat4 proj_mat);
