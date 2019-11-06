#pragma once

#include "apg_maths.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct mesh_t {
  uint32_t vao;
  uint32_t points_vbo, colours_vbo;
  size_t n_vertices;
} mesh_t;

typedef struct texture_t {
  uint32_t handle_gl;
  int w, h, n_channels;
  bool srgb, is_depth;
} texture_t;

typedef struct shader_t {
  uint32_t program;
	// mat4
  int u_P, u_V, u_M;
	// vec2
  int u_scale, u_pos;
	// float
	int u_alpha;
} shader_t;

typedef struct camera_t {
  mat4 P, V;
  vec3 pos_world;
} camera_t;

bool start_gl();
void stop_gl();

mesh_t create_mesh_from_mem( const float* points_buffer, int n_points_comps, const float* colours_buffer, int n_colours_comps, int n_vertices );
void delete_mesh( mesh_t* mesh );

texture_t create_texture_from_mem( const uint8_t* img_buffer, int w, int h, int n_channels, bool srgb, bool is_depth );
void delete_texture( texture_t* texture );

void draw_mesh( camera_t cam, mat4 M, uint32_t vao, size_t n_vertices, bool blend, float alpha );
void draw_textured_quad( texture_t texture, vec2 scale, vec2 pos );

void wireframe_mode();
void polygon_mode();

bool should_window_close();
void clear_colour_and_depth_buffers();
void clear_depth_buffer();
void swap_buffer_and_poll_events();
double get_time_s();
void mouse_pos_win( double* x_pos, double* y_pos );
void framebuffer_dims( int* width, int* height );
void window_dims( int* width, int* height );