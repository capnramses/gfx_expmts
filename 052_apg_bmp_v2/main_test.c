// Basic demo/test of BMP File Reader/Writer
// Anton Gerdelan
// Licence: see apg_bmp.h
// C99
//
// gcc -o test main_test.c apg_bmp.c

#include "apg_bmp.h"

#ifdef USE_STB_INSTEAD

#define STBI_ONLY_BMP
#define STB_IMAGE_IMPLEMENTATION
#include "../common/include/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../common/include/stb/stb_image_write.h"

#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// uncomment the stb read/write functions and comment the apg ones for comparison

int main( int argc, char** argv ) {
  if ( argc < 3 ) {
    printf( "usage: ./reader INFILE.BMP OUTFILE.BMP\n" );
    return 0;
  }
  printf( "reading file `%s`\n", argv[1] );

  int w = 0, h = 0, n = 0;
#ifdef USE_STB_INSTEAD
  unsigned char* pixel_data = stbi_load( argv[1], &w, &h, &n, 0 );
#else
	bool vertically_flip = true;
  unsigned char* pixel_data = apg_read_bmp( argv[1], &w, &h, &n, vertically_flip );
#endif
  if ( !pixel_data ) { return 1; }

  printf( "image loaded from `%s` %ix%i with %i channels. %u uncompressed bytes.\n", argv[1], w, h, n, w * h * n );

  printf( "writing file `%s` %ix%i with %i channels\n", argv[2], w, h, n );

#ifdef USE_STB_INSTEAD
  stbi_write_bmp( argv[2], w, h, n, pixel_data );
#else
  if ( !apg_write_bmp( argv[2], pixel_data, w, h, n ) ) {
    fprintf( stderr, "ERROR writing BMP file `%s`\n", argv[2] );
    return 1;
  }
#endif

  free( pixel_data ); // remember to free the loaded BMP file memory!

  return 0;
}

/*
test using `time` on my linux desktop
with a test image of 4800x4800 with 3 channels. 69120000 uncompressed bytes.
read and write combine:

apg_bmp: 0.396ms
stb:     0.796ms

and with the same image from a monochrome source:

apg_bmp: 0.422s
stb:     0.749s (output image channels corrupted)

all pretty quick - looks like my writing code is a little faster
writing with pngs added another 0.6 seconds+ onto program execution time
*/
