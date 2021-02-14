#pragma once

#include <stdbool.h>
#include <stdint.h>

// VBO used as params to instanced rendering
typedef struct gfx_buffer_t {
  uint32_t vbo_gl;
  uint32_t n_components;
  uint32_t n_elements;
  bool dynamic;
} gfx_buffer_t;

typedef struct gfx_mesh_t {
  uint32_t vao;
  uint32_t points_vbo, texcoords_vbo, normals_vbo, array_idx_vbo;
  size_t n_vertices;
  bool dynamic;
} gfx_mesh_t;

typedef struct gfx_texture_properties_t {
  bool is_srgb, is_bgr, is_depth, is_array, has_mips, repeats, bilinear;
} gfx_texture_properties_t;

typedef struct gfx_texture_t {
  uint32_t handle_gl;
  int w, h, n_channels;
  gfx_texture_properties_t properties;
} gfx_texture_t;

#define GFX_SHADER_PATH_MAX 1024
typedef struct gfx_shader_t {
  uint32_t program_gl;
  // mat4
  int u_P, u_V, u_M;
  // vec3
  int u_cam_pos;
  int u_fwd;
  int u_rgb;
  int u_output_tint_rgb;
  int u_underwater_wobble_fac;
  // vec2
  int u_scale, u_pos, u_offset, u_texcoord_scale;
  // float
  int u_alpha;
  int u_chunk_id;
  int u_fog_factor;
  int u_scroll;
  int u_time;
  int u_tint;
  int u_walk_time;
  int u_cam_y_rot;
  int u_progress_factor;
  int u_upper_clip_bound;
  int u_lower_clip_bound;
  // int
  int u_texture_a; // default to value 0
  int u_texture_b; // default to value 1
  int u_texture_c; // default to value 2

  char vs_filename[GFX_SHADER_PATH_MAX], fs_filename[GFX_SHADER_PATH_MAX];
  bool loaded;
} gfx_shader_t;

bool gfx_start( const char* window_title, const char* window_icon_filename, bool fullscreen );
void gfx_stop();


/****************************************************************************
 * Shaders
 ****************************************************************************/

gfx_shader_t gfx_create_shader_program_from_strings( const char* vert_shader_str, const char* frag_shader_str );

gfx_shader_t gfx_create_shader_program_from_files( const char* vert_shader_filename, const char* frag_shader_filename );

void gfx_delete_shader_program( gfx_shader_t* shader_ptr );

/** Registers the shader pointed to by shader_ptr to allow hot reloading.
 * @param shader_ptr An existing shader. Should have a vertex and fragment shader filename set. Must not be NULL.
 * @warning Note that the shader pointed to may be reused when reloading, so any modifications by your application to the shader pointed to by shader_ptr will
 * apply on reload. Do not delete the shader whilst it may still be reloaded!
 */
void gfx_shader_register( gfx_shader_t* shader_ptr );

/** Hot-reloads shaders given to `gfx_shader_register()` from source files. Any changes made to files will be reflected on reload, including errors. */
void gfx_shaders_reload();

int gfx_uniform_loc( const gfx_shader_t* shader_ptr, const char* name );

void gfx_uniform1i( const gfx_shader_t* shader_ptr, int loc, int i );

void gfx_uniform1f( const gfx_shader_t* shader_ptr, int loc, float f );

void gfx_uniform2f( const gfx_shader_t* shader_ptr, int loc, float x, float y );

void gfx_uniform3f( const gfx_shader_t* shader_ptr, int loc, float x, float y, float z );

void gfx_uniform4f( const gfx_shader_t* shader_ptr, int loc, float x, float y, float z, float w );
