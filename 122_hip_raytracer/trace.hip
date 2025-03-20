/**
 * HIP port of compute shader from 006_raytrace_cs/
 *
 * 12 Mar 2025 by Anton Gerdelan.
 *
 * hipcc trace.hip
 */

#include <hip/hip_runtime.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// HIP Error-check macro
// See https://rocm.docs.amd.com/projects/HIP/en/docs-develop/how-to/hip_runtime_api/error_handling.html
#define HIP_CHECK( expression )                                                                                                                                \
  {                                                                                                                                                            \
    const hipError_t status = expression;                                                                                                                      \
    if ( status != hipSuccess ) { fprintf( stderr, "HIP error: %s at %s:%i\n", hipGetErrorString( status ), __FILE__, __LINE__ ); }                            \
  }

#define APG_MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define APG_MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define APG_CLAMP( x, lo, hi ) ( APG_MIN( hi, APG_MAX( lo, x ) ) )

#define IMG_W 1024
#define IMG_H 1024
#define N_CHANS 3

struct vec3 {
  float x, y, z;
};

struct vec4 {
  float x, y, z, w;
};

__device__ __host__ vec3 addv3v3( vec3 a, vec3 b ) { return (vec3){ a.x + b.x, a.y + b.y, a.z + b.z }; }
__device__ __host__ vec3 subv3v3( vec3 a, vec3 b ) { return (vec3){ a.x - b.x, a.y - b.y, a.z - b.z }; }
__device__ __host__ float dotv3v3( vec3 a, vec3 b ) { return a.x * b.x + a.y * b.y + a.z * b.z; }
__device__ __host__ vec3 mulv3v3( vec3 a, vec3 b ) { return (vec3){ a.x * b.x, a.y * b.y, a.z * b.z }; }
__device__ __host__ vec3 mulv3f( vec3 v, float s ) { return (vec3){ v.x * s, v.y * s, v.z * s }; }
__device__ __host__ float lengthv3( vec3 v ) { return sqrtf( v.x * v.x + v.y * v.y + v.z * v.z ); }
__device__ __host__ vec3 normalisev3( vec3 v ) {
  float l = lengthv3( v );
  if ( 0.0f == l ) { return (vec3){ .x = 0.0f, .y = 0.0f, .z = 0.0f }; }
  return (vec3){ v.x / l, v.y / l, v.z / l };
}
__device__ __host__ vec3 reflectv3v3( vec3 i, vec3 n ) { return subv3v3( i, mulv3f( mulv3f( n, dotv3v3( n, i ) ), 2.0f ) ); }

__device__ __host__ float ray_sphere( vec3 ray_o, vec3 ray_d, vec3 sphere_c, float sphere_r, vec3* px ) {
  // t = -b +- sqrt (b*b - c)
  vec3 omc    = subv3v3( ray_o, sphere_c );
  float b     = dotv3v3( ray_d, omc );
  float c     = dotv3v3( omc, omc ) - sphere_r * sphere_r;
  float bsqmc = b * b - c;
  float t     = 10000.0f;

  if ( bsqmc < 0.0f ) { return t; }
  float srbsqmc = sqrtf( bsqmc );
  float pos_t   = -b + srbsqmc;
  float neg_t   = -b - srbsqmc;
  if ( pos_t <= 0.0f || neg_t <= 0.0f ) { return t; }
  t = pos_t < neg_t ? pos_t : neg_t;


  vec3 light_pos  = (vec3){ 5.0, 2.0, 0.0 };
  vec3 light_spec = (vec3){ 0.5, 0.5, 0.5 };
  vec3 light_diff = (vec3){ 0.5, 0.5, 0.5 };
  vec3 light_amb  = (vec3){ 0.33, 0.33, 0.33 };
  float spec_exp = 100.0f;

  vec3 sphere_kd = (vec3){ 0.2, 0.2, 0.9 };
  vec3 sphere_ka = (vec3){ 0.2, 0.2, 0.9 };
  vec3 sphere_ks = (vec3){ 1.0, 1.0, 1.0 };

  if ( t > 0.0f && t < 10000.0f ) {
    /* AMBIENT */
    vec3 rgb_a = mulv3v3( light_amb, sphere_ka );
    /* LAMBERT */
    // normal to a sphere is normalize (P - C);
    vec3 p = addv3v3( ray_o, mulv3f( ray_d, t ) );
    vec3 n = normalisev3( subv3v3( p, sphere_c ) );

    vec3 light_dir = normalisev3( subv3v3( p, light_pos ) );
    float diff_dp  = -dotv3v3( n, light_dir );
    diff_dp        = APG_MAX( diff_dp, 0.0 );

    // float max_range = 7.0f;
    // texel.g = 1.0 * (1.0 - t / max_range) + 0.5;
    vec3 rgb_d = mulv3v3( light_diff, mulv3f( sphere_kd, diff_dp ) );

    /* (not BLINN) PHONG */
    vec3 reflection        = reflectv3v3( light_dir, n );
    vec3 surface_to_viewer = mulv3f( normalisev3( p ), -1.0f ); // viewer is 0,0,0 so no need v-p
    float spec_dp          = dotv3v3( reflection, surface_to_viewer );
    spec_dp                = APG_MAX( spec_dp, 0.0 );
    float spec_fac         = powf( spec_dp, spec_exp );
    vec3 rgb_s             = mulv3v3( light_spec, mulv3f( sphere_ks, spec_fac ) );
    *px                    = addv3v3( rgb_a, addv3v3( rgb_d, rgb_s ) );
  }

  return t;
}

__device__ __host__ float dist_line_line( vec3 line_a_point_a, vec3 line_a_point_b, vec3 line_b_point_a, vec3 line_b_point_b, float* s, float* t ) {
  vec3 u  = subv3v3( line_a_point_b, line_a_point_a ); // this is just ray_d
  vec3 v  = subv3v3( line_b_point_b, line_b_point_a ); // this is cylinder inner central line
  vec3 w  = subv3v3( line_a_point_a, line_b_point_a ); // 'w' in diagram from lb_b to la_a
  float a = dotv3v3( u, u );
  float b = dotv3v3( u, v );
  float c = dotv3v3( v, v );
  float d = dotv3v3( u, w );
  float e = dotv3v3( v, w );
  float D = a * c - b * b; // determinant manually created i believe
  float sc, tc;            // closest points i believe

  // comp line params of 2 closest points
  if ( D < 0.0000001 ) { // almost parallel lines
    sc = 0.0;
    tc = b > c ? d / b : e / c; // use largest demoninator
  } else {
    sc = ( b * e - c * d ) / D;
    tc = ( a * e - b * d ) / D;
  }
  // distance between two closest points
  vec3 dist3d = addv3v3( w, subv3v3( mulv3f( u, sc ), mulv3f( v, tc ) ) );

  *s = sc; // distance 0-1 along first line seg from a->b
  *t = tc; // and along second
  // i used it to cut off the ends.
  return lengthv3( dist3d );
}

// capsule  ( a|----|b )
__device__ __host__ float ray_capsule( vec3 ray_o, vec3 ray_d, vec3 point_a, vec3 point_b, float cap_r, vec3* px ) {
  // do sphere-ray calcs for ends
  float t_a = ray_sphere( ray_o, ray_d, point_a, cap_r, px );
  float t_b = ray_sphere( ray_o, ray_d, point_b, cap_r, px );

  // ss and tt are the factors 0<->1 along each segment of nearest point
  float ss = 0.0, tt = 0.0;
  float dist = dist_line_line( ray_o, addv3v3( ray_o, mulv3f( ray_d, 100.0f ) ), point_a, point_b, &ss, &tt );

  if ( dist < cap_r && tt >= 0.0 && tt <= 1.0 ) {
    vec3 t_vec                    = subv3v3( point_b, point_a );
    vec3 point_on_cyl_medial_axis = addv3v3( point_a, mulv3f( t_vec, tt ) );
    float mid_t                   = ray_sphere( ray_o, ray_d, point_on_cyl_medial_axis, cap_r, px );
    return mid_t;
  }
  return APG_MIN( t_a, t_b );
}

__global__ void render_kernel( uint8_t* fb_ptr ) {
  int i = hipThreadIdx_x + hipBlockIdx_x * hipBlockDim_x;
  int j = hipThreadIdx_y + hipBlockIdx_y * hipBlockDim_y;
  if ( ( i >= IMG_W ) || ( j >= IMG_H ) ) return;

  vec3 px = (vec3){ 0.5, 0.5, 0.5 };

  vec3 light_pos  = (vec3){ 5.0, 2.0, 0.0 };
  vec3 light_spec = (vec3){ 0.5, 0.5, 0.5 };
  vec3 light_diff = (vec3){ 0.5, 0.5, 0.5 };
  vec3 light_amb  = (vec3){ 0.33, 0.33, 0.33 };
  float spec_exp = 100.0f;

  // sample or work-out ray origin and direction
  float max_x = 5.0;
  float max_y = 5.0;
  float x     = ( float( i * 2 - IMG_W ) / IMG_W );
  float y     = ( float( j * 2 - IMG_H ) / IMG_H );
  vec3 ray_o  = (vec3){ x * max_x, y * max_y, 0.0 };
  vec3 ray_d  = (vec3){ 0.0, 0.0, -1.0 }; // ortho

  // do intersection test

  /* SPHERE */
  vec3 sphere_c  = (vec3){ 0.0, 0.0, -10.0 };
  float sphere_r = 1.0;
  float sphere_t = ray_sphere( ray_o, ray_d, sphere_c, sphere_r, &px );


  /* CAPSULE */
  // NOTE: to make capsule cast a shadow, later for the plane do a light ray v
  // capsule and see if the cylinder OR the sphere is closer than view ray->plane t value
  float cap_length = 0.5f;
  vec3 capsule_a   = (vec3){ 3.0f, -1.0f, -10.0 };
  vec3 capsule_b   = (vec3){ 2.0f, -2.0f, -10.0 };
  float cap_r      = 1.0f;
  vec3 cyl_kd      = (vec3){ 0.5, 0.02, 0.02 };
  float cap_t      = ray_capsule( ray_o, ray_d, capsule_a, capsule_b, cap_r, &px );
  sphere_t = APG_MIN( sphere_t, cap_t );

  /* PLANE */
  vec3 plane_n  = normalisev3( (vec3){ 0.0, 0.0, 1.0 } );
  float plane_d = 11.0;
  vec3 plane_kd = (vec3){ 0.5, 0.5, 0.0 };
  vec3 plane_ks = (vec3){ 1.0, 1.0, 1.0 };
  vec3 plane_ka = (vec3){ 0.5, 0.5, 0.0 };
  float plane_t = -( ( dotv3v3( ray_o, plane_n ) + plane_d ) / dotv3v3( ray_d, plane_n ) );
  if ( plane_t > 0.0 && plane_t < sphere_t ) { // can now check if sphere_t is closer
    vec3 p         = addv3v3( ray_o, mulv3f( ray_d, plane_t ) );
    vec3 light_dir = normalisev3( subv3v3( p, light_pos ) );

    float plane_light_t = -( ( dotv3v3( light_pos, plane_n ) + plane_d ) / dotv3v3( light_dir, plane_n ) );

    /* check if there is a shadow */
    bool shadow = false;

    vec3 omc    = subv3v3( p, sphere_c );
    float b     = dotv3v3( normalisev3( subv3v3( light_pos, p ) ), omc );
    float c     = dotv3v3( omc, omc ) - sphere_r * sphere_r;
    float bsqmc = b * b - c;
    // hit one or both sides
    if ( bsqmc >= 0.0 ) { shadow = true; }

    float cap_light_t = ray_capsule( light_pos, light_dir, capsule_a, capsule_b, cap_r, &px );
    if ( cap_light_t < plane_light_t ) { shadow = true; }

    px = mulv3v3( light_amb, plane_ka );
    if ( !shadow ) {
      float diff_dp = -dotv3v3( plane_n, light_dir );
      diff_dp       = APG_MAX( diff_dp, 0.0 );
      px            = addv3v3( px, mulv3v3( light_diff, mulv3f( plane_kd, diff_dp ) ) );

      vec3 reflection        = reflectv3v3( light_dir, plane_n );
      vec3 surface_to_viewer = mulv3f( normalisev3( p ), -1.0f ); // viewer is 0,0,0 so no need v-p
      float spec_dp          = dotv3v3( reflection, surface_to_viewer );
      spec_dp                = APG_MAX( spec_dp, 0.0 );
      float spec_fac         = pow( spec_dp, spec_exp );
      px                     = addv3v3( px, mulv3v3( light_spec, mulv3f( plane_ks, spec_fac ) ) );
    }
  }

  /////////////////////////////////////

  int px_idx         = j * IMG_W * N_CHANS + i * N_CHANS;
  fb_ptr[px_idx + 0] = (uint8_t)APG_CLAMP( px.x * 255.9f, 0.0f, 255.0f );
  fb_ptr[px_idx + 1] = (uint8_t)APG_CLAMP( px.y * 255.9f, 0.0f, 255.0f );
  fb_ptr[px_idx + 2] = (uint8_t)APG_CLAMP( px.z * 255.9f, 0.0f, 255.0f );
}

int main() {
  hipDeviceProp_t devProp;
  HIP_CHECK( hipGetDeviceProperties( &devProp, 0 ) );
  printf( "ROCm HIP System: %i.%i %s\n", devProp.major, devProp.minor, devProp.name );

  printf( "Allocating unified memory for frame buffer..\n" );
  uint8_t* fb_ptr = NULL;
  size_t fb_sz    = N_CHANS * IMG_W * IMG_H * sizeof( uint8_t );
  HIP_CHECK( hipMallocManaged( (void**)&fb_ptr, fb_sz ) );

  {
    printf( "Launching kernel (rendering to frame buffer)..\n" );
    int threads_x = 8, threads_y = 8;
    dim3 blocks  = dim3( IMG_W / threads_x + 1, IMG_H / threads_y + 1 );
    dim3 threads = dim3( threads_x, threads_y );
    hipLaunchKernelGGL( render_kernel, blocks, threads, 0, 0, fb_ptr );
    HIP_CHECK( hipGetLastError() );
    HIP_CHECK( hipDeviceSynchronize() );
  }
  {
    FILE* f_ptr = fopen( "output.ppm", "w" );
    fprintf( f_ptr, "P3\n%i %i\n255\n", IMG_W, IMG_H );
    for ( int j = IMG_H - 1; j >= 0; j-- ) {
      for ( int i = 0; i < IMG_W; i++ ) {
        size_t px_idx = j * 3 * IMG_W + i * 3;
        fprintf( f_ptr, "%i %i %i\n", fb_ptr[px_idx + 0], fb_ptr[px_idx + 1], fb_ptr[px_idx + 2] );
      }
    }
    fclose( f_ptr );
  }

  printf( "Freeing memory..\n" );
  HIP_CHECK( hipFree( fb_ptr ) );

  vec3 op = addv3v3( (vec3){ 1, 2, 3 }, (vec3){ 4, 5, 6 } );
  printf( "test __host__ works:%.1f,%.1f,%.1f\n", op.x, op.y, op.z );

  printf( "Normal exit\n" );
  return 0;
}
