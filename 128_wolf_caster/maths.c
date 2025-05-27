#include "maths.h"
#include <math.h>

vec2_t add_vec2( vec2_t a, vec2_t b ) { return (vec2_t){ .x = a.x + b.x, .y = a.y + b.y }; }

vec2_t sub_vec2( vec2_t a, vec2_t b ) { return (vec2_t){ .x = a.x - b.x, .y = a.y - b.y }; }

vec2_t mul_vec2( vec2_t a, vec2_t b ) { return (vec2_t){ .x = a.x * b.x, .y = a.y * b.y }; }

vec2_t mul_vec2_f( vec2_t v, float s ) { return (vec2_t){ .x = v.x * s, .y = v.y * s }; }

vec2_t div_vec2( vec2_t a, vec2_t b ) { return (vec2_t){ .x = a.x / b.x, .y = a.y / b.y }; }

vec2_t div_vec2_f( vec2_t v, float s ) { return (vec2_t){ .x = v.x / s, .y = v.y / s }; }

float length_vec2( vec2_t v ) { return sqrtf( v.x * v.x + v.y * v.y ); }

vec2_t normalise_vec2( vec2_t v ) { return div_vec2_f( v, length_vec2( v ) ); }

vec2_t rot_90_cw_vec2( vec2_t v ) { return (vec2_t){ v.y, -v.x }; }

vec2_t rot_90_ccw_vec2( vec2_t v ) { return (vec2_t){ -v.y, v.x }; }

float dot_vec2( vec2_t a, vec2_t b ) { return a.x * b.x + a.y * b.y; }

vec2_t project_vec2( vec2_t a, vec2_t b ) {
  /* version 1
vec2_t n    = normalise_vec2( b );
float dp    = dot_vec2( a, n );
vec2_t prod = mul_vec2_f( n, dp );
// return prod;
*/
  // version 2- from Eric Lengyel's Foundations of Game Engine Development.
  // I think b should be a unit vector.
  return mul_vec2_f( b, dot_vec2( a, b ) / dot_vec2( b, b ) );
}

vec2_t rotate_vec2( vec2_t v, float rads ) { return (vec2_t){ .x = v.x * cosf( rads ) - v.y * sinf( rads ), .y = v.x * sinf( rads ) + v.y * cosf( rads ) }; }
