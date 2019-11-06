#include "voxels.h"
#define APG_TGA_IMPLEMENTATION
#include "../common/include/apg_tga.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define CLAMP( x, xmin, xmax ) ( ( x ) < ( xmin ) ? ( xmin ) : ( x ) > ( xmax ) ? ( xmax ) : ( x ) )

/*
* program maintains a chunks table
* chunks list has
-chunk (x,y) name/coords
-if it's loaded into memory
-if it's loadable from a file
* when loading a chunk
- is it already in memory?
- if not is it loadable from a file?
- if so then load it from within given chunks file by going to appropriate offset
- if not then generate chunk

chunks binary file:
31 -- number of chunks in file
-- table of chunk names and ~their memory offsets and size in the file
0,0 10 128
0,1 138 26
0,2 164 10
1,1
1,2 -1,0 -1,1 ...
-- first chunk's data
-- second chunk's data
...
*/

// clang-format off
//                                     x   y   z   x   y   z   x   y   z | x   y   z   x   y   z   x   y   z
static const float _west_face[]   = { -1,  1, -1, -1, -1, -1, -1,  1,  1, -1,  1,  1, -1, -1, -1, -1, -1,  1 };
static const float _east_face[]   = {  1,  1,  1,  1, -1,  1,  1,  1, -1,  1,  1, -1,  1, -1,  1,  1, -1, -1 };
static const float _bottom_face[] = { -1, -1,  1, -1, -1, -1,  1, -1,  1,  1, -1,  1, -1, -1, -1,  1, -1, -1 };
static const float _top_face[]    = { -1,  1, -1, -1,  1,  1,  1,  1, -1,  1,  1, -1, -1,  1,  1,  1,  1,  1 };
static const float _north_face[]  = {  1,  1, -1,  1, -1, -1, -1,  1, -1, -1,  1, -1,  1, -1, -1, -1, -1, -1 };
static const float _south_face[]  = { -1,  1,  1, -1, -1,  1,  1,  1,  1,  1,  1,  1, -1, -1,  1,  1, -1,  1 };

static float blue[]   = { 0, 0, 1 };
static float green[]  = { 0, 1, 0 };
static float cyan[]   = { 0, 1, 1 };
static float red[]    = { 1, 0, 0 };
static float violet[] = { 1, 0, 1 };
static float yellow[] = { 1, 1, 0 };
static float colour_crust[] = { 0.170, 0.157, 0.145 };
static float colour_dirt[] = { 0.550, 0.362, 0.187 };
static float colour_grass[] = { 0.0975, 0.650, 0.208 };
static float colour_stone[] = { 0.690, 0.649, 0.649 };
// clang-format on

chunk_vertex_data_t _test_cube_from_faces() {
  chunk_vertex_data_t cube = ( chunk_vertex_data_t ){ .sz = 6 * N_CUBE_FACE_BYTES, .n_vp_comps = 3, .n_vc_comps = 3 };
  cube.positions_ptr       = malloc( cube.sz );
  if ( !cube.positions_ptr ) { return cube; }
  cube.colours_ptr = malloc( cube.sz );
  if ( !cube.colours_ptr ) {
    free( cube.positions_ptr );
    cube.positions_ptr = NULL;
    return cube;
  }

  int float_idx         = 0;
  const float* faces[6] = { _west_face, _east_face, _bottom_face, _top_face, _north_face, _south_face };
  for ( int face_idx = 0; face_idx < 6; face_idx++ ) {
    memcpy( &cube.positions_ptr[float_idx], faces[face_idx], N_CUBE_FACE_BYTES );
    for ( int i = 0; i < N_CUBE_FACE_VERTS; i++ ) { memcpy( &cube.colours_ptr[float_idx + i * 3], blue, sizeof( float ) * 3 ); }
    float_idx += N_CUBE_FACE_FLOATS;
    cube.n_vertices += N_CUBE_FACE_VERTS;
  }

  return cube;
}

bool get_block_type_in_chunk( const chunk_t* chunk, int x, int y, int z, block_type_t* block_type ) {
  assert( chunk && chunk->voxels && block_type );
  if ( x < 0 || x >= CHUNK_X || y < 0 || y >= CHUNK_Y || z < 0 || z >= CHUNK_Z ) { return false; }

  int idx     = CHUNK_X * CHUNK_Z * y + CHUNK_X * z + x;
  *block_type = chunk->voxels[idx].type;
  return true;
}

// does nothing if coords are out of chunk bounds
bool set_block_type_in_chunk( chunk_t* chunk, int x, int y, int z, block_type_t type ) {
  assert( chunk && chunk->voxels );
  if ( x < 0 || x >= CHUNK_X || y < 0 || y >= CHUNK_Y || z < 0 || z >= CHUNK_Z ) { return false; }

  int idx = CHUNK_X * CHUNK_Z * y + CHUNK_X * z + x;

  block_type_t prev_type = chunk->voxels[idx].type;
  if ( prev_type == BLOCK_TYPE_AIR && type != BLOCK_TYPE_AIR ) { chunk->n_non_air_voxels++; }
  if ( prev_type != BLOCK_TYPE_AIR && type == BLOCK_TYPE_AIR ) { chunk->n_non_air_voxels--; }
  chunk->voxels[idx].type = type;

  int prev_height = chunk->heightmap[CHUNK_X * z + x];
  if ( y > prev_height && type != BLOCK_TYPE_AIR ) { // higher than before
    chunk->heightmap[CHUNK_X * z + x] = y;
  } else if ( y == prev_height && type == BLOCK_TYPE_AIR ) { // lowering
    for ( int yy = y; yy >= 0; yy-- ) {
      int idx = CHUNK_X * CHUNK_Z * y + CHUNK_X * z + x;
      if ( chunk->voxels[idx].type != BLOCK_TYPE_AIR ) {
        chunk->heightmap[CHUNK_X * z + x] = yy;
        break;
      }
    }
  }

  assert( chunk->n_non_air_voxels >= 0 && chunk->n_non_air_voxels <= CHUNK_X * CHUNK_Y * CHUNK_Z );

  return true;
}

// generate a y from an x and a z
uint8_t noisei( int x, int z, int max_height ) {
  int val = x * x * z + x; // TODO diamond square
  val %= max_height;
  return (uint8_t)CLAMP( val, 0, max_height ); // TODO can this limit be a problem?
}

// todo ifdef write_heightmap img
chunk_t chunk_generate() {
  chunk_t chunk;

  memset( &chunk, 0, sizeof( chunk_t ) );
  chunk.voxels = calloc( CHUNK_X * CHUNK_Y * CHUNK_Z, sizeof( voxel_t ) );
  assert( chunk.voxels );

  for ( int z = 0; z < CHUNK_Z; z++ ) {
    for ( int x = 0; x < CHUNK_X; x++ ) {
      uint8_t height = noisei( x, z, CHUNK_Y - 1 ); // TODO add position of chunk to x and y for continuous/infinite map
      set_block_type_in_chunk( &chunk, x, height, z, BLOCK_TYPE_GRASS );
      set_block_type_in_chunk( &chunk, x, height - 1, z, BLOCK_TYPE_DIRT );
      for ( int y = height - 2; y > 0; y-- ) { set_block_type_in_chunk( &chunk, x, y, z, BLOCK_TYPE_STONE ); }
      set_block_type_in_chunk( &chunk, x, 0, z, BLOCK_TYPE_CRUST );
    } // x
  }   // z

  return chunk;
}

bool chunk_load_from_file( const char* filename, chunk_t* chunk ) {
  assert( filename && chunk );

  memset( chunk, 0, sizeof( chunk_t ) );
  chunk->voxels = calloc( CHUNK_X * CHUNK_Y * CHUNK_Z, sizeof( voxel_t ) );
  assert( chunk->voxels );

  FILE* fptr = fopen( filename, "rb" );
  if ( !fptr ) { return false; }
  int n = fread( chunk->voxels, sizeof( voxel_t ) * CHUNK_X * CHUNK_Y * CHUNK_Z, 1, fptr );
  if ( 1 != n ) {
    fclose( fptr );
    return false;
  }
  n = fread( chunk->heightmap, sizeof( int ) * CHUNK_X * CHUNK_Z, 1, fptr );
  if ( 1 != n ) {
    fclose( fptr );
    return false;
  }
  n = fread( &chunk->n_non_air_voxels, sizeof( int ), 1, fptr );
  if ( 1 != n ) {
    fclose( fptr );
    return false;
  }
  fclose( fptr );
  return true;
}

bool chunk_save_to_file( const char* filename, const chunk_t* chunk ) {
  assert( filename && chunk && chunk->voxels );
  FILE* fptr = fopen( filename, "wb" );
  if ( !fptr ) { return false; }
  int n = fwrite( chunk->voxels, sizeof( voxel_t ) * CHUNK_X * CHUNK_Y * CHUNK_Z, 1, fptr );
  if ( 1 != n ) {
    fclose( fptr );
    return false;
  }
  n = fwrite( chunk->heightmap, sizeof( int ) * CHUNK_X * CHUNK_Z, 1, fptr );
  if ( 1 != n ) {
    fclose( fptr );
    return false;
  }
  n = fwrite( &chunk->n_non_air_voxels, sizeof( int ), 1, fptr );
  if ( 1 != n ) {
    fclose( fptr );
    return false;
  }
  fclose( fptr );
  return true;
}

bool chunk_write_heightmap( const char* filename, const chunk_t* chunk ) {
  assert( filename && chunk && chunk->voxels );
  uint8_t* img = malloc( CHUNK_X * CHUNK_Z * 3 );
  for ( int z = 0; z < CHUNK_Z; z++ ) {
    for ( int x = 0; x < CHUNK_X; x++ ) {
      img[( CHUNK_X * z + x ) * 3] = img[( CHUNK_X * z + x ) * 3 + 1] = img[( CHUNK_X * z + x ) * 3 + 2] = chunk->heightmap[CHUNK_X * z + x];
    }
  }
  bool result = (bool)apg_tga_write_file( filename, img, CHUNK_X, CHUNK_Z, 3 );
  free( img );
  return result;
}

void chunk_free( chunk_t* chunk ) {
  assert( chunk && chunk->voxels );

  free( chunk->voxels );
  memset( chunk, 0, sizeof( chunk_t ) );
}

static void _memcpy_colour( block_type_t block_type, float* dest ) {
  assert( dest );

  float* colour_ptr = NULL;
  switch ( block_type ) {
  case BLOCK_TYPE_CRUST: {
    colour_ptr = colour_crust;
  } break;
  case BLOCK_TYPE_GRASS: {
    colour_ptr = colour_grass;
  } break;
  case BLOCK_TYPE_DIRT: {
    colour_ptr = colour_dirt;
  } break;
  case BLOCK_TYPE_STONE: {
    colour_ptr = colour_stone;
  } break;
  default: { assert( false ); } break;
  }
  for ( int i = 0; i < 6; i++ ) { memcpy( dest + i * 3, colour_ptr, sizeof( float ) * 3 ); }
}

chunk_vertex_data_t chunk_gen_vertex_data( const chunk_t* chunk ) {
  chunk_vertex_data_t data = ( chunk_vertex_data_t ){ .n_vp_comps = 3, .n_vc_comps = 3 };

  data.sz            = N_FULL_CUBE_BYTES * chunk->n_non_air_voxels;
  data.positions_ptr = malloc( data.sz );
  assert( data.positions_ptr );
  data.colours_ptr = malloc( data.sz );
  assert( data.colours_ptr );

  int total_floats = 0;

  for ( int y = 0; y < CHUNK_Y; y++ ) {
    for ( int z = 0; z < CHUNK_Z; z++ ) {
      for ( int x = 0; x < CHUNK_X; x++ ) {
        block_type_t our_block_type = 0;
        bool ret                    = get_block_type_in_chunk( chunk, x, y, z, &our_block_type );
        int floats_added_this_block = 0;
        assert( ret );
        if ( our_block_type == BLOCK_TYPE_AIR ) { continue; }
        assert( total_floats < N_FULL_CUBE_BYTES * chunk->n_non_air_voxels );

        {
          // clang-format off
          int xs[6]             = { -1,  1,  0,  0,  0,  0 };
          int ys[6]             = {  0,  0, -1,  1,  0,  0 };
          int zs[6]             = {  0,  0,  0,  0, -1,  1 };
          // clang-format on
          const float* faces[6] = { _west_face, _east_face, _bottom_face, _top_face, _north_face, _south_face };
          block_type_t neighbour_block_type;
          for ( int i = 0; i < 6; i++ ) {
            bool ret = get_block_type_in_chunk( chunk, x + xs[i], y + ys[i], z + zs[i], &neighbour_block_type );
            if ( !ret || neighbour_block_type == BLOCK_TYPE_AIR ) {
              memcpy( &data.positions_ptr[total_floats + floats_added_this_block], faces[i], N_CUBE_FACE_BYTES );
              _memcpy_colour( our_block_type, &data.colours_ptr[total_floats + floats_added_this_block] );
              floats_added_this_block += N_CUBE_FACE_FLOATS;
            }
          }
        }

        // assumes xyz positions are first three bytes per vertex
        for ( int i = 0; i < floats_added_this_block; i += 3 ) {
          data.positions_ptr[total_floats + i + 0] += x * 2.0f;
          data.positions_ptr[total_floats + i + 1] += y * 2.0f;
          data.positions_ptr[total_floats + i + 2] += z * 2.0f;
        }
        total_floats += floats_added_this_block;
      } // endfor x
    }   // endfor z
  }     // endfor y

  data.n_vp_comps = N_CUBE_FACE_VERT_COMPS;
  data.n_vertices = total_floats / N_CUBE_FACE_VERT_COMPS;
  printf( " data resized from %u to %u\n", (uint32_t)data.sz * 2, ( uint32_t )( total_floats * sizeof( float ) ) * 2 );
  data.sz            = total_floats * sizeof( float );
  data.positions_ptr = realloc( data.positions_ptr, data.sz );
  assert( data.positions_ptr );
  data.colours_ptr = realloc( data.colours_ptr, data.sz );
  assert( data.colours_ptr );

  /* TODO - mesh rules
    - if AIR draw nothing
    - if WATER - special rules
    - if no cube west - add left
    - if no cube east - add right
    - if no cube north - add front
    - if no cube south - add rear
  */

  return data;
}

void chunk_free_vertex_data( chunk_vertex_data_t* chunk_vertex_data ) {
  assert( chunk_vertex_data && chunk_vertex_data->positions_ptr );

  free( chunk_vertex_data->positions_ptr );
  memset( chunk_vertex_data, 0, sizeof( chunk_vertex_data_t ) );
}