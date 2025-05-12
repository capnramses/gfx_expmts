#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

typedef struct mesh_t {
  GLuint vao, vbo;
  int n_points;
  GLenum primitive;
} mesh_t;

typedef struct shader_t {
  GLuint program;
  GLint u_scale, u_pos;
} shader_t;

typedef struct texture_t {
  GLuint handle;
  int w, h;
} texture_t;

typedef struct rgb_t {
  uint8_t r, g, b;
} rgb_t;

GLFWwindow* gfx_start( int win_w, int win_h, const char* title );

mesh_t gfx_create_mesh_from_mem( float* points, int n_comps, int n_points, GLenum primitive );

shader_t gfx_create_shader_from_strings( const char* vert_str, const char* frag_str );

texture_t gfx_create_texture_from_mem( int w, int h, void* pixels );

void gfx_update_texture_from_mem( texture_t* texture_ptr, void* pixels );

void plot_line( int x_i, int y_i, int x_f, int y_f, uint8_t* rgb, uint8_t* img_ptr, int w, int h, int n_chans );

extern mesh_t quad_mesh;
extern shader_t textured_shader;
