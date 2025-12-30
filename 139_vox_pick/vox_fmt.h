#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef VOX_FMT_MAIN
#define APG_IMPLEMENTATION
#define APG_NO_BACKTRACES
#endif
#include "apg.h"

typedef struct vox_info_t {
  uint32_t* n_models;
  uint32_t* dims_xyz_ptr;
  uint32_t* n_voxels;
  uint8_t* voxels_ptr; // n_voxels * 4 bytes (x,y,z,colour_index).
  uint8_t* rgba_ptr;   // Palette. 256 * 4 bytes (r,g,b,a).
} vox_info_t;

bool vox_fmt_read_file( const char* filename, apg_file_t* r_ptr, vox_info_t* info_ptr );
