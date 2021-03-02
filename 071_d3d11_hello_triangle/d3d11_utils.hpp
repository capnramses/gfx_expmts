#pragma once

#include <stdint.h>

struct Mesh {
  void* vertex_buffer_ptr;
  uint32_t vertex_stride;
  uint32_t vertex_offset;
  uint32_t offset;
  uint32_t n_vertices;
};

struct Shader {
  void* vertex_shader_ptr;
  void* pixel_shader_ptr;
  void* input_layout_ptr;
};

void d3d11_start( uint32_t w, uint32_t h, const wchar_t* window_title );

void d3d11_stop();

Mesh d3d11_create_mesh( const float* buffer, uint32_t buffer_sz, uint32_t vertex_stride, uint32_t vertex_offset, uint32_t n_vertices );
Shader d3d11_create_shader_program( const wchar_t* vs_filename, const wchar_t* ps_filename );

// returns false if quit was requested by eg clicking on the close window button
bool d3d11_process_events();

void d3d11_clear_framebuffer( float r, float g, float b, float a );

void d3d11_draw_mesh( Shader shader, Mesh mesh );

// swap buffers
void d3d11_present();
