#define APG_TGA_IMPLEMENTATION
#include "apg_tga.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define W 256
#define H 256

typedef struct vec3_t {
  float x, y, z;
} vec3_t;

typedef struct ray_t {
  vec3_t origin;
  vec3_t direction;
} ray_t;

typedef struct sphere_t {
  vec3_t centre;
  float radius;
  uint8_t colour[3]; // BGR
} sphere_t;

float dot_vec3( vec3_t a, vec3_t b ) { return a.x * b.x + a.y * b.y + a.z * b.z; }
vec3_t sub_vec3( vec3_t a, vec3_t b ) { return ( vec3_t ){ .x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z }; }

// return true if ray intersected sphere
bool ray_sphere_test( ray_t ray, sphere_t sphere, float* t0, float* t1 ) {
  assert( t0 && t1 );

  vec3_t omc           = sub_vec3( ray.origin, sphere.centre );
  float b              = dot_vec3( ray.direction, omc );
  float c              = dot_vec3( omc, omc ) - sphere.radius * sphere.radius;
  float bit_under_sqrt = b * b - c;
  if ( bit_under_sqrt < 0 ) { return false; }
  float sqrt_part = sqrt( bit_under_sqrt );
  *t0             = -b + sqrt_part;
  *t1             = -b - sqrt_part;
  return true;
}

void set_pixel( uint8_t* image, int x, int y, const uint8_t* colour ) {
  assert( image && x >= 0 && x < W && y >= 0 && y < H && colour );

  int idx = ( y * W + x ) * 3;
  memcpy( &image[idx], colour, sizeof( uint8_t ) * 3 );
}

int main() {
  uint8_t* image = calloc( W * H * sizeof( uint8_t ) * 3, 1 );
  assert( image );
  uint8_t background_colour[3] = { 0x77, 0x77, 0x77 };

  for ( int y = 0; y < H; y++ ) {
    for ( int x = 0; x < W; x++ ) {
      set_pixel( image, x, y, background_colour ); //
    }
  }

  sphere_t sphere = ( sphere_t ){ .centre = ( vec3_t ){ .x = 128, .y = 128, .z = 128 }, .radius = 64, .colour = { 0xFF, 0, 0 } };

  for ( int y = 0; y < H; y++ ) {
    for ( int x = 0; x < W; x++ ) {
      ray_t ray = ( ray_t ){ .origin = ( vec3_t ){ .x = x, .y = y, .z = 0 }, .direction = ( vec3_t ){ .x = 0, .y = 0, .z = 1 } };
      float t0 = 0.0f, t1 = 0.0f;
      bool hit = ray_sphere_test( ray, sphere, &t0, &t1 );
      if ( hit ) { set_pixel( image, x, y, sphere.colour ); }
    }
  }

  if ( !apg_tga_write_file( "out.tga", image, W, H, 3 ) ) {
    fprintf( stderr, "ERROR writing output file\n" );
    return 1;
  }

  free( image );
  printf( "Program halt.\n" );
  return 0;
}
