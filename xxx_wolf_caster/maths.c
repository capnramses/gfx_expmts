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
