/**
 * gcc main.c gfx.c glad/src/gl.c -I glad/include/ -lglfw
 */

#include "gfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

int main() {
  srand( time( NULL ) );

  gfx_t gfx = gfx_start( 800, 600, "3D Texture Demo" );
  if ( !gfx.started ) { return 1; }

  int w = 16, h = 16, d = 16;
  int n            = 3;
  uint8_t* img_ptr = calloc( 1, w * h * d * n );
  for ( int z = 0; z < d; z++ ) {
    for ( int y = 0; y < h; y++ ) {
      for ( int x = 0; x < w; x++ ) {
        int idx              = z * w * h + y * w + x;
        img_ptr[idx * n + 0] = x * ( 256 / w );
        img_ptr[idx * n + 1] = y * ( 256 / h );
        img_ptr[idx * n + 2] = z * ( 256 / d );
      }
    }
  }

  free( img_ptr );

  //
  //
  //

  gfx_stop();

  printf("Normal exit.\n");

  return 0;
}
