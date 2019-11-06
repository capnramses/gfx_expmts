// BMP File Reader
// Copyright Dr Anton Gerdelan <antongdl@protonmail.com> 2019
// C99

#include "apg_bmp.h"
#define STBI_ONLY_BMP
#define STB_IMAGE_IMPLEMENTATION
#include "../common/include/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../common/include/stb/stb_image_write.h"
#include <stdio.h>

// uncomment the stb read/write functions and comment the apg ones for comparison

int main( int argc, char** argv ) {
  if ( argc < 3 ) {
    printf( "usage: ./reader INFILE.BMP OUTFILE.BMP\n" );
    return 0;
  }
  printf( "reading file `%s`\n", argv[1] );

  int w = 0, h = 0, n = 0;
  unsigned char* pixel_data = apg_read_bmp( argv[1], &w, &h, &n );
  // unsigned char* pixel_data = stbi_load( argv[1], &w, &h, &n, 0 );

  printf( "writing file `%s`\n", argv[2] );

  if ( !apg_write_bmp( argv[2], pixel_data, w, h, n ) ) {
    fprintf( stderr, "ERROR writing BMP file `%s`\n", argv[2] );
    return 1;
  }
  // stbi_write_bmp( argv[2], w, h, n, pixel_data );

  return 0;
}

/*
quick test using `time` on my linux desktop
                       writing
                 none stb_bmp apg_bmp
        stb_bmp        0.18    0.14
reading apg_bmp        0.18    0.14

all pretty quick - looks like my writing code is a little faster
writing with pngs added another 0.6 seconds or so onto program execution time
*/