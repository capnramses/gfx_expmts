/*
"New Voxel Project"
Copyright: Anton Gerdelan <antonofnote@gmail.com>
C99
*/

/*
OpenGL Version Required: 4.1
============================================================================================================================================================
OpenGL Extensions Required
Extensions Name                | Core | Support* | Notes
============================================================================================================================================================
ARB_debug_output                 4.3    75%
ARB_texture_storage              4.2    80%        Used by glTexStorage3D(). May not have MacOS support**.
EXT_texture_compression_s3tc     None   99%        Implemented since the GeForce 1 era.
EXT_texture_filter_anisotropic   4.6    98%        Implemented since the GeForce 2 era (1999).
============================================================================================================================================================
* https://feedback.wildfiregames.com/report/opengl/
** https://feedback.wildfiregames.com/report/opengl/feature/GL_ARB_texture_storage
============================================================================================================================================================
*/
#pragma once

#include "stb/stb_image_write.h"
#include "apg_maths.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum gfx_cursor_mode_t { GFX_CURSOR_MODE_SYSTEM = 0, GFX_CURSOR_MODE_HIDDEN = 1, GFX_CURSOR_MODE_GRAB_AND_DISABLE = 2 } gfx_cursor_mode_t;

typedef struct gfx_mesh_t {
  uint32_t vao;
  uint32_t points_vbo, palidx_vbo, picking_vbo, texcoords_vbo, normals_vbo, vcolours_vbo, vedges_vbo, instanced_vbo;
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
  int u_fwd;
  int u_rgb;
  int u_output_tint_rgb;
  // vec2
  int u_scale, u_pos, u_offset;
  // float
  int u_alpha;
  int u_chunk_id;
  int u_fog_factor;
  int u_scroll;
  int u_time;
  int u_walk_time;
  int u_cam_y_rot;
  // int
  int u_texture_a; // default to value 0
  int u_texture_b; // default to value 1

  char vs_filename[GFX_SHADER_PATH_MAX], fs_filename[GFX_SHADER_PATH_MAX];
  bool loaded;
} gfx_shader_t;

// main properties and bounds outputs of a framebuffer rendering pass. use to daisy-chain passes
typedef struct gfx_framebuffer_t {
  int w, h;                     // some multiple of gfx_viewport dims
  gfx_texture_t output_texture; //
  gfx_texture_t depth_texture;  //
  unsigned int handle_gl;       //
  bool built;
} gfx_framebuffer_t;

typedef struct gfx_stats_t {
  uint32_t draw_calls;
  uint32_t drawn_verts;
} gfx_stats_t;

bool gfx_start( const char* window_title, int w, int h, bool fullscreen );
void gfx_stop();
const char* gfx_renderer_str( void );

gfx_mesh_t gfx_create_mesh_from_mem(                   //
  const float* points_buffer, int n_points_comps,      //
  const uint32_t* pal_idx_buffer, int n_pal_idx_comps, //
  const float* picking_buffer, int n_picking_comps,    //
  const float* texcoords_buffer, int n_texcoord_comps, //
  const float* normals_buffer, int n_normal_comps,     //
  const float* vcolours_buffer, int n_vcolour_comps,   //
  const float* edges_buffer, int n_edge_comps,         //
  int n_vertices, bool dynamic );

void gfx_update_mesh_from_mem( gfx_mesh_t* mesh,       //
  const float* points_buffer, int n_points_comps,      //
  const uint32_t* pal_idx_buffer, int n_pal_idx_comps, //
  const float* picking_buffer, int n_picking_comps,    //
  const float* texcoords_buffer, int n_texcoord_comps, //
  const float* normals_buffer, int n_normal_comps,     //
  const float* vcolours_buffer, int n_vcolour_comps,   //
  const float* edges_buffer, int n_edge_comps,         //
  int n_vertices, bool dynamic );

void gfx_mesh_gen_instanced_buffer( gfx_mesh_t* mesh );

/* PARAMS - divisor. Set to 0 to use as regular attribute. Set to 1 to increment 1 element per instanced draw. 2 to increment every 2 instanced draws, etc. */
void gfx_mesh_update_instanced_buffer( gfx_mesh_t* mesh, const float* buffer, int n_comps, int n_elements, int divisor );

void gfx_delete_mesh( gfx_mesh_t* mesh );

gfx_texture_t gfx_create_texture_from_mem( const uint8_t* img_buffer, int w, int h, int n_channels, gfx_texture_properties_t properties );
void gfx_delete_texture( gfx_texture_t* texture );
// PARAMS - if properties haven't changed you can use texture->is_depth etc.
void gfx_update_texture( gfx_texture_t* texture, const uint8_t* img_buffer, int w, int h, int n_channels );
void gfx_update_texture_sub_image( gfx_texture_t* texture, const uint8_t* img_buffer, int x_offs, int y_offs, int w, int h );

gfx_shader_t gfx_create_shader_program_from_strings( const char* vert_shader_str, const char* frag_shader_str );
gfx_shader_t gfx_create_shader_program_from_files( const char* vert_shader_filename, const char* frag_shader_filename );
void gfx_delete_shader_program( gfx_shader_t* shader );
void gfx_shader_register( gfx_shader_t* shader );
void gfx_shaders_reload(); // reloads registered shaders
int gfx_uniform_loc( gfx_shader_t shader, const char* name );
void gfx_uniform1i( gfx_shader_t shader, int loc, int i );
void gfx_uniform1f( gfx_shader_t shader, int loc, float f );
void gfx_uniform2f( gfx_shader_t shader, int loc, float x, float y );
void gfx_uniform3f( gfx_shader_t shader, int loc, float x, float y, float z );
void gfx_uniform4f( gfx_shader_t shader, int loc, float x, float y, float z, float w );

gfx_framebuffer_t gfx_create_framebuffer( int w, int h );
void gfx_bind_framebuffer( const gfx_framebuffer_t* fb );
// on success fb->built is set to true. on failure it is set to false
void gfx_rebuild_framebuffer( gfx_framebuffer_t* fb, int w, int h );
void gfx_read_pixels( int x, int y, int w, int h, int n_channels, uint8_t* data );

// textures - array of textures to bind or NULL for none. array is in order - active texture unit 0...onwards.
void gfx_draw_mesh( gfx_shader_t shader, mat4 P, mat4 V, mat4 M, uint32_t vao, size_t n_vertices, gfx_texture_t* textures, int n_textures );
void gfx_draw_mesh_instanced( gfx_shader_t shader, mat4 P, mat4 V, mat4 M, uint32_t vao, size_t n_vertices, int n_instances, gfx_texture_t* textures, int n_textures );
void gfx_draw_textured_quad( gfx_texture_t texture, vec2 scale, vec2 pos );

void gfx_wireframe_mode();
void gfx_polygon_mode();

bool gfx_should_window_close();
void gfx_viewport( int x, int y, int w, int h );
void gfx_clear_colour_and_depth_buffers( float r, float g, float b, float a );
void gfx_clear_colour_buffer();
void gfx_clear_depth_buffer();
// "If flag is GL_FALSE, depth buffer writing is disabled"
void gfx_depth_writing( bool enable );
void gfx_depth_testing( bool enable );
void gfx_alpha_testing( bool enable );
void gfx_backface_culling( bool enable );
void gfx_clip_planes( bool enable );
void gfx_swap_buffer();
void gfx_poll_events();
double gfx_get_time_s();
void gfx_framebuffer_dims( int* width, int* height );
void gfx_window_dims( int* width, int* height );
void gfx_screenshot();
void gfx_fullscreen( bool fullscreen );
// 0 normal, 1 hidden, 2 disabled
void gfx_cursor_mode( gfx_cursor_mode_t mode );
gfx_cursor_mode_t gfx_get_cursor_mode();

// call once per frame. resets stats on call.
gfx_stats_t gfx_frame_stats();

extern gfx_shader_t gfx_default_shader, gfx_text_shader;
