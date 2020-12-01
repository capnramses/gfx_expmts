#pragma once
#include <stdbool.h>

// This struct represents an element of the top-level "nodes" array.
typedef struct gltf_node_t {
  char name_str[256];
  int mesh_idx;
} gltf_node_t;

// This struct represents an element of the top-level "scenes" array.
typedef struct gltf_scene_t {
  int* node_idxs_ptr; // just a list of indexes into master nodes list
  int n_node_idxs;
} gltf_scene_t;

// This struct represents the "attributes" {} object within a primitive.
typedef struct gltf_attribute_t {
  int position_idx;
  int tangent_idx;
  int normal_idx;
  int texcoord_0_idx;
} gltf_attribute_t;

// This struct represents an element of the "primitives" array within a mesh.
typedef struct gltf_primitive_t {
  gltf_attribute_t attributes;
  int indices_idx;
  int material_idx;
} gltf_primitive_t;

// This struct represents an element of the top-level "meshes" array.
typedef struct gltf_mesh_t {
  char name_str[256];
  gltf_primitive_t* primitives_ptr;
  int n_primitives;
} gltf_mesh_t;

// Legible constants to replace the magic GL codes used for buffer component types.
typedef enum gltf_component_type_t {
  GLTF_BYTE           = 5120,
  GLTF_UNSIGNED_BYTE  = 5121,
  GLTF_SHORT          = 5122,
  GLTF_UNSIGNED_SHORT = 5123, // NOTE there is no 5124 in spec
  GLTF_UNSIGNED_INT   = 5125,
  GLTF_FLOAT          = 5126
} gltf_component_type_t;

// Legible constants to replace the strings used for data types.
typedef enum gltf_type_t { GLTF_SCALAR, GLTF_VEC2, GLTF_VEC3, GLTF_VEC4, GLTF_MAT2, GLTF_MAT3, GLTF_MAT4 } gltf_type_t;

// This struct represents an element of the top-level "accessors" array.
typedef struct gltf_accessor_t {
  char name_str[256];
  gltf_component_type_t component_type;
  gltf_type_t type;
  int buffer_view_idx;
  int byte_offset;
  int count;
  float max[3], min[3];
  bool has_max, has_min;
  // NB there exists also a "sparse" type structure here
} gltf_accessor_t;

// This is the master struct matching the top-level {} in a .gltf file.
typedef struct gltf_t {
  gltf_scene_t* scenes_ptr;       // "scenes"
  gltf_node_t* nodes_ptr;         // "nodes"
  gltf_mesh_t* meshes_ptr;        // "meshes"
  gltf_accessor_t* accessors_ptr; // "accessors"

  char version_str[16]; // "version"

  // counts of stuff
  int n_scenes; // NB: scenes are optional
  int n_nodes;  // NB: nodes are optional
  int n_meshes;
  int n_accessors;

  int default_scene_idx; // "scene"
} gltf_t;

bool gltf_read( const char* filename, gltf_t* gltf_ptr );
bool gltf_free( gltf_t* gltf_ptr );
void gltf_print( const gltf_t* gltf_ptr );
