#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void set_pixel( uint8_t* image_ptr, int w, int n_channels, int x, int y, uint8_t r, uint8_t g, uint8_t b ) {
  assert( image_ptr );

  int index            = ( w * y + x ) * n_channels;
  image_ptr[index + 0] = r;
  image_ptr[index + 1] = g;
  image_ptr[index + 2] = b;
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
  int width = 512, height = 512, n_channels = 3;
  size_t sz               = width * height * n_channels * sizeof( uint8_t );
  uint8_t* image_data_ptr = calloc( 1, sz );
  assert( image_data_ptr );

  set_pixel( image_data_ptr, width, n_channels, 0, 0, 0xFF, 0x00, 0x00 );

  if ( !write_ppm( "out.ppm", image_data_ptr, width, height ) ) {
    fprintf( stderr, "ERROR: could not write ppm file\n" );
    return 1;
  }

  free( image_data_ptr );

  printf( "Program done\n" );
  return 0;
}
