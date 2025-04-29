/*****************************************************************************\
Anton's Maths Library - C99 version
Anton Gerdelan <antonofnote at gmail>
First v. branched from C++ original 5 May 2015

C99 standard specifies memory stored in order vars are declared, which lets us
use .x .y instead of an array and still specify &my_vec3 or &my_vec3.x for
array equiv. Compiler padding isn't an issue because all members are size float.

WARNING
most vector functions - dot, length etc are simplified to assumes `a` and `b`
input vectors are of equal length and share an orthonormal coordinate system.
for full functions with all terms see
http://immersivemath.com/ila/ch03_dotproduct/ch03.html

11 April 2016 - compacted
12 April 2016 - switched to .x .y .z notation for vectors and quaternions
20 Sept 2016 - replace memset() with = { { 0 } } and removed zero_mat4()
\*****************************************************************************/
#pragma once
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <assert.h>
#include <math.h>
#include <string.h>
#ifndef M_PI // C99 removed M_PI
#define M_PI 3.14159265358979323846
#endif
#define ONE_DEG_IN_RAD ( 2.0 * M_PI ) / 360.0 // 0.017444444
#define ONE_RAD_IN_DEG 360.0 / ( 2.0 * M_PI ) // 57.2957795
#define MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define CLAMP( x, lo, hi ) ( MIN( hi, MAX( lo, x ) ) )

static int loopmod( int val, int divisor ) {
  if ( val < 0 ) {
    val = divisor + val;
  }
  return val % divisor;
}

typedef struct vec2 { float x, y; } vec2;
typedef struct vec3 { float x, y, z; } vec3;
typedef struct vec4 { float x, y, z, w; } vec4;
typedef struct mat4 { float m[16]; } mat4;
typedef struct versor { float w, x, y, z; } versor;

static inline void print_vec2( vec2 v ) { printf( "[%.2f, %.2f]\n", v.x, v.y ); }

static inline void print_vec3( vec3 v ) {
  printf( "[%.2f, %.2f, %.2f]\n", v.x, v.y, v.z );
}

static inline void print_vec4( vec4 v ) {
  printf( "[%.2f, %.2f, %.2f, %.2f]\n", v.x, v.y, v.z, v.w );
}

static inline void print_mat4( mat4 m ) {
  printf( "\n" );
  printf( "[%.2f][%.2f][%.2f][%.2f]\n", m.m[0], m.m[4], m.m[8], m.m[12] );
  printf( "[%.2f][%.2f][%.2f][%.2f]\n", m.m[1], m.m[5], m.m[9], m.m[13] );
  printf( "[%.2f][%.2f][%.2f][%.2f]\n", m.m[2], m.m[6], m.m[10], m.m[14] );
  printf( "[%.2f][%.2f][%.2f][%.2f]\n", m.m[3], m.m[7], m.m[11], m.m[15] );
}

static inline void print_quat( versor q ) {
  printf( "[%.2f ,%.2f, %.2f, %.2f]\n", q.w, q.x, q.y, q.z );
}

static inline vec3 v3_v4( vec4 v ) {
  return ( vec3 ){.x = v.x, .y = v.y, .z = v.z };
}

static inline vec2 sub_vec2_vec2( vec2 a, vec2 b ) {
  return ( vec2 ){.x = a.x - b.x, .y = a.y - b.y };
}

static inline vec3 add_vec3_f( vec3 a, float b ) {
  return ( vec3 ){.x = a.x + b, .y = a.y + b, .z = a.z + b };
}

static inline vec3 sub_vec3_f( vec3 a, float b ) {
  return ( vec3 ){.x = a.x - b, .y = a.y - b, .z = a.z - b };
}

static inline vec3 mult_vec3_f( vec3 a, float b ) {
  return ( vec3 ){.x = a.x * b, .y = a.y * b, .z = a.z * b };
}

static inline vec3 div_vec3_f( vec3 a, float b ) {
  return ( vec3 ){.x = a.x / b, .y = a.y / b, .z = a.z / b };
}

static inline vec3 add_vec3_vec3( vec3 a, vec3 b ) {
  return ( vec3 ){.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z };
}

static inline vec3 sub_vec3_vec3( vec3 a, vec3 b ) {
  return ( vec3 ){.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z };
}

static inline vec3 mult_vec3_vec3( vec3 a, vec3 b ) {
  return ( vec3 ){.x = a.x * b.x, .y = a.y * b.y, .z = a.z * b.z };
}

static inline vec3 div_vec3_vec3( vec3 a, vec3 b ) {
  return ( vec3 ){.x = a.x / b.x, .y = a.y / b.y, .z = a.z / b.z };
}

static inline float length_vec2( vec2 v ) { return sqrt( v.x * v.x + v.y * v.y ); }

static inline float length_vec3( vec3 v ) {
  return sqrt( v.x * v.x + v.y * v.y + v.z * v.z );
}

static inline float length2_vec3( vec3 v ) {
  return v.x * v.x + v.y * v.y + v.z * v.z;
}

static inline vec2 normalise_vec2( vec2 v ) {
  vec2 vb;
  float l = length_vec2( v );
  if ( 0.0f == l ) {
    return ( vec2 ){.x = 0.0f, .y = 0.0f };
  }
  vb.x = v.x / l;
  vb.y = v.y / l;
  return vb;
}

static inline vec3 normalise_vec3( vec3 v ) {
  vec3 vb;
  float l = length_vec3( v );
  if ( 0.0f == l ) {
    return ( vec3 ){.x = 0.0f, .y = 0.0f, .z = 0.0f };
  }
  vb.x = v.x / l;
  vb.y = v.y / l;
  vb.z = v.z / l;
  return vb;
}

static inline float dot_vec2( vec2 a, vec2 b ) { return a.x * b.x + a.y * b.y; }

static inline float dot_vec3( vec3 a, vec3 b ) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec3 cross_vec3( vec3 a, vec3 b ) {
  return ( vec3 ){.x = a.y * b.z - a.z * b.y,
                  .y = a.z * b.x - a.x * b.z,
                  .z = a.x * b.y - a.y * b.x };
}

// converts an un-normalised direction vector's X,Z components into a heading
// in degrees
static inline float vec3_to_heading( vec3 d ) {
  return atan2( -d.x, -d.z ) * ONE_RAD_IN_DEG;
}

// very informal function to convert a heading (e.g. y-axis orientation) into
// a 3d vector with components in x and z axes
static inline vec3 heading_to_vec3( float degrees ) {
  float rad = degrees * ONE_DEG_IN_RAD;
  return ( vec3 ){.x = -sinf( rad ), .y = 0.0f, .z = -cosf( rad ) };
}

static inline vec4 v4_v3f( vec3 v, float f ) {
  return ( vec4 ){.x = v.x, .y = v.y, .z = v.z, .w = f };
}

static inline mat4 identity_mat4() {
  mat4 r = { { 0 } };
  r.m[0] = 1.0f;
  r.m[5] = 1.0f;
  r.m[10] = 1.0f;
  r.m[15] = 1.0f;
  return r;
}

static inline mat4 mult_mat4_mat4( mat4 a, mat4 b ) {
  mat4 r = { { 0 } };
  int r_index = 0;
  for ( int col = 0; col < 4; col++ ) {
    for ( int row = 0; row < 4; row++ ) {
      float sum = 0.0f;
      for ( int i = 0; i < 4; i++ ) {
        sum += b.m[i + col * 4] * a.m[row + i * 4];
      }
      r.m[r_index] = sum;
      r_index++;
    }
  }
  return r;
}

static inline vec4 mult_mat4_vec4( mat4 m, vec4 v ) {
  float x = m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12] * v.w;
  float y = m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13] * v.w;
  float z = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w;
  float w = m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w;
  return ( vec4 ){.x = x, .y = y, .z = z, .w = w };
}

static inline float det_mat4( mat4 mm ) {
  return mm.m[12] * mm.m[9] * mm.m[6] * mm.m[3] -
         mm.m[8] * mm.m[13] * mm.m[6] * mm.m[3] -
         mm.m[12] * mm.m[5] * mm.m[10] * mm.m[3] +
         mm.m[4] * mm.m[13] * mm.m[10] * mm.m[3] +
         mm.m[8] * mm.m[5] * mm.m[14] * mm.m[3] -
         mm.m[4] * mm.m[9] * mm.m[14] * mm.m[3] -
         mm.m[12] * mm.m[9] * mm.m[2] * mm.m[7] +
         mm.m[8] * mm.m[13] * mm.m[2] * mm.m[7] +
         mm.m[12] * mm.m[1] * mm.m[10] * mm.m[7] -
         mm.m[0] * mm.m[13] * mm.m[10] * mm.m[7] -
         mm.m[8] * mm.m[1] * mm.m[14] * mm.m[7] +
         mm.m[0] * mm.m[9] * mm.m[14] * mm.m[7] +
         mm.m[12] * mm.m[5] * mm.m[2] * mm.m[11] -
         mm.m[4] * mm.m[13] * mm.m[2] * mm.m[11] -
         mm.m[12] * mm.m[1] * mm.m[6] * mm.m[11] +
         mm.m[0] * mm.m[13] * mm.m[6] * mm.m[11] +
         mm.m[4] * mm.m[1] * mm.m[14] * mm.m[11] -
         mm.m[0] * mm.m[5] * mm.m[14] * mm.m[11] -
         mm.m[8] * mm.m[5] * mm.m[2] * mm.m[15] +
         mm.m[4] * mm.m[9] * mm.m[2] * mm.m[15] +
         mm.m[8] * mm.m[1] * mm.m[6] * mm.m[15] -
         mm.m[0] * mm.m[9] * mm.m[6] * mm.m[15] -
         mm.m[4] * mm.m[1] * mm.m[10] * mm.m[15] +
         mm.m[0] * mm.m[5] * mm.m[10] * mm.m[15];
}

// TODO look up fast inverse video tutorial
static inline mat4 inverse_mat4( mat4 mm ) {
  float det = det_mat4( mm );
  if ( 0.0f == det ) {
    return mm;
  }
  float inv_det = 1.0f / det;
  mat4 r;
  r.m[0] =
    inv_det * ( mm.m[9] * mm.m[14] * mm.m[7] - mm.m[13] * mm.m[10] * mm.m[7] +
                mm.m[13] * mm.m[6] * mm.m[11] - mm.m[5] * mm.m[14] * mm.m[11] -
                mm.m[9] * mm.m[6] * mm.m[15] + mm.m[5] * mm.m[10] * mm.m[15] );
  r.m[1] =
    inv_det * ( mm.m[13] * mm.m[10] * mm.m[3] - mm.m[9] * mm.m[14] * mm.m[3] -
                mm.m[13] * mm.m[2] * mm.m[11] + mm.m[1] * mm.m[14] * mm.m[11] +
                mm.m[9] * mm.m[2] * mm.m[15] - mm.m[1] * mm.m[10] * mm.m[15] );
  r.m[2] =
    inv_det * ( mm.m[5] * mm.m[14] * mm.m[3] - mm.m[13] * mm.m[6] * mm.m[3] +
                mm.m[13] * mm.m[2] * mm.m[7] - mm.m[1] * mm.m[14] * mm.m[7] -
                mm.m[5] * mm.m[2] * mm.m[15] + mm.m[1] * mm.m[6] * mm.m[15] );
  r.m[3] =
    inv_det * ( mm.m[9] * mm.m[6] * mm.m[3] - mm.m[5] * mm.m[10] * mm.m[3] -
                mm.m[9] * mm.m[2] * mm.m[7] + mm.m[1] * mm.m[10] * mm.m[7] +
                mm.m[5] * mm.m[2] * mm.m[11] - mm.m[1] * mm.m[6] * mm.m[11] );
  r.m[4] =
    inv_det * ( mm.m[12] * mm.m[10] * mm.m[7] - mm.m[8] * mm.m[14] * mm.m[7] -
                mm.m[12] * mm.m[6] * mm.m[11] + mm.m[4] * mm.m[14] * mm.m[11] +
                mm.m[8] * mm.m[6] * mm.m[15] - mm.m[4] * mm.m[10] * mm.m[15] );
  r.m[5] =
    inv_det * ( mm.m[8] * mm.m[14] * mm.m[3] - mm.m[12] * mm.m[10] * mm.m[3] +
                mm.m[12] * mm.m[2] * mm.m[11] - mm.m[0] * mm.m[14] * mm.m[11] -
                mm.m[8] * mm.m[2] * mm.m[15] + mm.m[0] * mm.m[10] * mm.m[15] );
  r.m[6] =
    inv_det * ( mm.m[12] * mm.m[6] * mm.m[3] - mm.m[4] * mm.m[14] * mm.m[3] -
                mm.m[12] * mm.m[2] * mm.m[7] + mm.m[0] * mm.m[14] * mm.m[7] +
                mm.m[4] * mm.m[2] * mm.m[15] - mm.m[0] * mm.m[6] * mm.m[15] );
  r.m[7] =
    inv_det * ( mm.m[4] * mm.m[10] * mm.m[3] - mm.m[8] * mm.m[6] * mm.m[3] +
                mm.m[8] * mm.m[2] * mm.m[7] - mm.m[0] * mm.m[10] * mm.m[7] -
                mm.m[4] * mm.m[2] * mm.m[11] + mm.m[0] * mm.m[6] * mm.m[11] );
  r.m[8] =
    inv_det * ( mm.m[8] * mm.m[13] * mm.m[7] - mm.m[12] * mm.m[9] * mm.m[7] +
                mm.m[12] * mm.m[5] * mm.m[11] - mm.m[4] * mm.m[13] * mm.m[11] -
                mm.m[8] * mm.m[5] * mm.m[15] + mm.m[4] * mm.m[9] * mm.m[15] );
  r.m[9] =
    inv_det * ( mm.m[12] * mm.m[9] * mm.m[3] - mm.m[8] * mm.m[13] * mm.m[3] -
                mm.m[12] * mm.m[1] * mm.m[11] + mm.m[0] * mm.m[13] * mm.m[11] +
                mm.m[8] * mm.m[1] * mm.m[15] - mm.m[0] * mm.m[9] * mm.m[15] );
  r.m[10] =
    inv_det * ( mm.m[4] * mm.m[13] * mm.m[3] - mm.m[12] * mm.m[5] * mm.m[3] +
                mm.m[12] * mm.m[1] * mm.m[7] - mm.m[0] * mm.m[13] * mm.m[7] -
                mm.m[4] * mm.m[1] * mm.m[15] + mm.m[0] * mm.m[5] * mm.m[15] );
  r.m[11] =
    inv_det * ( mm.m[8] * mm.m[5] * mm.m[3] - mm.m[4] * mm.m[9] * mm.m[3] -
                mm.m[8] * mm.m[1] * mm.m[7] + mm.m[0] * mm.m[9] * mm.m[7] +
                mm.m[4] * mm.m[1] * mm.m[11] - mm.m[0] * mm.m[5] * mm.m[11] );
  r.m[12] =
    inv_det * ( mm.m[12] * mm.m[9] * mm.m[6] - mm.m[8] * mm.m[13] * mm.m[6] -
                mm.m[12] * mm.m[5] * mm.m[10] + mm.m[4] * mm.m[13] * mm.m[10] +
                mm.m[8] * mm.m[5] * mm.m[14] - mm.m[4] * mm.m[9] * mm.m[14] );
  r.m[13] =
    inv_det * ( mm.m[8] * mm.m[13] * mm.m[2] - mm.m[12] * mm.m[9] * mm.m[2] +
                mm.m[12] * mm.m[1] * mm.m[10] - mm.m[0] * mm.m[13] * mm.m[10] -
                mm.m[8] * mm.m[1] * mm.m[14] + mm.m[0] * mm.m[9] * mm.m[14] );
  r.m[14] =
    inv_det * ( mm.m[12] * mm.m[5] * mm.m[2] - mm.m[4] * mm.m[13] * mm.m[2] -
                mm.m[12] * mm.m[1] * mm.m[6] + mm.m[0] * mm.m[13] * mm.m[6] +
                mm.m[4] * mm.m[1] * mm.m[14] - mm.m[0] * mm.m[5] * mm.m[14] );
  r.m[15] =
    inv_det * ( mm.m[4] * mm.m[9] * mm.m[2] - mm.m[8] * mm.m[5] * mm.m[2] +
                mm.m[8] * mm.m[1] * mm.m[6] - mm.m[0] * mm.m[9] * mm.m[6] -
                mm.m[4] * mm.m[1] * mm.m[10] + mm.m[0] * mm.m[5] * mm.m[10] );
  return r;
}

static inline mat4 transpose_mat4( mat4 mm ) {
  mat4 r;
  r.m[0] = mm.m[0];
  r.m[4] = mm.m[1];
  r.m[8] = mm.m[2];
  r.m[12] = mm.m[3];
  r.m[1] = mm.m[4];
  r.m[5] = mm.m[5];
  r.m[9] = mm.m[6];
  r.m[13] = mm.m[7];
  r.m[2] = mm.m[8];
  r.m[6] = mm.m[9];
  r.m[10] = mm.m[10];
  r.m[14] = mm.m[11];
  r.m[3] = mm.m[12];
  r.m[7] = mm.m[13];
  r.m[11] = mm.m[14];
  r.m[15] = mm.m[15];
  return r;
}

static inline mat4 translate_mat4( vec3 vv ) {
  mat4 r = identity_mat4();
  r.m[12] = vv.x;
  r.m[13] = vv.y;
  r.m[14] = vv.z;
  return r;
}

static inline mat4 rot_x_deg_mat4( float deg ) {
  float rad = deg * ONE_DEG_IN_RAD;
  mat4 r = identity_mat4();
  r.m[5] = r.m[10] = cos( rad );
  r.m[9] = -sin( rad );
  r.m[6] = sin( rad );
  return r;
}

static inline mat4 rot_y_deg_mat4( float deg ) {
  float rad = deg * ONE_DEG_IN_RAD;
  mat4 r = identity_mat4();
  r.m[0] = r.m[10] = cos( rad );
  r.m[8] = sin( rad );
  r.m[2] = -sin( rad );
  return r;
}

static inline mat4 rot_z_deg_mat4( float deg ) {
  float rad = deg * ONE_DEG_IN_RAD;
  mat4 r = identity_mat4();
  r.m[0] = r.m[5] = cos( rad );
  r.m[4] = -sin( rad );
  r.m[1] = sin( rad );
  return r;
}

static inline mat4 scale_mat4( vec3 v ) {
  mat4 r = identity_mat4();
  r.m[0] = v.x;
  r.m[5] = v.y;
  r.m[10] = v.z;
  return r;
}

static inline mat4 look_at( vec3 cam_pos, vec3 targ_pos, vec3 up ) {
  mat4 p =
    translate_mat4( ( vec3 ){.x = -cam_pos.x, .y = -cam_pos.y, .z = -cam_pos.z } );
  vec3 d = sub_vec3_vec3( targ_pos, cam_pos );
  vec3 f = normalise_vec3( d );
  vec3 r = normalise_vec3( cross_vec3( f, up ) );
  vec3 u = normalise_vec3( cross_vec3( r, f ) );
  mat4 ori = identity_mat4();
  ori.m[0] = r.x;
  ori.m[4] = r.y;
  ori.m[8] = r.z;
  ori.m[1] = u.x;
  ori.m[5] = u.y;
  ori.m[9] = u.z;
  ori.m[2] = -f.x;
  ori.m[6] = -f.y;
  ori.m[10] = -f.z;
  return mult_mat4_mat4( ori, p );
}

static inline mat4 perspective( float fovy, float aspect, float near, float far ) {
  float fov_rad = fovy * ONE_DEG_IN_RAD;
  float range = tan( fov_rad / 2.0f ) * near;
  float sx = ( 2.0f * near ) / ( range * aspect + range * aspect );
  float sy = near / range;
  float sz = -( far + near ) / ( far - near );
  float pz = -( 2.0f * far * near ) / ( far - near );
  mat4 m = { { 0 } };
  m.m[0] = sx;
  m.m[5] = sy;
  m.m[10] = sz;
  m.m[14] = pz;
  m.m[11] = -1.0f;
  return m;
}

static inline versor div_quat_f( versor qq, float s ) {
  return ( versor ){.w = qq.w / s, .x = qq.x / s, .y = qq.y / s, .z = qq.z / s };
}

static inline versor mult_quat_f( versor qq, float s ) {
  return ( versor ){.w = qq.w * s, .x = qq.x * s, .y = qq.y * s, .z = qq.z * s };
}

static inline versor normalise_quat( versor q ) {
  float sum = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
  const float thresh = 0.0001f;
  if ( fabs( 1.0f - sum ) < thresh ) {
    return q;
  }
  float mag = sqrt( sum );
  return div_quat_f( q, mag );
}

static inline versor mult_quat_quat( versor a, versor b ) {
  versor result;
  result.w = b.w * a.w - b.x * a.x - b.y * a.y - b.z * a.z;
  result.x = b.w * a.x + b.x * a.w - b.y * a.z + b.z * a.y;
  result.y = b.w * a.y + b.x * a.z + b.y * a.w - b.z * a.x;
  result.z = b.w * a.z - b.x * a.y + b.y * a.x + b.z * a.w;
  return normalise_quat( result );
}

static inline versor add_quat_quat( versor a, versor b ) {
  versor result;
  result.w = b.w + a.w;
  result.x = b.x + a.x;
  result.y = b.y + a.y;
  result.z = b.z + a.z;
  return normalise_quat( result );
}

static inline versor quat_from_axis_rad( float radians, vec3 axis ) {
  versor result;
  result.w = cos( radians / 2.0 );
  result.x = sin( radians / 2.0 ) * axis.x;
  result.y = sin( radians / 2.0 ) * axis.y;
  result.z = sin( radians / 2.0 ) * axis.z;
  return result;
}

static inline versor quat_from_axis_deg( float degrees, vec3 axis ) {
  return quat_from_axis_rad( ONE_DEG_IN_RAD * degrees, axis );
}

static inline mat4 quat_to_mat4( versor q ) {
  float w = q.w;
  float x = q.x;
  float y = q.y;
  float z = q.z;
  mat4 r;
  r.m[0] = 1.0f - 2.0f * y * y - 2.0f * z * z;
  r.m[1] = 2.0f * x * y + 2.0f * w * z;
  r.m[2] = 2.0f * x * z - 2.0f * w * y;
  r.m[3] = 0.0f;
  r.m[4] = 2.0f * x * y - 2.0f * w * z;
  r.m[5] = 1.0f - 2.0f * x * x - 2.0f * z * z;
  r.m[6] = 2.0f * y * z + 2.0f * w * x;
  r.m[7] = 0.0f;
  r.m[8] = 2.0f * x * z + 2.0f * w * y;
  r.m[9] = 2.0f * y * z - 2.0f * w * x;
  r.m[10] = 1.0f - 2.0f * x * x - 2.0f * y * y;
  r.m[11] = r.m[12] = r.m[13] = r.m[14] = 0.0f;
  r.m[15] = 1.0f;
  return r;
}

static inline float dot_quat( versor q, versor r ) {
  return q.w * r.w + q.x * r.x + q.y * r.y + q.z * r.z;
}

static inline versor slerp_quat( versor q, versor r, float t ) {
  float cos_half_theta = dot_quat( q, r );
  if ( cos_half_theta < 0.0f ) {
    q = mult_quat_f( q, -1.0f );
    cos_half_theta = dot_quat( q, r );
  }
  if ( fabs( cos_half_theta ) >= 1.0f ) {
    return q;
  }
  float sin_half_theta = sqrt( 1.0f - cos_half_theta * cos_half_theta );
  versor result;
  if ( fabs( sin_half_theta ) < 0.001f ) {
    result.w = ( 1.0f - t ) * q.w + t * r.w;
    result.x = ( 1.0f - t ) * q.x + t * r.x;
    result.y = ( 1.0f - t ) * q.y + t * r.y;
    result.z = ( 1.0f - t ) * q.z + t * r.z;
    return result;
  }
  float half_theta = acos( cos_half_theta );
  float a = sin( ( 1.0f - t ) * half_theta ) / sin_half_theta;
  float b = sin( t * half_theta ) / sin_half_theta;
  result.w = q.w * a + r.w * b;
  result.x = q.x * a + r.x * b;
  result.y = q.y * a + r.y * b;
  result.z = q.z * a + r.z * b;
  return result;
}

// a
// |
// b---c
static inline vec3 normal_from_triangle( vec3 a, vec3 b, vec3 c ) {
  vec3 edge_ab = sub_vec3_vec3( b, a );
  vec3 edge_bc = sub_vec3_vec3( c, b );
  vec3 cross = cross_vec3( edge_ab, edge_bc );
  vec3 n = normalise_vec3( cross );
  return n;
}

// assumption: interior of simple polygon is TO THE RIGHT
// i.e. is going in clockwise order around a polygon
// note: this is the opposite to what i would have called convex/concave
static inline bool is_vec2_convex( vec2 prev, vec2 curr, vec2 next ) {
  vec3 fwd, up, rgt;
  vec2 rgtv2, probev2;

  vec2 prev_to_curr = sub_vec2_vec2( curr, prev );
  vec2 curr_to_next = sub_vec2_vec2( next, curr );

  fwd.x = prev_to_curr.x;
  fwd.y = 0.0f;
  fwd.z = prev_to_curr.y;
  fwd = normalise_vec3( fwd );

  up = ( vec3 ){ 0, 1, 0 };

  rgt = cross_vec3( fwd, up );

  rgtv2.x = rgt.x;
  rgtv2.y = rgt.z;
  probev2 = normalise_vec2( curr_to_next );

  float dp = dot_vec2( rgtv2, probev2 );

  if ( dp <= 0.0f ) {
    return false;
  }
  return true;
}

// note: CLOCKWISE point order please
static inline float area_of_tri( vec2 a, vec2 b, vec2 c ) {
  return 0.5f * ( a.x * ( b.y - c.y ) + b.x * ( c.y - a.y ) + c.x * ( a.y - b.y ) );
}

// point inside triangle - john vince page 174
// note: CLOCKWISE point order please
// note: if one area is zero and all others positive this point is on boundary
//       if two areas are zero and other positive -> point is on a vertex
static inline bool is_vec2_in_tri( vec2 probe, vec2 a, vec2 b, vec2 c ) {
  float aa = area_of_tri( a, b, probe );
  float ab = area_of_tri( b, c, probe );
  float ac = area_of_tri( c, a, probe );
  if ( aa > 0.0f && ab > 0.0f && ac > 0.0f ) {
    return true;
  }
  return false;
}

// note: verts should be ORDERERD in sequence, taking RIGHT-HAND turns around
// note: WILL NOT WORK on non-simple polygon
// returns index of ear
// -- inside_us versus a second original list (DOES THIS DO ANYTHING? - dont think so)
// TODO -- do a hole-finding version using a third list
static inline int find_ear_in_simple_polygon( vec2 *verts, int nverts ) {
  int current = 0;
  bool ear_found = false;
  while ( !ear_found ) {
    int prev = loopmod( current - 1, nverts );
    int next = loopmod( current + 1, nverts );
    vec2 curr_vec2 = verts[current];
    vec2 prev_vec2 = verts[prev];
    vec2 next_vec2 = verts[next];
    next_vec2.y = -next_vec2.y;
    prev_vec2.y = -prev_vec2.y;
    curr_vec2.y = -curr_vec2.y;
    // WORKS
    bool current_is_convex = is_vec2_convex( prev_vec2, curr_vec2, next_vec2 );
    // current_is_convex = true;
    if ( current_is_convex ) {
      ear_found = true; // unless disproven shortly
      // make sure no CONCAVE are inside
      for ( int probe = 0; probe < nverts; probe++ ) {
        // dunno if i need this --> not if triangle test successfully ignore same sames
        // took it out cos using original list and numbers differed
        if ( probe == current || probe == prev || probe == next ) {
          continue;
        }
        int probe_prev = loopmod( probe - 1, nverts );
        int probe_next = loopmod( probe + 1, nverts );
        vec2 probe_curr_vec2 = verts[probe];
        vec2 probe_prev_vec2 = verts[probe_prev];
        vec2 probe_next_vec2 = verts[probe_next];
        probe_next_vec2.y = -probe_next_vec2.y;
        probe_prev_vec2.y = -probe_prev_vec2.y;
        probe_curr_vec2.y = -probe_curr_vec2.y;
        bool probe_is_convex =
          is_vec2_convex( probe_prev_vec2, probe_curr_vec2, probe_next_vec2 );
        // NOTE: worked better when removing the above test
        probe_is_convex = false;
        if ( !probe_is_convex ) {
          // WORKS
          bool is_inside_us =
            is_vec2_in_tri( probe_prev_vec2, prev_vec2, curr_vec2, next_vec2 );
          if ( is_inside_us ) {
            ear_found = false;
            break;
          }
        }
      } // endfor probe
    }   // endif current_is_convex
    if ( ear_found ) {
      break; // ear found
    }
    current++;
    if ( current == nverts ) {
      return -1;
    }
  } // endwhile
  return current;
}
