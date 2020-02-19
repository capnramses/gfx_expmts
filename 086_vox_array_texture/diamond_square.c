// C99

#include "diamond_square.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define CLAMP( x, lo, hi ) ( MIN( hi, MAX( lo, x ) ) )

static int _comp( const void* e1, const void* e2 ) {
  int f = *( (int*)e1 );
  int s = *( (int*)e2 );
  if ( f > s ) return 1;
  if ( f < s ) return -1;
  return 0;
}

static void _median_filter( uint8_t* output, const uint8_t* input, int dims ) {
  assert( output && input );

  int window[9];

  const int win_dims = 3;
  int edgex          = ( win_dims / 2 ); // rounded down
  int edgey          = ( win_dims / 2 ); // rounded down

  // NOTE(Anton) other mediam filters start at (edge_x, edge_y) and go to (dims - edge_x, dims - edge_y).
  // This ignores the border of pixels around the edge of the image. Therefore I go from (0,0) to (dims,dims),
  // but clamp the result into the dims range for the whole image.
  for ( int y = 0; y < dims; y++ ) {
    for ( int x = 0; x < dims; x++ ) {
      int i = 0;
      for ( int fy = 0; fy < win_dims; fy++ ) {
        for ( int fx = 0; fx < win_dims; fx++ ) {
          int ix        = x + fx - edgex;
          int iy        = y + fy - edgey;
          ix            = CLAMP( ix, 0, dims - 1 );
          iy            = CLAMP( iy, 0, dims - 1 );
          int input_idx = iy * dims + ix;
          window[i]     = input[input_idx];
          i++;
        }
      }
      qsort( window, 9, sizeof( int ), _comp );
      int output_idx     = y * dims + x;
      output[output_idx] = window[9 / 2];
    }
  }
}

// TODO(Anton) this wrap for negatives is unsafe
static uint8_t sample( int x, int y, dsquare_heightmap_t* hm ) {
  int xx = x >= 0 ? x % hm->w : ( hm->w + x ) % hm->w;
  int yy = y >= 0 ? y % hm->h : ( hm->h + y ) % hm->h;
  return hm->unfiltered_heightmap[hm->w * yy + xx];
}

static void set_sample( int x, int y, int value, dsquare_heightmap_t* hm ) {
  int xx                                    = x >= 0 ? x % hm->w : ( hm->w + x ) % hm->w;
  int yy                                    = y >= 0 ? y % hm->h : ( hm->h + y ) % hm->h;
  hm->unfiltered_heightmap[hm->w * yy + xx] = CLAMP( value, 0, 255 );
}

static int _apg_rand( int x1, int x2 ) {
  int min = MIN( x1, x2 );
  int max = MAX( x1, x2 );
  return min + rand() % ( max - min );
}

static void sample_square( int x, int y, int halfstep, int noise, dsquare_heightmap_t* hm ) {
  // a     b
  //
  //    x
  //
  // c     d
  uint8_t a = sample( x - halfstep, y - halfstep, hm );
  uint8_t b = sample( x + halfstep, y - halfstep, hm );
  uint8_t c = sample( x - halfstep, y + halfstep, hm );
  uint8_t d = sample( x + halfstep, y + halfstep, hm );
  set_sample( x, y, ( ( a + b + c + d ) / 4.0 ) + noise, hm );
}

static void sample_diamond( int x, int y, int halfstep, int noise, dsquare_heightmap_t* hm ) {
  //    c
  //
  // a  x  b
  //
  //    d
  uint8_t a = sample( x - halfstep, y, hm );
  uint8_t b = sample( x + halfstep, y, hm );
  uint8_t c = sample( x, y - halfstep, hm );
  uint8_t d = sample( x, y + halfstep, hm );
  set_sample( x, y, ( ( a + b + c + d ) / 4.0 ) + noise, hm );
}

static void diamond_square( int step_size, int noise_scale, dsquare_heightmap_t* hm ) {
  while ( step_size > 1 ) {
    int halfstep = step_size / 2;

    for ( int y = halfstep; y < hm->h + halfstep; y += step_size ) {
      for ( int x = halfstep; x < hm->w + halfstep; x += step_size ) {
        sample_square( x, y, halfstep, _apg_rand( -noise_scale, noise_scale ), hm ); //
      }
    }

    for ( int y = 0; y < hm->h; y += step_size ) {
      for ( int x = 0; x < hm->w; x += step_size ) {
        sample_diamond( x + halfstep, y, halfstep, _apg_rand( -noise_scale, noise_scale ), hm );
        sample_diamond( x, y + halfstep, halfstep, _apg_rand( -noise_scale, noise_scale ), hm );
      }
    }
    step_size /= 2;
    noise_scale = noise_scale > 2 ? noise_scale / 2 : 1;
  } // endwhile
}

dsquare_heightmap_t dsquare_heightmap_alloc( int dims, int default_height ) {
  assert( dims > 0 );
  dsquare_heightmap_t h;
  h.w                    = dims;
  h.h                    = dims;
  h.unfiltered_heightmap = (uint8_t*)calloc( h.w * h.h, sizeof( uint8_t ) );
  h.filtered_heightmap   = (uint8_t*)calloc( h.w * h.h, sizeof( uint8_t ) );
  assert( h.unfiltered_heightmap && h.filtered_heightmap );
  memset( h.unfiltered_heightmap, default_height, h.w * h.h * sizeof( uint8_t ) );
  return h;
}

void dsquare_heightmap_free( dsquare_heightmap_t* hm ) {
  assert( hm && hm->unfiltered_heightmap && hm->filtered_heightmap );
  free( hm->unfiltered_heightmap );
  free( hm->filtered_heightmap );
  memset( hm, 0, sizeof( dsquare_heightmap_t ) );
}

void dsquare_heightmap_gen( dsquare_heightmap_t* hm, int noise_scale, int feature_size, int feature_max_height ) {
  assert( hm && hm->unfiltered_heightmap && hm->w > 0 && hm->h > 0 );
  // create some starting crap
  for ( int y = 0; y < hm->h; y += feature_size ) {
    for ( int x = 0; x < hm->w; x += feature_size ) { set_sample( x, y, _apg_rand( 0, feature_max_height ), hm ); }
  }
  // algorithm
  int step_size = feature_size;
  noise_scale   = noise_scale > 0 ? noise_scale : 1;
  diamond_square( step_size, noise_scale, hm );

  assert( hm->w == hm->h );
  _median_filter( hm->filtered_heightmap, hm->unfiltered_heightmap, hm->w );

  // write out the image
  int result = stbi_write_png( "unfiltered_heightmap_out.png", hm->w, hm->h, 1, hm->unfiltered_heightmap, hm->w );
  if ( !result ) { fprintf( stderr, "ERROR: could not write out image to unfiltered_heightmap_out.png\n" ); }
  result = stbi_write_png( "filtered_heightmap_out.png", hm->w, hm->h, 1, hm->filtered_heightmap, hm->w );
  if ( !result ) { fprintf( stderr, "ERROR: could not write out image to filtered_heightmap_out.png\n" ); }
}
