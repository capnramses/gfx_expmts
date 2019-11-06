#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define CLAMP( x, lo, hi ) ( MIN( hi, MAX( lo, x ) ) )

typedef struct ivec2_t {
  int x, y;
} ivec2_t;

typedef struct ivec3_t {
  int x, y, z;
} ivec3_t;

int cross_ivec2( ivec2_t a, ivec2_t b ) { return a.x * b.y - a.y * b.x; }

void set_pixel( uint8_t* image_ptr, int w, int n_channels, int x, int y, uint8_t r, uint8_t g, uint8_t b ) {
  assert( image_ptr );

  int index            = ( w * y + x ) * n_channels;
  image_ptr[index + 0] = r;
  image_ptr[index + 1] = g;
  image_ptr[index + 2] = b;
}

bool edge_function( ivec2_t a, ivec2_t b, ivec2_t c ) { return ( ( c.x - a.x ) * ( b.y - a.y ) - ( c.y - a.y ) * ( b.x - a.x ) >= 0 ); }

// described here: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage
void fill_triangle( ivec2_t a, ivec2_t b, ivec2_t c, uint8_t* image_ptr, int width, int height, int n_channels, uint8_t red, uint8_t green, uint8_t blue ) {
  assert( image_ptr );

  int min_x = MIN( a.x, MIN( b.x, c.x ) );
  int max_x = MAX( a.x, MAX( b.x, c.x ) );
  int min_y = MIN( a.y, MIN( b.y, c.y ) );
  int max_y = MAX( a.y, MAX( b.y, c.y ) );

  // TODO clip min,max within image bounds

  // fill in scanlines inside bbox
  for ( int y = min_y; y <= max_y; y++ ) {
    for ( int x = min_x; x <= max_x; x++ ) {
      ivec2_t p   = ( ivec2_t ){ .x = x, .y = y };
      bool inside = true;
      inside &= edge_function( a, b, p );
      inside &= edge_function( b, c, p );
      inside &= edge_function( c, a, p );

      if ( inside ) { set_pixel( image_ptr, width, n_channels, x, y, red, green, blue ); }
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

int main() {
  // alloc memory for RGB image
  int width = 256, height = 256, n_channels = 3;
  size_t sz               = width * height * n_channels * sizeof( uint8_t );
  uint8_t* image_data_ptr = calloc( 1, sz );
  assert( image_data_ptr );

  set_pixel( image_data_ptr, width, n_channels, 0, 0, 0xFF, 0x00, 0x00 );

  fill_triangle( ( ivec2_t ){ .x = 32, .y = 32 }, ( ivec2_t ){ .x = 32, .y = height-1 }, ( ivec2_t ){ .x = width-1, .y = 128 }, image_data_ptr, width, height, n_channels,
    0xFF, 0x00, 0xFF );

  if ( !write_ppm( "out.ppm", image_data_ptr, width, height ) ) {
    fprintf( stderr, "ERROR: could not write ppm file\n" );
    return 1;
  }

  free( image_data_ptr );

  printf( "Program done\n" );
  return 0;
}
