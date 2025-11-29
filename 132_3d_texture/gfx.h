#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct texture_t {
  uint32_t texture;
  uint32_t w, h, d, n;
} texture_t;

typedef struct gfx_t {
  GLFWwindow* window_ptr;
  bool started;
} gfx_t;

gfx_t gfx_start( int w, int h, const char* title_str );

void gfx_stop( void );

texture_t gfx_texture_create( uint32_t w, uint32_t h, uint32_t d, const uint8_t* pixels_ptr );
