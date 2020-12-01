#include "gltf.h"
#include <stdint.h>
#include <stdio.h>

int main( int argc, char** argv ) {
  if ( argc < 2 ) {
    printf( "Usage ./a.out MYFILE.gltf\n" );
    return 0;
  }
  printf( "Reading `%s`\n", argv[1] );
  gltf_t gltf = { 0 };
  if ( !gltf_read( argv[1], &gltf ) ) {
    fprintf( stderr, "ERROR: reading gltf `%s`\n", argv[1] );
    return 1;
  }
  gltf_print( &gltf );

  gltf_free( &gltf );
  printf( "done\n" );
  return 0;
}
