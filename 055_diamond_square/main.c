// An Implementation of the Diamond-Square algorithm (Fournier, Fussell and Carpenter at SIGGRAPH 1982) for heightmap output
// based on http://www.bluh.org/code-the-diamond-square-algorithm/
// Anton Gerdelan 15 July 2019
// gcc main.c apg_bmp.c -I ..\common\include\stb\
// C99

#include "apg_bmp.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#define MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define CLAMP( x, lo, hi ) ( MIN( hi, MAX( lo, x ) ) )

uint8_t* heightmap;
int width, height;

// NOTE(Anton) this wrap for negatives is unsafe
uint8_t sample( int x, int y ) {
  int xx = x >= 0 ? x % width : ( width + x ) % width;
  int yy = y >= 0 ? y % height : ( height + y ) % height;
  return heightmap[width * yy + xx];
}
void set_sample( int x, int y, int value ) {
  int xx                     = x >= 0 ? x % width : ( width + x ) % width;
  int yy                     = y >= 0 ? y % height : ( height + y ) % height;
  heightmap[width * yy + xx] = CLAMP( value, 0, 255 );
}

static int _apg_rand( int x1, int x2 ) {
  int min = MIN( x1, x2 );
  int max = MAX( x1, x2 );
  return min + rand() % ( max - min );
}

void sample_square( int x, int y, int halfstep, int noise ) {
  // a     b
  //
  //    x
  //
  // c     d
  uint8_t a = sample( x - halfstep, y - halfstep );
  uint8_t b = sample( x + halfstep, y - halfstep );
  uint8_t c = sample( x - halfstep, y + halfstep );
  uint8_t d = sample( x + halfstep, y + halfstep );
  set_sample( x, y, ( ( a + b + c + d ) / 4.0 ) + noise );
}

void sample_diamond( int x, int y, int halfstep, int noise ) {
  //    c
  //
  // a  x  b
  //
  //    d
  uint8_t a = sample( x - halfstep, y );
  uint8_t b = sample( x + halfstep, y );
  uint8_t c = sample( x, y - halfstep );
  uint8_t d = sample( x, y + halfstep );
  set_sample( x, y, ( ( a + b + c + d ) / 4.0 ) + noise );
}

void diamond_square( int step_size, int noise_scale ) {
  while ( step_size > 1 ) {
    int halfstep = step_size / 2;

    for ( int y = halfstep; y < height + halfstep; y += step_size ) {
      for ( int x = halfstep; x < width + halfstep; x += step_size ) {
        sample_square( x, y, halfstep, _apg_rand( -noise_scale, noise_scale ) ); //
      }
    }

    for ( int y = 0; y < height; y += step_size ) {
      for ( int x = 0; x < width; x += step_size ) {
        sample_diamond( x + halfstep, y, halfstep, _apg_rand( -noise_scale, noise_scale ) );
        sample_diamond( x, y + halfstep, halfstep, _apg_rand( -noise_scale, noise_scale ) );
      }
    }
    step_size /= 2;
    noise_scale = noise_scale > 2 ? noise_scale / 2 : 1;
  } // endwhile
}

int main( int argc, char** argv ) {
  if ( argc < 4 ) {
    printf( "usage: %s WIDTH NOISE_SCALE FILENAME\n", argv[0] );
    return 0;
  }
  srand( time( NULL ) );

  width  = atoi( argv[1] );
  height = width;
  printf( "heightmap %ix%i\n", width, height );

  heightmap = (uint8_t*)calloc( width * height, sizeof( uint8_t ) );
  memset( heightmap, 127, width * height * sizeof( uint8_t ) );
  {
    printf( "feature seeding...\n" );
    // create some starting crap
    int feature_size = 128; // bigger equals wider mountainous/tables
    for ( int y = 0; y < height; y += feature_size ) {
      for ( int x = 0; x < width; x += feature_size ) { set_sample( x, y, _apg_rand( 0, 255 ) ); }
    }

    printf( "alg...\n" );
    // algorithm
    int step_size   = feature_size;
    int noise_scale = atoi( argv[2] );
    noise_scale     = noise_scale > 0 ? noise_scale : 1;
    diamond_square( step_size, noise_scale );

    printf( "writing image to `%s`\n", argv[3] );
    // write out the image
    int result = stbi_write_png( "stb.bmp", width, height, 1, heightmap, width );
    result =apg_write_bmp( argv[3], heightmap, width, height, 1 );
	if ( !result ) { fprintf( stderr, "ERROR: could not write out image to `%s`\n", argv[3] ); }
  }
  free( heightmap );
  printf( "HALT\n" );
  return 0;
}
