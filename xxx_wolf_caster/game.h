#pragma once

#include "apg_pixfont.h"
#include "gfx.h"
#include "maths.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO consider stretching display pixels vertically to match dims of an old beige monitor.
#define FPS_W 320
#define FPS_H 200
#define FPS_VERTICAL_STRETCH 1.2f
#define FPS_VIEWPORT_W 320 * 3
#define FPS_VIEWPORT_H 200 * 3
#define FOV_DEGS 60.0f
#define FOV_HALF_DEGS ( FOV_DEGS * 0.5f )
#define DEGS_PER_RAY ( FOV_DEGS / FPS_W )
#define MINIMAP_W 512
#define MINIMAP_H 512
#define WIN_W 1200
#define WIN_H 1300
#define TILES_W 8
#define TILES_H 8
#define RAY_STEPS_MAX 16

typedef struct minimap_t {
  texture_t tex;
  uint8_t* img_ptr;
} minimap_t;

typedef struct player_t {
  vec2_t pos, dir;
  float heading;
} player_t;

typedef struct fps_view_t {
  texture_t tex;
  uint8_t* img_ptr;
} fps_view_t;

minimap_t mmap_init( int w, int h );
void mmap_free( minimap_t* map_ptr );
void mmap_update_image( minimap_t map, const uint8_t* tiles_ptr, int tiles_w, int tiles_h, player_t player );
// Helper functions - wrap pixel plotting functions but do all the conversion from game/world coords to pixel coords. 
void mmap_plot_line( vec2_t ori, vec2_t dest, rgb_t rgb );
void mmap_plot_cross( vec2_t pos, rgb_t rgb );
void mmap_draw( minimap_t map );

fps_view_t fps_view_init( int w, int h );
void fps_view_free( fps_view_t* fps_view_ptr );
void fps_view_update_image( fps_view_t fps_view, const uint8_t* tiles_ptr, int tiles_w, int tiles_h, player_t player );
void fps_view_draw( fps_view_t fps_view );

extern minimap_t mmap;
extern const uint8_t tiles_ptr[TILES_W * TILES_H];

extern rgb_t empty_tile_colour;
extern rgb_t red_tile_colour;
extern rgb_t green_tile_colour;
extern rgb_t blue_tile_colour;
extern rgb_t error_colour;
extern rgb_t grid_line_colour;
extern rgb_t player_colour;
extern rgb_t fov_colour;
extern rgb_t to_nearest_gridlines_colour;
extern rgb_t intersect_point_colour_a;
extern rgb_t intersect_point_colour_b;
extern rgb_t intersect_hit_colour;
extern rgb_t ceiling_colour;
extern rgb_t floor_colour;
extern float dir_line_length_tiles;
extern bool minimap_draw_rays;