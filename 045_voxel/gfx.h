#pragma once
#include <stdbool.h>
#include <stdlib.h>

////////////////////////////////////////// context /////////////////////////////////////////////////

// returns false if can't start context
bool start_gfx();
void stop_gfx();

bool should_window_close();
void buffer_colour( float r, float g, float b, float a );
void clear_colour_and_depth_buffers();
void swap_buffer_and_poll_events();
void wireframe_mode();
void polygon_mode();

////////////////////////////////////////// geometry ////////////////////////////////////////////////

typedef struct mesh_t {
  unsigned int vao, vbo, vcount;
} mesh_t;

typedef enum draw_mode_t { STREAM_DRAW, STATIC_DRAW, DYNAMIC_DRAW } draw_mode_t;

typedef enum geom_mem_layout_t {
  MEM_POS,     // 3 floats
  MEM_POS_ST,  // 5 floats, 2 attribs
  MEM_POS_ST_R // 6 floats, 3 attribs
} geom_mem_layout_t;

// sz can be 0 and data can be NULL, and vcount can be 0
mesh_t create_mesh( const void* data, size_t sz, geom_mem_layout_t layout, unsigned int vcount, draw_mode_t mode );

mesh_t unit_cube_mesh();

// also resets memory struct memory and handles to zero
// asserts if mesh ptr is NULL
void delete_mesh( mesh_t* mesh );

////////////////////////////////////////// shaders /////////////////////////////////////////////////

typedef struct shader_t {
  unsigned int vertex_shader, fragment_shader, program;
} shader_t;

shader_t create_shader( const char* vs_str, const char* fs_str );

// also resets memory struct memory and handles to zero
// asserts if mesh ptr is NULL
void delete_shader( shader_t* shader );

int uniform_loc( shader_t shader, const char* name );

void uniform_mat4( shader_t shader, int loc, const float* m );

void uniform_1i( shader_t shader, int loc, int i );

////////////////////////////////////////// textures ////////////////////////////////////////////////

typedef struct texture_t {
  unsigned int handle;
  char filename[1024];
} texture_t;

texture_t load_image_to_texture( const char* filename );

////////////////////////////////////////// draw helpers ////////////////////////////////////////////

void draw_mesh( mesh_t mesh, shader_t shader );

void draw_mesh_textured( mesh_t mesh, shader_t shader, texture_t texture );

void draw_mesh_texturedv( mesh_t mesh, shader_t shader, texture_t* textures, int ntextures );
