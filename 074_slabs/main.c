#define APG_TGA_IMPLEMENTATION
#include "../common/include/apg_tga.h"
#include <assert.h>
#include <float.h>
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

typedef struct triangle_t {
  vec3_t p0, p1, p2;
  uint8_t colour[3]; // BGR
} triangle_t;

typedef struct oob_t {
  vec3_t centre;
  vec3_t slab_directions[3];  // u,v,w
  float slab_half_lengths[3]; // u,v,w
  uint8_t colour[3];          // BGR
} oob_t;

float dot_vec3( vec3_t a, vec3_t b ) { return a.x * b.x + a.y * b.y + a.z * b.z; }

vec3_t cross_vec3( vec3_t a, vec3_t b ) {
  vec3_t c;
  c.x = a.y * b.z - a.z * b.y;
  c.y = a.z * b.x - a.x * b.z;
  c.z = a.x * b.y - a.y * b.x;
  return c;
}

vec3_t sub_vec3( vec3_t a, vec3_t b ) { return ( vec3_t ){ .x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z }; }

// NOTE(Anton) this function can be generalised to intersect with a frustum shape
bool ray_oob_test( ray_t ray, oob_t oob, float* t ) {
  assert( t );

  float t_min = -INFINITY;
  float t_max = INFINITY;

  vec3_t ray_origin_to_box_centre = sub_vec3( oob.centre, ray.origin ); // P

  // for each {u,v,w}
  // this is just 6 ray/plane intersections (3 pairs)
  for ( int i = 0; i < 3; i++ ) {
    float e = dot_vec3( oob.slab_directions[i], ray_origin_to_box_centre ); // == dot(o,d) from ray/plane where o is offset (because in ray/plane plane origin is 0)
    float f = dot_vec3( oob.slab_directions[i], ray.direction );            // ~angle between slab facing and ray
    if ( fabs( f ) > FLT_EPSILON ) {                   // eg if f is not 0 so wont div 0 (also checks if ray is not parallel to the slab planes)
      float t1 = ( e + oob.slab_half_lengths[i] ) / f; // the half length is the +d from the ray/plane function
      float t2 = ( e - oob.slab_half_lengths[i] ) / f; // /f is the /dot(d,n)
      if ( t1 > t2 ) {                                 // swap(t1,t2) so t1 is the min and t2 is the max
        float tmp = t1;
        t1        = t2;
        t2        = tmp;
      }
      if ( t1 > t_min ) { t_min = t1; }      // find maximum of the minimums
      if ( t2 < t_max ) { t_max = t2; }      // find minimum of the maximums
      if ( t_min > t_max ) { return false; } // if largest min is greater than smallest max then we missed the box
      if ( t_max < 0.0f ) { return false; }  // box is behind us NOTE(Anton) we could also check if ray origin is inside the box here
      continue;
    } else if ( -e - oob.slab_half_lengths[i] > 0.0f || -e + oob.slab_half_lengths[i] < 0.0f ) {
      return false;
    }
    // assert( false ); // does it ever get here? if so is this a bug?
  }
  *t = t_min > 0.0f ? t_min : t_max;
  return true;
}

// return true if ray intersected sphere
// note: t0 isnt necessarily the minimum of t0 and t1
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

// NOTE(Anton) can also get back u,v,w
// - does not cull back-facing triangles
// - also returns true with negative t values
bool ray_tri_test( ray_t ray, triangle_t triangle, float* t ) {
  assert( t );

  /* clang-format off

|t|                     | det(S,E1,E2) |
|u| = 1 / det(-D,E1,E2) | det(-D,S,E2) |
|v|                     | det(-D,E1,S) |

from linear algebra we know det(a,b,c) = |a b c| = -(a cross c) dot b = -(c cross b) dot a
so the first part:

    = 1 / (D cross E2) dot E1

to speed up computations

Q = D cross E2
and
R = S cross E1

  clang-format on */

  vec3_t e1 = sub_vec3( triangle.p1, triangle.p0 );
  vec3_t e2 = sub_vec3( triangle.p2, triangle.p0 );
  vec3_t q  = cross_vec3( ray.direction, e2 );                      // for speeding up determinant calcs (-a cross c)
  float det = dot_vec3( q, e1 );                                    // work out the full determinant: (-a cross c) dot b
  if ( det > -FLT_EPSILON && det < -FLT_EPSILON ) { return false; } // avoid det close to zero
  float det_inv = 1.0f / det;
  // Cramer's rule to solve by determinants
  vec3_t s = sub_vec3( ray.origin, triangle.p0 );
  float u  = det_inv * dot_vec3( s, q ); // this resolves the middle bit of the vector det(-d,s,e2) where q is the d and e2 bit
  if ( u < 0.0f ) { return false; }      // outside of triangle's barycentric coords
  vec3_t r = cross_vec3( s, e1 );
  float v  = det_inv * dot_vec3( ray.direction, r ); // this resolve the bottom bit det(-d,e1,s)
  if ( v < 0.0f || u + v > 1.0f ) { return false; }  // outside of u,v, and w < 0 (u+v) because w = (1-u-v)
  *t = det_inv * dot_vec3( e2, r );
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

  oob_t oob = ( oob_t ){ .centre = ( vec3_t ){ .x = 64, .y = 64, .z = 256 },
    .slab_directions[0]          = { .x = 0, .y = 0, .z = 1 },
    .slab_directions[1]          = { .x = 0, .y = 1, .z = 0 },
    .slab_directions[2]          = { .x = 1, .y = 0, .z = 0 },
    .slab_half_lengths[0]        = 64,
    .slab_half_lengths[1]        = 64,
    .slab_half_lengths[2]        = 64,
    .colour                      = { 0, 0, 0xFF } };

  triangle_t triangle = ( triangle_t ){ .p0 = oob.centre,
    .p1                                     = sphere.centre,
    .p2                                     = ( vec3_t ){ .x = W - 1, .y = 0, .z = 128 },
    .colour                                 = { 0xFF, 0, 0xFF } };

  for ( int y = 0; y < H; y++ ) {
    for ( int x = 0; x < W; x++ ) {
      float closest_t = INFINITY;

      ray_t ray = ( ray_t ){ .origin = ( vec3_t ){ .x = x, .y = y, .z = 0 }, .direction = ( vec3_t ){ .x = 0, .y = 0, .z = 1 } };
      float t0 = 0.0f, t1 = 0.0f;
      bool hit = ray_sphere_test( ray, sphere, &t0, &t1 );
      if ( hit ) {
        set_pixel( image, x, y, sphere.colour );
        closest_t = t0 < t1 ? t0 : t1;
      }
      float t = 0.0f;
      hit     = ray_oob_test( ray, oob, &t );
      if ( hit && t < closest_t ) {
        set_pixel( image, x, y, oob.colour );
        closest_t = t;
      }

      float tri_t = 0.0f;
      hit         = ray_tri_test( ray, triangle, &tri_t );
      if ( hit && tri_t < closest_t ) { set_pixel( image, x, y, triangle.colour ); }
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
