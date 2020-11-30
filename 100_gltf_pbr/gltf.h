#pragma once
#include "gfx.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct gltf_scene_t {
  gfx_mesh_t* meshes_ptr;
  uint32_t n_meshes;
} gltf_scene_t;

bool gltf_load( const char* filename, gltf_scene_t* gltf_scene_ptr );

bool gltf_free( gltf_scene_t* gltf_scene_ptr );
