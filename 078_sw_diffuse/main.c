#include "apg_maths.h"
#include "apg_ply.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../common/include/stb/stb_image_write.h"
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
  vec3 pos_wor;
  vec3 n_wor;
  vec3 colour;
} vertex_t;

// output image properties
int width = 2048, height = 2048, n_channels = 3;

vec3 g_light_pos, g_light_colour;

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

      // try barycentric instead of edge test for rasterising triangles. code for this function is in apg_maths.h
      vec3 bary = barycentric(
        ( vec2 ){ .x = x, .y = y }, ( vec2 ){ .x = a.pos.x, .y = a.pos.y }, ( vec2 ){ .x = b.pos.x, .y = b.pos.y }, ( vec2 ){ .x = c.pos.x, .y = c.pos.y } );

      bool inside = true;
      if ( bary.x < 0 || bary.x >= 1 || bary.y < 0 || bary.y >= 1 || bary.z < 0 || bary.z >= 1 ) { inside = false; }
      if ( inside ) {
        // z is in range 0 (near) to 1 (far)
        float depthf = ( a.pos.z * bary.x + b.pos.z * bary.y + c.pos.z * bary.z );
        if ( depthf <= depth_buffer_ptr[width * y + x] ) { continue; } // failed depth test

        depth_buffer_ptr[width * y + x] = depthf;     // store depth in depth buffer
        uint8_t d                       = depthf * 5; // for printing depth buffer

        vec3 frag_colour;
        frag_colour.x = ( a.colour.x * bary.x + b.colour.x * bary.y + c.colour.x * bary.z );
        frag_colour.y = ( a.colour.y * bary.x + b.colour.y * bary.y + c.colour.y * bary.z );
        frag_colour.z = ( a.colour.z * bary.x + b.colour.z * bary.y + c.colour.z * bary.z );

        // lighting
        vec3 interpolated_pos = add_vec3_vec3( add_vec3_vec3( mult_vec3_f( a.pos_wor, bary.x ), mult_vec3_f( b.pos_wor, bary.y ) ), mult_vec3_f( c.pos_wor, bary.z ) );
        vec3 dir_to_light   = sub_vec3_vec3( g_light_pos, interpolated_pos );
        vec3 dtl_n          = normalise_vec3( dir_to_light );
        vec3 interpolated_n = add_vec3_vec3( add_vec3_vec3( mult_vec3_f( a.n_wor, bary.x ), mult_vec3_f( b.n_wor, bary.y ) ), mult_vec3_f( c.n_wor, bary.z ) );
        interpolated_n      = normalise_vec3( interpolated_n );
        float l_dot_n       = CLAMP( dot_vec3( dtl_n, interpolated_n ), 0, 1 );
        frag_colour         = mult_vec3_f( frag_colour, l_dot_n );

        // convert colour from 0.0 to 1.0 range to 0-255 byte. and colour the pixel
        rgb_byte_t rgb = ( rgb_byte_t ){ .r = frag_colour.x * 255.0, .g = frag_colour.y * 255.0, .b = frag_colour.z * 255.0 };
        set_pixel( image_ptr, width, n_channels, x, width - y - 1, rgb );
      }
    }
  }
}

// function loads a .ply mesh file given as an argument on the command line. eg drag a .ply onto the .exe in Explorer
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

  // NOTE(Anton) my -Y was upwards...which is kind of silly
  g_light_pos    = ( vec3 ){ .x = 0, .y = 100, .z = 100 };
  g_light_colour = ( vec3 ){ .x = 1, .y = 1, .z = 1 };

  // alloc memory for RGB image
  size_t sz               = width * height * n_channels * sizeof( uint8_t );
  uint8_t* image_data_ptr = calloc( 1, sz );
  assert( image_data_ptr );
  float* depth_buffer_ptr = calloc( 1, width * height * sizeof( float ) );
  assert( depth_buffer_ptr );
  const float nearc = 0.01f;
  const float farc  = 1000.0f;
  for ( int i = 0; i < sz; i++ ) { image_data_ptr[i] = 100; } // grey background

  // set up transformation matrices (P is camera perpsective, V is camera orientation and position, M is 'position the model in the world'
  // PVM is a combination of all three so that i can do v' = PVM * v instead of v' = P * V * M * v
  mat4 P   = perspective( 66.6f, width / (float)height, nearc, farc );
  vec3 upv = normalise_vec3( ( vec3 ){ .y = 1.0f, .z = -1.0 } );
  mat4 V   = look_at( ( vec3 ){ .x = 0, .y = 20, .z = 30 }, ( vec3 ){ .x = 0, .y = 5 }, upv );
  mat4 M   = mult_mat4_mat4( rot_y_deg_mat4( 134.0f ), scale_mat4( ( vec3 ){ 1, 1, 1 } ) );
  mat4 VM  = mult_mat4_mat4( V, M ); // model-view matrix
  mat4 PVM = mult_mat4_mat4( P, VM );

  // ==FOR EACH TRIANGLE'S VERTICES==
  for ( int i = 0; i < ply.n_vertices; i += 3 ) {
    vec4 vertex[3];
    vec4 vertex_wor[3];
    vec3 normal[3];
    vec4 normal_wor[3];
    vec3 colourf[3] = { ( vec3 ){ .x = 1 }, ( vec3 ){ .y = 1 }, ( vec3 ){ .z = 1 } };
    // every 3 vertices is 1 triangle's worth
    for ( int v = 0; v < 3; v++ ) {
      memcpy( &vertex[v].x, &ply.positions_ptr[( i + v ) * ply.n_positions_comps], sizeof( float ) * ply.n_positions_comps );
      if ( ply.normals_ptr ) { memcpy( &normal[v].x, &ply.normals_ptr[( i + v ) * ply.n_normals_comps], sizeof( float ) * ply.n_normals_comps ); }
      if ( ply.colours_ptr ) { memcpy( &colourf[v].x, &ply.colours_ptr[( i + v ) * ply.n_colours_comps], sizeof( float ) * ply.n_colours_comps ); }
      vertex[v].w = 1.0f;

      vertex_wor[v] = mult_mat4_vec4( M, vertex[v] );
      normal_wor[v] = mult_mat4_vec4( M, v4_v3f( normal[v], 0.0f ) );

      // apply a world transformation to the geometry
      vertex[v] = mult_mat4_vec4( PVM, vertex[v] );
      vertex[v].x /= vertex[v].w;
      vertex[v].y /= vertex[v].w;

      // transform into viewport space
      vertex[v].x = vertex[v].x * width + width / 2;
      vertex[v].y = vertex[v].y * height + height / 2;
      // vertex[v].w = ( vertex[v].w - nearc ) / farc; // depth in range 1 (near) to 0 (far). anything outside can be clipped
      vertex[v].w = farc - vertex[v].w;
    } // endfor 3

    // rasterise NOTE(Anton) I had to reverse abc winding order because it was rendering inside-out
    vertex_t verts[3];
    for ( int i = 0; i < 3; i++ ) {
      verts[i] = ( vertex_t ){
        .pos.x = vertex[i].x, .pos.y = vertex[i].y, .pos.z = vertex[i].w, .pos_wor = v3_v4( vertex_wor[i] ), .colour = colourf[i], .n_wor = v3_v4( normal_wor[i] )
      };
    }
    fill_triangle( verts[2], verts[1], verts[0], image_data_ptr, depth_buffer_ptr, width, height, n_channels );
  }

  // write out result to an image file
  stbi_write_png( "out.png", width, height, 3, image_data_ptr, width * 3 );

  // delete allocated memory
  free( image_data_ptr );

  printf( "Program done\n" );
  return 0;
}
