#pragma once
#include "gfx.h"
#include "gltf.h"
#include <stdbool.h>

typedef struct gfx_gltf_t {
  gltf_t gltf;
  gfx_mesh_t* meshes_ptr;
  gfx_texture_t* textures_ptr;
  int n_meshes;
  int n_textures;
} gfx_gltf_t;

bool gfx_gltf_load( const char* filename, gfx_gltf_t* gfx_gltf );
bool gfx_gltf_free( gfx_gltf_t* gfx_gltf );
void gfx_gltf_draw( const gfx_gltf_t* gfx_gltf );
