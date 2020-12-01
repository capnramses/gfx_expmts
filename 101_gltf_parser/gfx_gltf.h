#pragma once
#include "gfx.h"
#include "gltf.h"

typedef struct gfx_gltf_t {
  gltf_t gltf;
  gfx_mesh_t* meshes_ptr;
  gfx_textures_t* textures_ptr;
  int n_meshes;
  int n_textures;
} gfx_gltf_t;
