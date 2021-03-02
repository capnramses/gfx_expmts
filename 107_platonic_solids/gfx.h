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

#include "apg_maths.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum gfx_cursor_mode_t { GFX_CURSOR_MODE_SYSTEM = 0, GFX_CURSOR_MODE_HIDDEN = 1, GFX_CURSOR_MODE_TRAP_AND_HIDE = 2 } gfx_cursor_mode_t;
typedef enum gfx_primitive_type_t { GFX_PT_TRIANGLES = 0, GFX_PT_TRIANGLE_STRIP, GFX_PT_POINTS } gfx_primitive_type_t;

// VBO used as params to instanced rendering
typedef struct gfx_buffer_t {
  uint32_t vbo_gl;
  uint32_t n_components;
  uint32_t n_elements;
  bool dynamic;
} gfx_buffer_t;

typedef struct gfx_pbo_pair_t {
  uint32_t pbo_pair_gl[2];
  int current_idx;
} gfx_pbo_pair_t;

typedef struct gfx_mesh_t {
  uint32_t vao;
  uint32_t points_vbo, palidx_vbo, picking_vbo, texcoords_vbo, normals_vbo, vcolours_vbo, vedges_vbo, vlight_vbo;
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
  int u_water_depth_tint_fac;
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

bool gfx_start( const char* window_title, const char* window_icon_filename, bool fullscreen );
void gfx_stop();
const char* gfx_renderer_str( void );

/****************************************************************************
 * Meshes
 ****************************************************************************/

/** @note Requires apg_ply. */
gfx_mesh_t gfx_mesh_create_from_ply( const char* ply_filename );

gfx_mesh_t gfx_mesh_create_empty(
  bool has_pts, bool has_pal_idx, bool has_picking, bool has_texcoords, bool has_normals, bool has_vcolours, bool has_edges, bool has_light, bool dynamic );

typedef struct gfx_mesh_params_t {
  float* points_buffer;
  float* normals_buffer;
  float* texcoords_buffer;
  float* vcolours_buffer;
  float* picking_buffer;
  float* edges_buffer;
  float* light_buffer;
  uint32_t* pal_idx_buffer;

  int n_points_comps;
  int n_normal_comps; //
  int n_texcoord_comps;
  int n_vcolour_comps; //
  int n_picking_comps;
  int n_edge_comps;  //
  int n_light_comps; //
  int n_pal_idx_comps;

  int n_vertices;
  bool dynamic;
} gfx_mesh_params_t;

gfx_mesh_t gfx_create_mesh_from_mem( const gfx_mesh_params_t* mesh_params_ptr );

void gfx_update_mesh_from_mem( gfx_mesh_t* mesh, const gfx_mesh_params_t* mesh_params_ptr );

/** @param data can be NULL */
gfx_buffer_t gfx_buffer_create( float* data, uint32_t n_components, uint32_t n_elements, bool dynamic );

/** @param buffer can not be NULL. */
void gfx_buffer_delete( gfx_buffer_t* buffer );

/** @param buffer can not be NULL.
 * @param  data can be NULL. */
bool gfx_buffer_update( gfx_buffer_t* buffer, float* data, uint32_t n_components, uint32_t n_elements );

void gfx_delete_mesh( gfx_mesh_t* mesh );

/****************************************************************************
 * Textures
 ****************************************************************************/

gfx_texture_t gfx_texture_create_from_file( const char* filename, gfx_texture_properties_t properties );

gfx_texture_t gfx_texture_array_create_from_files( const char** filenames, int n_files, int w, int h, int n, gfx_texture_properties_t properties );

gfx_texture_t gfx_create_texture_from_mem( const uint8_t* img_buffer, int w, int h, int n_channels, gfx_texture_properties_t properties );

/**
 * @param six_filenames Order starts at pos X, neg X, pos Y, neg Y, pos Z, neg Z as per GL enums.
 * @param w,h           Dimensions of images in the box.
 * @returns             A texture that can be used with gfx_skybox_draw().
 * @warning             Textures may need a flip to line up correctly - not tested!
 * @note                Texture coordinates are based on vertex positions of cube, so textures will appear mirrored.
 */
gfx_texture_t gfx_texture_create_skybox( const char** six_filenames, uint32_t w, uint32_t h );

void gfx_skybox_draw( const gfx_texture_t* texture_ptr, mat4 P, mat4 V );

void gfx_delete_texture( gfx_texture_t* texture );

// PARAMS - if properties haven't changed you can use texture->is_depth etc.
void gfx_update_texture( gfx_texture_t* texture, const uint8_t* img_buffer, int w, int h, int n_channels );

void gfx_update_texture_sub_image( gfx_texture_t* texture, const uint8_t* img_buffer, int x_offs, int y_offs, int w, int h );

/** Text-to-texture helper.
 * @param texture_ptr Creates new texture if gl_handle is 0, otherwise updates existing texture. Must not be NULL.
 * @return            False on error.
 */
bool gfx_string_to_pixel_font_texture( const char* ascii_str, int thickness_px, bool add_outline, uint8_t r, uint8_t g, uint8_t b, uint8_t a, gfx_texture_t* texture_ptr );

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

/****************************************************************************
 * Framebuffers
 ****************************************************************************/

gfx_framebuffer_t gfx_create_framebuffer( int w, int h );
void gfx_bind_framebuffer( const gfx_framebuffer_t* fb );
// on success fb->built is set to true. on failure it is set to false
void gfx_rebuild_framebuffer( gfx_framebuffer_t* fb, int w, int h );

/****************************************************************************
 * Drawing
 ****************************************************************************/

/**
 * @param textures_ptr - Array of textures to bind or NULL for none. Array is in order - active texture unit 0...onwards.
 * @todo textures ptr maybe could be pointers to pointers so we don't need to copy ~140 bytes to form array items but just 64 (register sized).
 */
void gfx_draw_mesh( gfx_primitive_type_t pt, const gfx_shader_t* shader_ptr, mat4 P, mat4 V, mat4 M, uint32_t vao, size_t n_vertices,
  gfx_texture_t* textures_ptr, int n_textures );

void gfx_draw_mesh_instanced( const gfx_shader_t* shader_ptr, mat4 P, mat4 V, mat4 M, uint32_t vao, size_t n_vertices, const gfx_texture_t* textures_ptr,
  int n_textures, int n_instances, const gfx_buffer_t* instanced_buffer, int n_buffers );

void gfx_draw_textured_quad( gfx_texture_t texture, vec2 scale, vec2 pos, vec2 texcoord_scale, vec4 tint_rgba );

void gfx_skysphere_draw( float time_fac, mat4 P, mat4 V, const gfx_texture_t* time_texture_ptr );

/****************************************************************************
 * Drawing States
 ****************************************************************************/

void gfx_wireframe_mode();
void gfx_polygon_mode();
void gfx_depth_writing( bool enable );
void gfx_depth_testing( bool enable );
void gfx_alpha_testing( bool enable );
void gfx_clip_planes_enable( bool enable );
void gfx_backface_culling( bool enable );
void gfx_clip_planes( bool enable );

/****************************************************************************
 * Window
 ****************************************************************************/

bool gfx_should_window_close();

void gfx_window_dims( int* width, int* height );

void gfx_fullscreen( bool fullscreen );
bool gfx_is_fullscreen();

void gfx_vsync( int interval );
int gfx_get_vsync();

void gfx_poll_events();

// 0 normal, 1 hidden, 2 disabled
void gfx_cursor_mode( gfx_cursor_mode_t mode );

gfx_cursor_mode_t gfx_get_cursor_mode();

/** @return UTF-8 clipboard string or NULL on error. */
const char* gfx_get_clipboard_string();

/****************************************************************************
 * Viewport, colour & depth buffers
 ****************************************************************************/

void gfx_framebuffer_dims( int* width, int* height );

void gfx_viewport( int x, int y, int w, int h );

void gfx_clear_colour_and_depth_buffers( float r, float g, float b, float a );

void gfx_clear_colour_buffer();

void gfx_clear_depth_buffer();

void gfx_swap_buffer();

gfx_pbo_pair_t gfx_pbo_pair_create( size_t data_sz );
void gfx_pbo_pair_delete( gfx_pbo_pair_t* pbo_pair_ptr );

/**
 * @param pbo_pair_ptr If NULL then read without a pixel buffer.
 */
void gfx_read_pixels_bgra( int x, int y, int w, int h, int n_channels, gfx_pbo_pair_t* pbo_pair_ptr, uint8_t* data_ptr );

void gfx_screenshot();

/****************************************************************************
 * Misc. Utils.
 ****************************************************************************/
double gfx_get_time_s();

/** Call once per frame. Resets stats on call. */
gfx_stats_t gfx_frame_stats();

/****************************************************************************
 * Externally-declared global variables
 ****************************************************************************/

extern gfx_shader_t gfx_dice_shader;        /** A default shader with vertex points and colours, but no texture. Points are scaled by 0.1. */
extern gfx_shader_t gfx_quad_texture_shader;     /** A default shader for gfx_ss_quad_mesh. Uses u_scale, u_pos, u_texcoord_scale, 1 texture. */
extern gfx_shader_t gfx_default_textured_shader; /** A default shader, as gfx_quad_texture_shader but using u_P, u_V, u_M instead of scale, pos etc. */
extern gfx_texture_t gfx_checkerboard_texture;   /** A fall-back checkerboard texture */
extern gfx_texture_t gfx_white_texture;          /** A plain white texture */
extern gfx_mesh_t gfx_ss_quad_mesh;              /** A 2D triangle strip "screen space" quad that will cover the clip space coords x,y -1:1. */
extern gfx_mesh_t gfx_skybox_cube_mesh;
extern gfx_mesh_t gfx_sphere_mesh;
