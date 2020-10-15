#include <assimp/cimport.h>     // Plain-C interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include <stdbool.h>
#include <stdio.h>

bool DoTheImportThing( const char* pFile ) {
  // Start the import on the given file with some example postprocessing
  // Usually - if speed is not the most important aspect for you - you'll t
  // probably to request more postprocessing than we do in this example.
  const struct aiScene* scene = aiImportFile( pFile, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType );

  // If the import failed, report it
  if ( NULL != scene ) {
    printf( "assimp error: %s\n", aiGetErrorString() );
    return false;
  }

  // Now we can access the file's contents
  {
    // access scene-> ...
  }

  // We're done. Release all resources associated with this import
  aiReleaseImport( scene );
  return true;
}

int main( int argc, char** argv ) {
  if ( argc < 3 ) {
    printf( "usage: convert INPUTMESH.x OUTPUTMESH.obj\n" );
    return 0;
  }

  if ( !DoTheImportThing( argv[1] ) ) { return 1; }

  return 0;
}
