#pragma once

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

bool start_gl();
void stop_gl();

mesh_t create_mesh_from_mem( const float* points_buffer, int n_points_comps, const float* colours_buffer, int n_colours_comps, int n_vertices );
void delete_mesh( mesh_t* mesh );

texture_t create_texture_from_mem( const uint8_t* img_buffer, int w, int h, int n_channels, bool srgb, bool is_depth );
void delete_texture( texture_t* texture );

void draw_mesh( uint32_t vao, size_t n_vertices );
void draw_textured_quad( texture_t texture );

void wireframe_mode();
void polygon_mode();

bool should_window_close();
void clear_colour_and_depth_buffers();
void swap_buffer_and_poll_events();
double get_time_s();