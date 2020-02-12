#include "voxels.h"
#define APG_TGA_IMPLEMENTATION
#include "../common/include/apg_tga.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* TODO / rough plan
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
// clang-format on

static int palette_grass = 0;
static int palette_stone = 1;
static int palette_dirt  = 2;
static int palette_crust = 3;

chunk_vertex_data_t _test_cube_from_faces() {
  chunk_vertex_data_t cube = ( chunk_vertex_data_t ){
    .n_vp_comps = VOXEL_VP_COMPS, .n_vt_comps = VOXEL_VT_COMPS, .n_vpalidx_comps = VOXEL_VPALIDX_COMPS, .n_vn_comps = VOXEL_VN_COMPS, .n_vpicking_comps = VOXEL_VPICKING_COMPS
  };
  cube.positions_ptr = malloc( VOXEL_CUBE_VP_BYTES );
  if ( !cube.positions_ptr ) { return cube; }
  cube.palette_indices_ptr = malloc( VOXEL_CUBE_VPALIDX_BYTES );
  if ( !cube.palette_indices_ptr ) {
    free( cube.positions_ptr );
    cube.positions_ptr = NULL;
    return cube;
  }

  int float_idx                     = 0;
  int palette_indices_idx           = 0;
  const float* faces[6]             = { _west_face, _east_face, _bottom_face, _top_face, _north_face, _south_face };
  const uint32_t palette_indices[6] = { 0, 1, 2, 3, 4, 5 };
  for ( int face_idx = 0; face_idx < 6; face_idx++ ) {
    memcpy( &cube.positions_ptr[float_idx], faces[face_idx], VOXEL_FACE_VP_BYTES );
    for ( int i = 0; i < VOXEL_FACE_VERTS; i++ ) {
      memcpy( &cube.palette_indices_ptr[palette_indices_idx + i], &palette_indices[face_idx], sizeof( uint32_t ) );
    }
    float_idx += VOXEL_FACE_VP_FLOATS;
    palette_indices_idx += VOXEL_FACE_VERTS;
    cube.n_vertices += VOXEL_FACE_VERTS;
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

bool is_voxel_above_surface( const chunk_t* chunk, int x, int y, int z ) {
  assert( chunk && chunk->voxels );
  // edges of chunk will be lit by sunlight
  if ( x < 0 || x >= CHUNK_X || y < 0 || y >= CHUNK_Y || z < 0 || z >= CHUNK_Z ) { return true; }

  int idx    = CHUNK_X * z + x;
  int height = chunk->heightmap[idx];
  if ( y > height ) { return true; }
  return false;
}

bool is_voxel_face_exposed_to_sun( const chunk_t* chunk, int x, int y, int z, int face_idx ) {
  assert( chunk && chunk->voxels );

  switch ( face_idx ) {
  case 0: return is_voxel_above_surface( chunk, x - 1, y, z );
  case 1: return is_voxel_above_surface( chunk, x + 1, y, z );
  case 2: return is_voxel_above_surface( chunk, x, y - 1, z );
  case 3: return is_voxel_above_surface( chunk, x, y + 1, z );
  case 4: return is_voxel_above_surface( chunk, x, y, z - 1 );
  case 5: return is_voxel_above_surface( chunk, x, y, z + 1 );
  default: assert( false ); break;
  }
  return false;
}

bool set_block_type_in_chunk( chunk_t* chunk, int x, int y, int z, block_type_t type ) {
  assert( chunk && chunk->voxels );
  if ( x < 0 || x >= CHUNK_X || y < 0 || y >= CHUNK_Y || z < 0 || z >= CHUNK_Z ) { return false; }

  int idx = CHUNK_X * CHUNK_Z * y + CHUNK_X * z + x;

  block_type_t prev_type = chunk->voxels[idx].type;
  if ( prev_type == type ) { return false; } // no change!
  if ( prev_type == BLOCK_TYPE_AIR && type != BLOCK_TYPE_AIR ) { chunk->n_non_air_voxels++; }
  if ( prev_type != BLOCK_TYPE_AIR && type == BLOCK_TYPE_AIR ) {
    assert( chunk->n_non_air_voxels > 0 );
    chunk->n_non_air_voxels--;
  }
  chunk->voxels[idx].type = type;

  int prev_height = chunk->heightmap[CHUNK_X * z + x];
  if ( y > prev_height && type != BLOCK_TYPE_AIR ) { // higher than before
    chunk->heightmap[CHUNK_X * z + x] = y;
  } else if ( y == prev_height && type == BLOCK_TYPE_AIR ) { // lowering
    for ( int yy = y; yy >= 0; yy-- ) {
      int idx = CHUNK_X * CHUNK_Z * yy + CHUNK_X * z + x;
      if ( chunk->voxels[idx].type != BLOCK_TYPE_AIR ) {
        chunk->heightmap[CHUNK_X * z + x] = yy;
        break;
      }
    }
  }

  assert( chunk->n_non_air_voxels <= CHUNK_X * CHUNK_Y * CHUNK_Z );

  return true;
}

// generate a y from an x and a z
uint8_t noisei( int x, int z, int max_height ) {
  int val = x * x * z + x; // TODO diamond square
  val %= max_height;
  return (uint8_t)CLAMP( val, 0, max_height ); // TODO can this limit be a problem?
}

// todo ifdef write_heightmap img
chunk_t chunk_generate( const uint8_t* heightmap, int hm_w, int hm_h, int x_offset, int z_offset ) {
  assert( heightmap );

  chunk_t chunk;
  memset( &chunk, 0, sizeof( chunk_t ) );
  chunk.voxels = calloc( CHUNK_X * CHUNK_Y * CHUNK_Z, sizeof( voxel_t ) );
  assert( chunk.voxels );

  for ( int z = 0; z < CHUNK_Z; z++ ) {
    for ( int x = 0; x < CHUNK_X; x++ ) {
      // uint8_t height = noisei( x, z, CHUNK_Y - 1 ); // TODO add position of chunk to x and y for continuous/infinite map

      int xx         = x_offset + x;
      int zz         = z_offset + z;
      int idx        = hm_w * zz + xx; // uses offsets and width/height of map
      const int underground_height = 16;
      const int heightmap_sample = heightmap[idx]; // uint8_t to int to avoid overflow when adding a hm sample of 255 to underground height > 0

      uint8_t height = CLAMP( heightmap_sample + underground_height, 1, CHUNK_Y - 1 ); // -1 because chunk_y is 256 which would wrap around to 0 in a uint8

      set_block_type_in_chunk( &chunk, x, height, z, BLOCK_TYPE_GRASS );
      set_block_type_in_chunk( &chunk, x, height - 1, z, BLOCK_TYPE_DIRT );
      for ( int y = height - 2; y > 0; y-- ) { set_block_type_in_chunk( &chunk, x, y, z, BLOCK_TYPE_STONE ); }
      set_block_type_in_chunk( &chunk, x, 0, z, BLOCK_TYPE_CRUST );
    } // x
  }   // z

  return chunk;
}

chunk_t chunk_generate_flat() {
  chunk_t chunk;

  memset( &chunk, 0, sizeof( chunk_t ) );
  chunk.voxels = calloc( CHUNK_X * CHUNK_Y * CHUNK_Z, sizeof( voxel_t ) );
  assert( chunk.voxels );

  for ( int z = 0; z < CHUNK_Z; z++ ) {
    for ( int x = 0; x < CHUNK_X; x++ ) {
      uint8_t height = 0;
      set_block_type_in_chunk( &chunk, x, height, z, BLOCK_TYPE_CRUST );
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
  n = fread( &chunk->n_non_air_voxels, sizeof( uint32_t ), 1, fptr );
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
  n = fwrite( &chunk->n_non_air_voxels, sizeof( uint32_t ), 1, fptr );
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

static void _memcpy_face_palidx( block_type_t block_type, uint32_t* dest ) {
  assert( dest );

  uint32_t palidx = 0;
  switch ( block_type ) {
  case BLOCK_TYPE_CRUST: {
    palidx = palette_crust;
  } break;
  case BLOCK_TYPE_GRASS: {
    palidx = palette_grass;
  } break;
  case BLOCK_TYPE_DIRT: {
    palidx = palette_dirt;
  } break;
  case BLOCK_TYPE_STONE: {
    palidx = palette_stone;
  } break;
  default: { assert( false ); } break;
  }
  // per the 6 vertices in this face
  for ( int v = 0; v < VOXEL_FACE_VERTS; v++ ) { *( dest + v ) = palidx; }
}

static void _memcpy_face_picking( int x, int y, int z, int face_idx, float* dest, size_t curr_float, size_t max_floats ) {
  uint32_t a    = z * CHUNK_X + x; // up 0-255 (16*16). to retrieve: z= a/CHUNK_X, x=a%CHUNK_X (integer rounding maths)
  uint32_t b    = y;               // up 0-255
  uint32_t c    = face_idx;        // up 0-5
  float buff[3] = { a / 255.0f, b / 255.0f, c / 255.0f };
  // per the 6 vertices in this face
  assert( curr_float + VOXEL_VPICKING_COMPS * VOXEL_FACE_VERTS < max_floats );
  for ( int v = 0; v < VOXEL_FACE_VERTS; v++ ) { memcpy( &dest[curr_float + v * VOXEL_VPICKING_COMPS], buff, sizeof( float ) * VOXEL_VPICKING_COMPS ); }
}

static void _memcpy_face_texcoords( const chunk_t* chunk, float* dest, size_t curr_float, size_t max_floats ) {
  assert( chunk );

  const float buff[] = { 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0 };
  assert( curr_float + VOXEL_VT_COMPS * VOXEL_FACE_VERTS < max_floats );
  memcpy( &dest[curr_float], buff, sizeof( float ) * VOXEL_VT_COMPS * VOXEL_FACE_VERTS );
}

static void _memcpy_face_normals( const chunk_t* chunk, int x, int y, int z, int face_idx, float* dest, size_t curr_float, size_t max_floats ) {
  assert( chunk );

  float buff[4] = { 0, 0, 0, 0 };
  switch ( face_idx ) {
  case 0: buff[0] = -1.0f; break;
  case 1: buff[0] = 1.0f; break;
  case 2: buff[1] = -1.0f; break;
  case 3: buff[1] = 1.0f; break;
  case 4: buff[2] = -1.0f; break;
  case 5: buff[2] = 1.0f; break;
  default: assert( false ); break;
  }
  bool sunlit = is_voxel_face_exposed_to_sun( chunk, x, y, z, face_idx );
  if ( sunlit ) { buff[3] = 1.0f; }
  assert( curr_float + VOXEL_VN_COMPS * VOXEL_FACE_VERTS < max_floats );
  for ( int v = 0; v < VOXEL_FACE_VERTS; v++ ) { memcpy( &dest[curr_float + v * VOXEL_VN_COMPS], buff, sizeof( float ) * VOXEL_VN_COMPS ); }
}

chunk_vertex_data_t chunk_gen_vertex_data( const chunk_t* chunk, int from_y_inclusive, int to_y_exclusive ) {
	assert( chunk );
	assert( from_y_inclusive >= 0 && to_y_exclusive <= CHUNK_Y );

  chunk_vertex_data_t data = ( chunk_vertex_data_t ){
    .n_vp_comps = VOXEL_VP_COMPS, .n_vt_comps = VOXEL_VT_COMPS, .n_vpicking_comps = VOXEL_VPICKING_COMPS, .n_vn_comps = VOXEL_VN_COMPS, .n_vpalidx_comps = VOXEL_VPALIDX_COMPS
  };
  data.vp_buffer_sz       = VOXEL_CUBE_VP_BYTES * chunk->n_non_air_voxels;
  data.vn_buffer_sz       = VOXEL_CUBE_VN_BYTES * chunk->n_non_air_voxels;
  data.vt_buffer_sz       = VOXEL_CUBE_VT_BYTES * chunk->n_non_air_voxels;
  data.vpicking_buffer_sz = VOXEL_CUBE_VPICKING_BYTES * chunk->n_non_air_voxels;
  data.vpalidx_buffer_sz  = VOXEL_CUBE_VPALIDX_BYTES * chunk->n_non_air_voxels;
  data.positions_ptr      = malloc( data.vp_buffer_sz );
  assert( data.positions_ptr );
  data.palette_indices_ptr = malloc( data.vpalidx_buffer_sz );
  assert( data.palette_indices_ptr );
  data.picking_ptr = malloc( data.vpicking_buffer_sz );
  assert( data.picking_ptr );
  data.texcoords_ptr = malloc( data.vt_buffer_sz );
  assert( data.texcoords_ptr );
  data.normals_ptr = malloc( data.vn_buffer_sz );
  assert( data.normals_ptr );

  uint32_t total_vp_floats  = 0;
  uint32_t total_vt_floats  = 0;
  uint32_t total_vn_floats  = 0;
  uint32_t total_palindices = 0;

  for ( int y = from_y_inclusive; y < to_y_exclusive; y++ ) {
    for ( int z = 0; z < CHUNK_Z; z++ ) {
      for ( int x = 0; x < CHUNK_X; x++ ) {
        block_type_t our_block_type = 0;
        bool ret                    = get_block_type_in_chunk( chunk, x, y, z, &our_block_type );
        int floats_added_this_block = 0;
        assert( ret );
        if ( our_block_type == BLOCK_TYPE_AIR ) { continue; }
        assert( total_vp_floats < VOXEL_CUBE_VP_FLOATS * chunk->n_non_air_voxels );

        {
          // clang-format off
          int xs[6]             = { -1,  1,  0,  0,  0,  0 };
          int ys[6]             = {  0,  0, -1,  1,  0,  0 };
          int zs[6]             = {  0,  0,  0,  0, -1,  1 };
          // clang-format on
          const float* faces[6] = { _west_face, _east_face, _bottom_face, _top_face, _north_face, _south_face };
          block_type_t neighbour_block_type;
          for ( int face_idx = 0; face_idx < 6; face_idx++ ) {
            bool ret = get_block_type_in_chunk( chunk, x + xs[face_idx], y + ys[face_idx], z + zs[face_idx], &neighbour_block_type );
            // if face is valid then add one face's worth of vertex data to the buffer
            if ( !ret || neighbour_block_type == BLOCK_TYPE_AIR ) {
              memcpy( &data.positions_ptr[total_vp_floats + floats_added_this_block], faces[face_idx], VOXEL_FACE_VP_BYTES );
              _memcpy_face_palidx( our_block_type, &data.palette_indices_ptr[total_palindices] );
              _memcpy_face_picking( x, y, z, face_idx, data.picking_ptr, total_vp_floats + floats_added_this_block, VOXEL_CUBE_VPICKING_FLOATS * chunk->n_non_air_voxels );
              _memcpy_face_texcoords( chunk, data.texcoords_ptr, total_vt_floats, VOXEL_CUBE_VT_FLOATS * chunk->n_non_air_voxels );
              _memcpy_face_normals( chunk, x, y, z, face_idx, data.normals_ptr, total_vn_floats, VOXEL_CUBE_VN_FLOATS * chunk->n_non_air_voxels );
              floats_added_this_block += VOXEL_FACE_VP_FLOATS;
              total_vt_floats += VOXEL_FACE_VT_FLOATS;
              total_vn_floats += VOXEL_FACE_VN_FLOATS;
              total_palindices += VOXEL_FACE_VPALIDX_UINTS;
            }
          }
        }

        // assumes xyz positions are first three bytes per vertex
        for ( int i = 0; i < floats_added_this_block; i += 3 ) {
          data.positions_ptr[total_vp_floats + i + 0] += x * 2.0f;
          data.positions_ptr[total_vp_floats + i + 1] += y * 2.0f;
          data.positions_ptr[total_vp_floats + i + 2] += z * 2.0f;
        }
        total_vp_floats += floats_added_this_block;
      } // endfor x
    }   // endfor z
  }     // endfor y

  data.n_vp_comps         = VOXEL_VP_COMPS;
  data.n_vertices         = total_vp_floats / VOXEL_VP_COMPS;
  data.vp_buffer_sz       = total_vp_floats * sizeof( float );
  data.vt_buffer_sz       = total_vt_floats * sizeof( float );
  data.vn_buffer_sz       = total_vn_floats * sizeof( float );
  data.vpicking_buffer_sz = total_vp_floats * sizeof( float ); // NOTE(Anton) same as vp
  data.vpalidx_buffer_sz  = total_palindices * sizeof( uint32_t );
  // TODO(Anton) add a tmp var to check for realloc success for each
  data.positions_ptr = realloc( data.positions_ptr, data.vp_buffer_sz );
  assert( data.positions_ptr );
  data.palette_indices_ptr = realloc( data.palette_indices_ptr, data.vpalidx_buffer_sz );
  assert( data.palette_indices_ptr );
  data.picking_ptr = realloc( data.picking_ptr, data.vpicking_buffer_sz );
  assert( data.picking_ptr );
  data.texcoords_ptr = realloc( data.texcoords_ptr, data.vt_buffer_sz );
  assert( data.texcoords_ptr );
  data.normals_ptr = realloc( data.normals_ptr, data.vn_buffer_sz );
  assert( data.normals_ptr );

  return data;
}

void chunk_free_vertex_data( chunk_vertex_data_t* chunk_vertex_data ) {
  assert( chunk_vertex_data && chunk_vertex_data->positions_ptr );

  free( chunk_vertex_data->positions_ptr );
  free( chunk_vertex_data->palette_indices_ptr );
  free( chunk_vertex_data->picking_ptr );
  free( chunk_vertex_data->texcoords_ptr );
  free( chunk_vertex_data->normals_ptr );
  memset( chunk_vertex_data, 0, sizeof( chunk_vertex_data_t ) );
}

bool picked_colour_to_voxel_idx( uint8_t r, uint8_t g, uint8_t b, uint8_t a, int* x, int* y, int* z, int* face, int* chunk_id ) {
  if ( b > 5 ) { return false; }
  *x        = (int)r % CHUNK_X;
  *y        = (int)g;
  *z        = (int)r / CHUNK_X;
  *face     = (int)b;
  *chunk_id = (int)a;
  return true;
}
