#pragma once
#include <stdbool.h>

#define GLTF_URI_MAX 1024
#define GLTF_NAME_MAX 256

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

// This struct represents an element of the top-level "bufferViews" array.
typedef struct gltf_buffer_view_t {
  int buffer_idx;
  int byte_offset;
  int byte_length;
  int byte_stride; // NB: Usually this is just the size of eg the vec4 data type.
  char name_str[256];
} gltf_buffer_view_t;

// This struct represents an element of the top-level "buffers" array.
typedef struct gltf_buffer_t {
  char uri_str[1024]; // e.g. "FlightHelmet.bin"
  int byte_length;    // e.g. 3227148
} gltf_buffer_t;

// This struct represents the "pbrMetallicRoughness" {} object within a material.
typedef struct gltf_pbr_metallic_roughness_t {
  int base_colour_texture_idx;
  int metallic_roughness_texture_idx;
} gltf_pbr_metallic_roughness_t;

// This struct represents an element of the top-level "materials" array.
// Lighting models, texture indices, any constants, and eg doubleSided flags.
typedef struct gltf_material_t {
  gltf_pbr_metallic_roughness_t pbr_metallic_roughness;
  int normal_texture_idx;
  int occlusion_texture_idx;
  int emissive_texture_idx;
  float emissive_factor[3];
  bool has_emissive_factor;
  bool is_doubled_sided;
  bool alpha_blend;
  char name_str[256];
} gltf_material_t;

// This struct represents an element of the top-level "images" array.
// URI to a JPG or PNG or a base64 encoded embedded image.
typedef struct gltf_image_t {
  char uri_str[GLTF_URI_MAX]; // e.g. "Default_albedo.jpg"
} gltf_image_t;

// This struct represents an element of the top-level "textures" array.
// Just match image array element with sampler array element.
typedef struct gltf_texture_t {
  int sampler_idx;
  int source_idx; // image array index
  char name_str[256];
} gltf_texture_t;

// This struct represents an element of the top-level "samplers" array.
// Ignored for now.
typedef struct gltf_sampler_t {
  int mag_filter; // GL enum number
  int min_filter; // GL enum number
  bool has_mag_filter;
  bool has_min_filter;
} gltf_sampler_t;

// This is the master struct matching the top-level {} in a .gltf file.
typedef struct gltf_t {
  // major arrays at top level
  gltf_accessor_t* accessors_ptr;       // "accessors"
  gltf_buffer_t* buffers_ptr;           // "buffers"
  gltf_buffer_view_t* buffer_views_ptr; // "bufferViews"
  gltf_image_t* images_ptr;             // "images"
  gltf_material_t* materials_ptr;       // "materials"
  gltf_mesh_t* meshes_ptr;              // "meshes"
  gltf_node_t* nodes_ptr;               // "nodes"
  gltf_sampler_t* samplers_ptr;         // "samplers"
  gltf_scene_t* scenes_ptr;             // "scenes"
  gltf_texture_t* textures_ptr;         // "textures"

  // counts of major array elements
  int n_scenes; // NB: scenes are optional
  int n_nodes;  // NB: nodes are optional
  int n_meshes;
  int n_accessors;
  int n_buffer_views;
  int n_buffers;
  int n_materials;
  int n_textures;
  int n_samplers;
  int n_images;

  int default_scene_idx; // "scene"
  char version_str[16];  // "version"
} gltf_t;

bool gltf_read( const char* filename, gltf_t* gltf_ptr );
bool gltf_free( gltf_t* gltf_ptr );
void gltf_print( const gltf_t* gltf_ptr );
