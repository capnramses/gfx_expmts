
#ifndef GFX_H_
#define GFX_H_

#ifdef __cplusplus
extern "C" {
#endif /* CPP */

#include <stdbool.h>
#include <stddef.h> // size_t
#include <stdint.h>

typedef enum gfx_indices_type_t { GFX_INDICES_TYPE_UBYTE = 0, GFX_INDICES_TYPE_UINT16 = 1, GFX_INDICES_TYPE_UINT32 = 2 } gfx_indices_type_t;

typedef struct gfx_mesh_t {
  uint32_t vao;
  uint32_t points_vbo, texcoords_vbo, normals_vbo, colours_vbo, indices_vbo;
  size_t n_vertices;
  size_t n_indices;
  gfx_indices_type_t indices_type; // 0=ubyte, 1=ushort, 2=uint
  bool dynamic;
} gfx_mesh_t;

#define GFX_SHADER_PATH_MAX 1024
typedef struct gfx_shader_t {
  uint32_t program_gl;
  // mat4
  int u_P, u_V, u_M;
  // int
  int u_texture_a; // default to value 0
  int u_texture_b; // default to value 1

  int u_texture_albedo;            // default to value 0
  int u_texture_metal_roughness;   // default to value 1
  int u_texture_emissive;          // default to value 2
  int u_texture_ambient_occlusion; // default to value 3
  int u_texture_normal;            // default to value 4
  int u_texture_environment;       // default to value 5
  int u_texture_irradiance_map;    // default to value 6
  int u_texture_prefilter_map;     // default to value 7
  int u_texture_brdf_lut;          // default to value 8

  // vec3
  int u_cam_pos_wor;
  int u_light_pos_wor;
  // float
  int u_alpha;
  // PBR
  int u_base_colour_rgba;
  int u_roughness_factor;
  int u_metallic_factor;

  char vs_filename[GFX_SHADER_PATH_MAX], fs_filename[GFX_SHADER_PATH_MAX];
  bool loaded;
} gfx_shader_t;

bool gfx_start( const char* window_title, int w, int h, bool fullscreen );
void gfx_stop();

bool gfx_should_window_close();
void gfx_framebuffer_dims( int* width, int* height );
void gfx_window_dims( int* width, int* height );
void gfx_viewport( int x, int y, int w, int h );
void gfx_clear_colour_and_depth_buffers( float r, float g, float b, float a );
void gfx_swap_buffer();
void gfx_poll_events();
void gfx_backface_culling( bool enable );
void gfx_depth_mask( bool enable );
void gfx_cubemap_seamless( bool enable );

/*
PARAMS
 indices_buffer    - An optional ( can be NULL ) array of indices, one index per vertex. Type is either ubyte, ushort, or uint - specified in indices_type
 indices_buffer_sz - Size of the indices_buffer in bytes. The number of indices in the buffer will be inferred from this size / indices_type size (1,2, or 4).
 indices_type      - Data type of indices - 0=unsigned byte, 1=unsigned short, 2=unsigned int.
*/
gfx_mesh_t gfx_create_mesh_from_mem(                                                     //
  const float* points_buffer, int n_points_comps,                                        //
  const float* texcoords_buffer, int n_texcoord_comps,                                   //
  const float* normals_buffer, int n_normal_comps,                                       //
  const float* colours_buffer, int n_colours_comps,                                      //
  const void* indices_buffer, size_t indices_buffer_sz, gfx_indices_type_t indices_type, //
  int n_vertices, bool dynamic );

void gfx_update_mesh_from_mem( gfx_mesh_t* mesh,                                         //
  const float* points_buffer, int n_points_comps,                                        //
  const float* texcoords_buffer, int n_texcoord_comps,                                   //
  const float* normals_buffer, int n_normal_comps,                                       //
  const float* colours_buffer, int n_colours_comps,                                      //
  const void* indices_buffer, size_t indices_buffer_sz, gfx_indices_type_t indices_type, //
  int n_vertices, bool dynamic );

// requires apg_ply
gfx_mesh_t gfx_mesh_create_from_ply( const char* ply_filename );

void gfx_delete_mesh( gfx_mesh_t* mesh );

gfx_shader_t gfx_create_shader_program_from_files( const char* vert_shader_filename, const char* frag_shader_filename );
gfx_shader_t gfx_create_shader_program_from_strings( const char* vert_shader_str, const char* frag_shader_str );
void gfx_delete_shader_program( gfx_shader_t* shader );
bool gfx_uniform1f( gfx_shader_t shader, int uniform_location, float f );
bool gfx_uniform3f( gfx_shader_t shader, int uniform_location, float x, float y, float z );
bool gfx_uniform4f( gfx_shader_t shader, int uniform_location, float x, float y, float z, float w );

typedef struct gfx_texture_properties_t {
  bool is_srgb, is_bgr, is_depth, is_array, has_mips, repeats, bilinear, is_cube, is_hdr;
  int cube_max_mip_level;
} gfx_texture_properties_t;

typedef struct gfx_texture_t {
  uint32_t handle_gl;
  int w, h, n_channels;
  gfx_texture_properties_t properties;
} gfx_texture_t;

gfx_texture_t gfx_create_texture_from_mem( const uint8_t* img_buffer, int w, int h, int n_channels, gfx_texture_properties_t properties );

/*
PARAMS
  imgs_buffer - To match OpenGL constants order is: posx,negx,posy,negy,posz,negz.
  n_channels  - Only 4,3, and 1 supported currently.
 */
gfx_texture_t gfx_create_cube_texture_from_mem( uint8_t* imgs_buffer[6], int w, int h, int n_channels, gfx_texture_properties_t properties );

void gfx_delete_texture( gfx_texture_t* texture );

// PARAMS - if properties haven't changed you can use texture->is_depth etc.
void gfx_update_texture( gfx_texture_t* texture, const uint8_t* img_buffer, int w, int h, int n_channels );

void gfx_update_texture_sub_image( gfx_texture_t* texture, const uint8_t* img_buffer, int x_offs, int y_offs, int w, int h );

typedef enum gfx_primitive_type_t { GFX_PT_TRIANGLES = 0, GFX_PT_TRIANGLE_STRIP, GFX_PT_POINTS } gfx_primitive_type_t;
typedef enum gfx_texture_unit_t {
  GFX_TEXTURE_UNIT_ALBEDO            = 0,
  GFX_TEXTURE_UNIT_METAL_ROUGHNESS   = 1,
  GFX_TEXTURE_UNIT_EMISSIVE          = 2,
  GFX_TEXTURE_UNIT_AMBIENT_OCCLUSION = 3,
  GFX_TEXTURE_UNIT_NORMAL            = 4,
  GFX_TEXTURE_UNIT_ENVIRONMENT       = 5,
  GFX_TEXTURE_UNIT_IRRADIANCE        = 6,
  GFX_TEXTURE_UNIT_PREFILTER         = 7,
  GFX_TEXTURE_UNIT_BRDF_LUT          = 8,
  GFX_TEXTURE_UNIT_MAX
} gfx_texture_unit_t;

/* Draw a mesh, in a given primitive mode, with a shader, using virtual camera and local->world matrices, and optional array of textures */
void gfx_draw_mesh( gfx_mesh_t mesh, gfx_primitive_type_t pt, gfx_shader_t shader, float* P, float* V, float* M, gfx_texture_t* textures, int n_textures );

void gfx_wireframe_mode();
void gfx_polygon_mode();

double gfx_get_time_s();
extern gfx_shader_t gfx_default_shader;
extern gfx_mesh_t gfx_cube_mesh;

// main properties and bounds outputs of a framebuffer rendering pass. use to daisy-chain passes
typedef struct gfx_framebuffer_t {
  int w, h;                     // some multiple of gfx_viewport dims
  gfx_texture_t output_texture; //
  gfx_texture_t depth_texture;  //
  unsigned int handle_gl;       //
  bool has_cubemap;
  bool built;
} gfx_framebuffer_t;

gfx_framebuffer_t gfx_create_framebuffer( int w, int h, bool has_cubemap );

// TODO(Anton) delete fb

void gfx_bind_framebuffer( const gfx_framebuffer_t* fb );
/*
On success fb->built is set to true. On failure it is set to false.
PARAMS
  fb                    - A valid framebuffer created with a call to gfx_crate_framebuffer.
  w, h                  - Framebuffer's textures are set to these dimensions.
                          Remember to set viewport to these dimensions too.
*/
void gfx_rebuild_framebuffer( gfx_framebuffer_t* fb, int w, int h );

/*
Remember to also set viewport to approriate dims and clear the framebuffer before drawing.
IMPORTANT NOTE: Also binds the framebuffer.

params
  mip_level - Specify a MIP level or leave at 0 for default behaviour.
*/
void gfx_framebuffer_bind_cube_face( gfx_framebuffer_t fb, gfx_texture_t tex, int face_idx, int mip_level );
void gfx_framebuffer_bind_texture( gfx_framebuffer_t fb, gfx_texture_t tex );

void gfx_framebuffer_update_depth_texture_dims( gfx_framebuffer_t fb, int w, int h );

// note: unbinds framebuffer!!
bool gfx_framebuffer_status( gfx_framebuffer_t fb );

void gfx_read_pixels( int x, int y, int w, int h, int n_channels, uint8_t* data );

extern gfx_mesh_t gfx_ss_quad_mesh;

#ifdef __cplusplus
}
#endif /* CPP */

#endif /* GFX_H_ */
