/* WIP demo to try out some glTF loading libraries and display options */

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include <stdio.h>

int main( int argc, char** argv ) {
  if ( argc < 2 ) {
    printf( "usage: %s YOUR_GLTF_FILE.json\n", argv[0] );
    return 0;
  }
  printf( "loading `%s`\n", argv[1] );
  {
    cgltf_options options = { 0 };
    cgltf_data* data      = NULL;
    cgltf_result result   = cgltf_parse_file( &options, argv[1], &data );
    if ( result == cgltf_result_success ) {
      /* TODO make awesome stuff */
      cgltf_free( data );
    } else {
      printf( "error loading gltf file\n" );
      return 1;
    }
  }

  printf( "done\n" );
  return 0;
}