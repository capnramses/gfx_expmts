#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct texture_t {
  uint32_t texture;
  uint32_t w, h, d, n;
  uint32_t target; // GL_TEXTURE_2D.
} texture_t;

typedef struct mesh_t {
  uint32_t vao, vbo, n_points;
} mesh_t;

typedef struct shader_t {
  uint32_t program;
} shader_t;

typedef struct gfx_t {
  GLFWwindow* window_ptr;
  bool started;
} gfx_t;

gfx_t gfx_start( int w, int h, const char* title_str );

void gfx_stop( void );

texture_t gfx_texture_create( uint32_t w, uint32_t h, uint32_t d, uint32_t n, bool integer, const uint8_t* pixels_ptr );

mesh_t gfx_mesh_cube_create( void );

bool gfx_shader_create_from_file( const char* vs_path, const char* fs_path, shader_t* shader_ptr );

void gfx_draw( shader_t shader, mesh_t mesh, const texture_t** textures_ptr, int n_textures );
