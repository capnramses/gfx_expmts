// Generate some default palettes.

#include "apg_bmp.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

static int _cmp( const void* u, const void* v ) {
  uint8_t* a = (uint8_t*)u;
  uint8_t* b = (uint8_t*)v;
  int x      = ( 256 * 256 * a[2] ) + 256 * a[1] + a[0];
  int y      = ( 256 * 256 * b[2] ) + 256 * b[1] + b[0];
  return x - y;
}

int main() {
  srand( time( NULL ) );

  uint8_t* pal_ptr = calloc( 256 * 3, 1 );
  assert( pal_ptr );

  for ( int i = 0; i < 256 * 3; i++ ) { pal_ptr[i] = rand() % 255; }
  pal_ptr[0] = pal_ptr[1] = pal_ptr[2] = 0;
  pal_ptr[3] = pal_ptr[4] = pal_ptr[5] = 255;
  qsort( pal_ptr, 256, 3, _cmp );

  FILE* f_ptr = fopen( "my.pal", "wb" );
  assert( f_ptr );
  fwrite( pal_ptr, 1, 256 * 3, f_ptr );
  fclose( f_ptr );

  apg_bmp_write( "mypal.bmp", pal_ptr, 16, 16, 3 );

  {
    for ( int i = 0; i < 256; i++ ) {
      pal_ptr[i * 3 + 0] = i;
      pal_ptr[i * 3 + 1] = 0;
      pal_ptr[i * 3 + 2] = 0;
    }

    f_ptr = fopen( "reds.pal", "wb" );
    assert( f_ptr );
    fwrite( pal_ptr, 1, 256 * 3, f_ptr );
    fclose( f_ptr );
    apg_bmp_write( "reds.bmp", pal_ptr, 16, 16, 3 );
  }
  {
    int w, h, n;
    uint8_t* bpal = stbi_load( "blood-1x.png", &w, &h, &n, 3 );
    printf("loaded blood pal %ix%i @ %i\n", w,h,n);
    f_ptr = fopen( "blood.pal", "wb" );
    assert( f_ptr );
    fwrite( bpal, 1, 256 * 3, f_ptr );
    fclose( f_ptr );
    apg_bmp_write( "blood.bmp", bpal, 16, 16, 3 );
    free( bpal );
  }
  free( pal_ptr );

  return 0;
}
