#pragma once
#include <math.h>

#define APG_MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define APG_MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define APG_CLAMP( x, lo, hi ) ( APG_MIN( hi, APG_MAX( lo, x ) ) )
#define PI 3.14159265358979323846f
#define DEGS_TO_RADS ( PI / 180.0f ) // multiply degrees by this constant to get rads.
#define RADS_TO_DEGS ( 180.0f / PI )

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

float dot_vec2( vec2_t a, vec2_t b );

// Project a onto b.
// Meaning - get the part magnitude of a that goes in the same direction as b,
// and create a new vector, in the direction of b, with that magnitude.
vec2_t project_vec2( vec2_t a, vec2_t b );

vec2_t rotate_vec2( vec2_t v, float rads );
