/* a simple test program to be compiled as C++ and run from the command line */

#include "apg_bmp.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../common/include/stb/stb_image_write.h"
#include <iostream>

int main( int argc, char** argv ) {
  if ( argc < 2 ) {
    printf( "usage: ./demo INFILE.BMP\n" );
    return 0;
  }
  std::cout << "Reading " << argv[1] << std::endl;
  int w = 0, h = 0, n_chans = 0, vertical_flip = 0;
  unsigned char* img_mem = apg_read_bmp( argv[1], &w, &h, &n_chans, vertical_flip );
  if ( !img_mem ) {
    std::cerr << "ERROR reading image\n";
    return 1;
  }

  std::cout << "Writing test_out.png" << std::endl;

  int ret = stbi_write_png( "test_out.png", w, h, n_chans, img_mem, w * n_chans );
  if ( !ret ) {
    std::cerr << "ERROR writing image\n";
    return 1;
  }

  std::cout << "Writing test_out.bmp" << std::endl;
  ret = apg_write_bmp( "test_out.bmp", img_mem, w, h, n_chans );
  if ( !ret ) {
    std::cerr << "ERROR writing image\n";
    return 1;
  }

  free( img_mem );
  return 0;
}