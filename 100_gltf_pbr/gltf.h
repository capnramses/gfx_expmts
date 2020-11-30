#pragma once
#include "gfx.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct gltf_material_t {
  char name[1024];
  // -- PBR material stuff --
  int base_colour_texture_idx;     // 0 - u_texture_albedo
  int metal_roughness_texture_idx; // 1 - u_texture_metal_roughness
  float base_colour_rgba[4];
  float metallic_f;
  float roughness_f;
  // -- basic material stuff --
  int emissive_texture_idx; // 2 - u_texture_emissive
  float emissive_rgb[3];
  int ambient_occlusion_texture_idx; // 3 - u_texture_ambient_occlusion
  int normal_texture_idx;            // 4 - u_texture_normal

  bool unlit;
} gltf_material_t;

// NOTE: this does not currently handle re-using duplicates of textures/materials.
// each mesh has an unique set.
typedef struct gltf_scene_t {
  gfx_texture_t* textures_ptr;
  uint32_t n_textures;

  gfx_mesh_t* meshes_ptr;
  uint32_t n_meshes;

  gltf_material_t material;
} gltf_scene_t;

bool gltf_load( const char* filename, gltf_scene_t* gltf_scene_ptr );

bool gltf_free( gltf_scene_t* gltf_scene_ptr );

gltf_material_t gltf_default_material();

void gltf_print_material_summary( gltf_material_t mat );
