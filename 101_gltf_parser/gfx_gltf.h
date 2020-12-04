#pragma once
#include "gfx.h"
#include "gltf.h"
#include <stdbool.h>

typedef struct gfx_gltf_t {
  gltf_t gltf;
  gfx_mesh_t* meshes_ptr;
  int* mat_idx_for_mesh_idx_ptr;
  // this array corresponds to the _images_ gltf array (not textures)
  gfx_texture_t* textures_ptr;
  int n_meshes;
  int n_textures;
} gfx_gltf_t;

bool gfx_gltf_load( const char* filename, gfx_gltf_t* gfx_gltf_ptr );
bool gfx_gltf_free( gfx_gltf_t* gfx_gltf_ptr );
void gfx_gltf_draw( const gfx_gltf_t* gfx_gltf_ptr );

float gfx_gltf_biggest_xyz( const gfx_gltf_t* gfx_gltf_ptr );
