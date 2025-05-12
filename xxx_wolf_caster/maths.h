#pragma once
#include <math.h>

#define APG_MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define APG_MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define APG_CLAMP( x, lo, hi ) ( APG_MIN( hi, APG_MAX( lo, x ) ) )
#define PI 3.14159265358979323846f

typedef struct vec2_t {
  float x, y;
} vec2_t;

vec2_t add_vec2( vec2_t a, vec2_t b );

vec2_t sub_vec2( vec2_t a, vec2_t b );

vec2_t mul_vec2_f( vec2_t v, float s );

vec2_t div_vec2( vec2_t a, vec2_t b );

vec2_t div_vec2_f( vec2_t v, float s );

float length_vec2( vec2_t v );

vec2_t normalise_vec2( vec2_t v );

vec2_t rot_90_cw_vec2( vec2_t v );

vec2_t rot_90_ccw_vec2( vec2_t v );