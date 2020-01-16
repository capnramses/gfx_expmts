#pragma once
#include <stdint.h>

typedef struct dsquare_heightmap_t {
  int w,h;
  uint8_t* filtered_heightmap;
  uint8_t* unfiltered_heightmap;
} dsquare_heightmap_t;

// default_height - eg 127 for half way up/down a 256 height block
dsquare_heightmap_t dsquare_heightmap_alloc( int dims, int default_height );
void dsquare_heightmap_gen( dsquare_heightmap_t* hm, int noise_scale, int feature_size, int feature_max_height );
void dsquare_heightmap_free(dsquare_heightmap_t* hm);
