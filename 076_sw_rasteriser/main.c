#include "apg_maths.h"
#include "apg_ply.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define CLAMP( x, lo, hi ) ( MIN( hi, MAX( lo, x ) ) )

typedef struct rgb_byte_t {
  uint8_t r, g, b;
} rgb_byte_t;

typedef struct ivec2_t {
  int x, y;
} ivec2_t;

typedef struct ivec3_t {
  int x, y, z;
} ivec3_t;

typedef struct vertex_t {
  vec3 pos;
  rgb_byte_t colour;
} vertex_t;

int cross_ivec2( ivec2_t a, ivec2_t b ) { return a.x * b.y - a.y * b.x; }

void set_pixel( uint8_t* image_ptr, int w, int n_channels, int x, int y, rgb_byte_t rgb ) {
  assert( image_ptr );

  int index            = ( w * y + x ) * n_channels;
  image_ptr[index + 0] = rgb.r;
  image_ptr[index + 1] = rgb.g;
  image_ptr[index + 2] = rgb.b;
}

// described here: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage
void fill_triangle( vertex_t a, vertex_t b, vertex_t c, uint8_t* image_ptr, float* depth_buffer_ptr, int width, int height, int n_channels ) {
  assert( image_ptr && depth_buffer_ptr );

  // found triangle bounds and clip min,max within image bounds
  int min_x = MAX( MIN( a.pos.x, MIN( b.pos.x, c.pos.x ) ), 0 );
  int max_x = MIN( MAX( a.pos.x, MAX( b.pos.x, c.pos.x ) ), width - 1 );
  int min_y = MAX( MIN( a.pos.y, MIN( b.pos.y, c.pos.y ) ), 0 );
  int max_y = MIN( MAX( a.pos.y, MAX( b.pos.y, c.pos.y ) ), height - 1 );

  // fill in scanlines inside bbox
  for ( int y = min_y; y <= max_y; y++ ) {
    for ( int x = min_x; x <= max_x; x++ ) {
      vec3 p = ( vec3 ){ .x = x, .y = y };

      // TODO try barycentric instead of edge test
      vec3 bary = barycentric(
        ( vec2 ){ .x = x, .y = y }, ( vec2 ){ .x = a.pos.x, .y = a.pos.y }, ( vec2 ){ .x = b.pos.x, .y = b.pos.y }, ( vec2 ){ .x = c.pos.x, .y = c.pos.y } );

      // bool inside = edge_function( a.pos, b.pos, p ) && edge_function( b.pos, c.pos, p ) && edge_function( c.pos, a.pos, p );
      bool inside = true;
      if ( bary.x < 0 || bary.x >= 1 || bary.y < 0 || bary.y >= 1 || bary.z < 0 || bary.z >= 1 ) { inside = false; }
      if ( inside ) {
        // z is in range 0 (near) to 1 (far)
        float depthf = ( a.pos.z * bary.x + b.pos.z * bary.y + c.pos.z * bary.z );
        // if ( depthf < 0.0 || depthf >= 1.0f ) { continue; } // depth is no longer 0 to 1
        if ( depthf <= depth_buffer_ptr[width * y + x] ) { continue; } // failed depth test

        depth_buffer_ptr[width * y + x] = depthf;     // store depth in depth buffer
        uint8_t d                       = depthf * 5; // for printing depth buffer

        float red      = ( a.colour.r * bary.x + b.colour.r * bary.y + c.colour.r * bary.z );
        float green    = ( a.colour.g * bary.x + b.colour.g * bary.y + c.colour.g * bary.z );
        float blue     = ( a.colour.b * bary.x + b.colour.b * bary.y + c.colour.b * bary.z );
        rgb_byte_t rgb = ( rgb_byte_t ){ .r = red, .g = green, .b = blue };

        set_pixel( image_ptr, width, n_channels, x, y, rgb );
      }
    }
  }
}

bool write_ppm( const char* filename, const uint8_t* image_ptr, int w, int h ) {
  assert( filename );
  assert( image_ptr );

  FILE* fp = fopen( filename, "wb" );
  if ( !fp ) { return false; }
  fprintf( fp, "P6\n%i %i\n255\n", w, h );
  fwrite( image_ptr, w * h * 3 * sizeof( uint8_t ), 1, fp );
  fclose( fp );

  return true;
}

int main( int argc, char** argv ) {
  if ( argc < 2 ) {
    printf( "usage: %s YOUR_MESH.ply\n", argv[0] );
    return 0;
  }
  apg_ply_t ply = apg_ply_read( argv[1] );
  if ( !ply.loaded ) {
    fprintf( stderr, "ERROR: ply didn't load\n" );
    return 1;
  }

  // alloc memory for RGB image
  int width = 512, height = 512, n_channels = 3;
  size_t sz               = width * height * n_channels * sizeof( uint8_t );
  uint8_t* image_data_ptr = calloc( 1, sz );
  assert( image_data_ptr );
  float* depth_buffer_ptr = calloc( 1, width * height * sizeof( float ) );
  assert( depth_buffer_ptr );
  const float nearc = 0.01f;
  const float farc  = 100.0f;

  mat4 P   = perspective( 66.6f, width / (float)height, nearc, farc );
  mat4 V   = look_at( ( vec3 ){ .x = 0, .y = 0, .z = 25 }, ( vec3 ){ .x = 0, .y = 0 }, ( vec3 ){ .y = 1.0f } );
  mat4 M   = mult_mat4_mat4( rot_y_deg_mat4( 45.0f ), scale_mat4( ( vec3 ){ 1, 1, 1 } ) );
  mat4 PV  = mult_mat4_mat4( P, V );
  mat4 PVM = mult_mat4_mat4( PV, M );

  // ==FOR EACH TRIANGLE'S VERTICES==
  for ( int i = 0; i < ply.n_vertices; i += 3 ) {
    vec4 vertex[3];
    vec3 colourf[3] = { ( vec3 ){ .x = 1 }, ( vec3 ){ .y = 1 }, ( vec3 ){ .z = 1 } };
    for ( int v = 0; v < 3; v++ ) {
      memcpy( &vertex[v].x, &ply.positions_ptr[( i + v ) * ply.n_positions_comps], sizeof( float ) * ply.n_positions_comps );
      if ( ply.colours_ptr ) { memcpy( &colourf[v].x, &ply.colours_ptr[( i + v ) * ply.n_colours_comps], sizeof( float ) * ply.n_colours_comps ); }
      vertex[v].w = 1.0f;

      // apply a world transformation to the geometry
      vertex[v] = mult_mat4_vec4( PVM, vertex[v] );
      vertex[v].x /= vertex[v].w;
      vertex[v].y /= vertex[v].w;
      // vertex[v].z /= vertex[v].w;

      // transform into viewport space
      vertex[v].x = vertex[v].x * width + width / 2;
      vertex[v].y = vertex[v].y * height + height / 2;
      // vertex[v].w = ( vertex[v].w - nearc ) / farc; // depth in range 1 (near) to 0 (far). anything outside can be clipped
      vertex[v].w = farc - vertex[v].w;
      //  printf( "%f\n", vertex[v].w );
    }
    // printf( "ws %f %f %f\n", vertex[0].w, vertex[1].w, vertex[2].w );

    // rasterise NOTE(Anton) I had to reverse abc winding order because it was rendering inside-out
    vertex_t a = ( vertex_t ){
      .pos.x = vertex[2].x, .pos.y = vertex[2].y, .pos.z = vertex[2].w, .colour.r = colourf[2].x * 255, .colour.g = colourf[2].y * 255, .colour.b = colourf[2].z * 255
    };
    vertex_t b = ( vertex_t ){
      .pos.x = vertex[1].x, .pos.y = vertex[1].y, .pos.z = vertex[1].w, .colour.r = colourf[1].x * 255, .colour.g = colourf[1].y * 255, .colour.b = colourf[1].z * 255
    };
    vertex_t c = ( vertex_t ){
      .pos.x = vertex[0].x, .pos.y = vertex[0].y, .pos.z = vertex[0].w, .colour.r = colourf[0].x * 255, .colour.g = colourf[0].y * 255, .colour.b = colourf[0].z * 255
    };
    fill_triangle( a, b, c, image_data_ptr, depth_buffer_ptr, width, height, n_channels );
  }

  if ( !write_ppm( "out.ppm", image_data_ptr, width, height ) ) {
    fprintf( stderr, "ERROR: could not write ppm file\n" );
    return 1;
  }

  free( image_data_ptr );

  printf( "Program done\n" );
  return 0;
}
