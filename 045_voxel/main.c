// author/copright: Anton Gerdelan <antonofnote@gmail.com>. 16 July 2018
// licence: public domain, and use at your own risk.
//
// input-> a 2d grid with some annotations to say if floor, wall, etc
// -> interpret as a single layer of voxels ->
// output-> a single triangulated mesh with some basic optimisation (eg don't build interior voxel faces) and colour annotations
//
// C99
// compile:
//   gcc main.c -lm

// TODO -
// - encode texture id for each tile
// - render with appropriate texture
// - consider packing textures
// - otherwise use solid colours rather than textures + toon shader

#include "gfx.h"
#include "linmath.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../common/include/stb/stb_image.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum tile_types_t {
  TILE_AIR = 0,
  TILE_GRASS = 1,
  TILE_WALL_STONE = 2,
  TILE_WALL_GOLD_ORE = 5,
  TILE_WALL_SILVER_ORE = 6,
  TILE_WALL_IRON_ORE = 7,
  TILE_WOOD = 4,
  TILE_WATER = 3,
  TILE_STOCKPILE,
  TILE_STONE_SLAB,
  TILE_FLOOR_STONE,
  TILE_BRIDGE_STONE,
  TILE_TORCH,
  TILE_WATERWHEEL,
  TILE_FOOD_STORE,
  TILE_WALL_STONE_BRICKS,
  TILE_TYPE_MAX
} tile_types_t;

typedef struct tile_t {
  tile_types_t type;
  bool explored;
  bool pickaxe_tagged;
} tile_t;

#define TILES_ACROSS 50
#define TILES_DOWN 50
typedef struct map_t {
  tile_t tiles[TILES_DOWN][TILES_ACROSS];
} map_t;

map_t g_map;

bool load_map_file( const char* filename ) {
  //    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
  int x, y, n;
  uint8_t* data = stbi_load( filename, &x, &y, &n, 0 );
  if ( !data ) {
    fprintf( stderr, "ERROR: loading map file `%s`\n", filename );
    return false;
  }

  printf( "loaded map file %ix%i@%i\n", x, y, n );

  assert( y == TILES_DOWN );
  assert( x == TILES_ACROSS );

  for ( int yi = 0; yi < y; yi++ ) {
    for ( int xi = 0; xi < x; xi++ ) {
      int idx   = yi * y * n + xi * n;
      uint8_t r = data[idx];
      uint8_t g = data[idx + 1];
      uint8_t b = data[idx + 2];
      if ( 0 == r && 0 == g && 0 == b ) {
        g_map.tiles[yi][xi].type = TILE_AIR;
        continue;
      }
      if ( 0 == r && 255 == g && 0 == b ) {
        g_map.tiles[yi][xi].type = TILE_GRASS;
        continue;
      }
      if ( 0 == r && 128 == g && 0 == b ) {
        g_map.tiles[yi][xi].type = TILE_WOOD;
        continue;
      }
      if ( 128 == r && 128 == g && 128 == b ) {
        g_map.tiles[yi][xi].type = TILE_WALL_STONE;
        continue;
      }
      if ( 255 == r && 255 == g && 0 == b ) {
        g_map.tiles[yi][xi].type = TILE_WALL_GOLD_ORE;
        continue;
      }
      if ( 255 == r && 255 == g && 255 == b ) {
        g_map.tiles[yi][xi].type = TILE_WALL_SILVER_ORE;
        continue;
      }
      if ( 204 == r && 204 == g && 204 == b ) {
        g_map.tiles[yi][xi].type = TILE_WALL_IRON_ORE;
        continue;
      }
      if ( 0 == r && 0 == g && 255 == b ) {
        g_map.tiles[yi][xi].type = TILE_WATER;
        continue;
      }
      printf( "WARNING: unhandled colour: (%i,%i,%i)\n", r, g, b );
    }
  }

  free( data );
  return true;
}

static bool _is_tile_in_map( int x, int y ) {
  if ( x >= 0 && x < TILES_ACROSS && y >= 0 && y < TILES_DOWN ) { return true; }
  return false;
}

static bool _is_tile_a_floor( int x, int y ) {
  bool ret = _is_tile_in_map( x, y );
  assert( ret );

  if ( TILE_WOOD == g_map.tiles[y][x].type || TILE_WALL_STONE == g_map.tiles[y][x].type || TILE_WALL_GOLD_ORE == g_map.tiles[y][x].type ||
       TILE_WALL_SILVER_ORE == g_map.tiles[y][x].type || TILE_WALL_IRON_ORE == g_map.tiles[y][x].type ) {
    return false;
  }
  return true;
}

#define MAX_MESH_FLOATS 1000000
float vox_mesh[MAX_MESH_FLOATS];
int vox_mesh_floatcount;
int vox_mesh_vcount;
#define VOXEL_DIMS 1.0
#define VOXEL_VERT_ATTRIB_COMPS 6 // xyzstr

static void _voxelise_cube( int x, int y ) {
  bool ret = _is_tile_in_map( x, y );
  assert( ret );

  // if air do nothing
  tile_types_t type = g_map.tiles[y][x].type;
  if ( TILE_AIR == type ) { return; }

  // check if a floor or a wall
  bool is_floor = _is_tile_a_floor( x, y );

  // floors and walls always draw the top bit
  /*
     a--d   a : (x - 1/2, y - 1/2)
     |\ |   b : (x - 1/2, y + 1/2)
     | \|   c : (x + 1/2, y + 1/2)
     b--c   d : (x + 1/2, y - 1/2)
  */
  {
    float height = 0.0f;
    if ( !is_floor ) { height += 1.0f; }
    vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // a
    vox_mesh[vox_mesh_floatcount++] = height;
    vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // a
    vox_mesh[vox_mesh_floatcount++] = 0.0f;
    vox_mesh[vox_mesh_floatcount++] = 1.0f;
    vox_mesh[vox_mesh_floatcount++] = (float)type;

    vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // b
    vox_mesh[vox_mesh_floatcount++] = height;
    vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // b
    vox_mesh[vox_mesh_floatcount++] = 0.0f;
    vox_mesh[vox_mesh_floatcount++] = 0.0f;
    vox_mesh[vox_mesh_floatcount++] = (float)type;

    vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // c
    vox_mesh[vox_mesh_floatcount++] = height;
    vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // c
    vox_mesh[vox_mesh_floatcount++] = 1.0f;
    vox_mesh[vox_mesh_floatcount++] = 0.0f;
    vox_mesh[vox_mesh_floatcount++] = (float)type;

    vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // c
    vox_mesh[vox_mesh_floatcount++] = height;
    vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // c
    vox_mesh[vox_mesh_floatcount++] = 1.0f;
    vox_mesh[vox_mesh_floatcount++] = 0.0f;
    vox_mesh[vox_mesh_floatcount++] = (float)type;

    vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // d
    vox_mesh[vox_mesh_floatcount++] = height;
    vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // d
    vox_mesh[vox_mesh_floatcount++] = 1.0f;
    vox_mesh[vox_mesh_floatcount++] = 1.0f;
    vox_mesh[vox_mesh_floatcount++] = (float)type;

    vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // a
    vox_mesh[vox_mesh_floatcount++] = height;
    vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // a
    vox_mesh[vox_mesh_floatcount++] = 0.0f;
    vox_mesh[vox_mesh_floatcount++] = 1.0f;
    vox_mesh[vox_mesh_floatcount++] = (float)type;
    vox_mesh_vcount += 6;
  }

  // walls may draw sides if air is next door
  if ( !is_floor ) {
    /* wall facing to the west
     a--d   a : (x - 1/2, y - 1/2)
     |\ |   b : (x - 1/2, y - 1/2)
     | \|   c : (x - 1/2, y + 1/2)
     b--c   d : (x - 1/2, y + 1/2)
    */
    if ( _is_tile_in_map( x - 1, y ) && ( TILE_AIR == g_map.tiles[y][x - 1].type || _is_tile_a_floor( x - 1, y ) ) ) {
      vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // b
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // b
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // d
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // d
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;
      vox_mesh_vcount += 6;
    }

    /* wall facing to the east
     a--d   a : (x + 1/2, y + 1/2)
     |\ |   b : (x + 1/2, y + 1/2)
     | \|   c : (x + 1/2, y - 1/2)
     b--c   d : (x + 1/2, y - 1/2)
    */
    if ( _is_tile_in_map( x + 1, y ) && ( TILE_AIR == g_map.tiles[y][x + 1].type || _is_tile_a_floor( x + 1, y ) ) ) {
      vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // b
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // b
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // d
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // d
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;
      vox_mesh_vcount += 6;
    }

    /* wall facing to the north
     a--d   a : (x + 1/2, y - 1/2)
     |\ |   b : (x + 1/2, y - 1/2)
     | \|   c : (x - 1/2, y - 1/2)
     b--c   d : (x - 1/2, y - 1/2)
    */
    if ( _is_tile_in_map( x, y - 1 ) && ( TILE_AIR == g_map.tiles[y - 1][x].type || _is_tile_a_floor( x, y - 1 ) ) ) {
      vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // b
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // b
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // d
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // d
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y - 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;
      vox_mesh_vcount += 6;
    }

    /* wall facing to the south
     a--d   a : (x - 1/2, y + 1/2)
     |\ |   b : (x - 1/2, y + 1/2)
     | \|   c : (x + 1/2, y + 1/2)
     b--c   d : (x + 1/2, y + 1/2)
    */
    if ( _is_tile_in_map( x, y + 1 ) && ( TILE_AIR == g_map.tiles[y + 1][x].type || _is_tile_a_floor( x, y + 1 ) ) ) {
      vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // b
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // b
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // c
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x + 0.5f * VOXEL_DIMS; // d
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // d
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;

      vox_mesh[vox_mesh_floatcount++] = (float)x - 0.5f * VOXEL_DIMS; // a
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)y + 0.5f * VOXEL_DIMS; // a
      vox_mesh_vcount += 6;
      vox_mesh[vox_mesh_floatcount++] = 0.0f;
      vox_mesh[vox_mesh_floatcount++] = 1.0f;
      vox_mesh[vox_mesh_floatcount++] = (float)type;
    }
  }
}

void init_gl() {}

int main( int argc, char** argv ) {
  if ( argc < 3 ) {
    printf( "usage: ./voxelise INPUTFILE OUTPUTFILE\n" );
    return 0;
  }

  //////////////////////////////// voxelise and tessellate /////////////////////////////////////////
  {
    printf( "voxelising `%s` into `%s`...\n", argv[1], argv[2] );
    bool res = load_map_file( argv[1] );
    if ( !res ) { return 1; }
    for ( int y = 0; y < TILES_DOWN; y++ ) {
      for ( int x = 0; x < TILES_ACROSS; x++ ) { _voxelise_cube( x, y ); }
    }
    int fcount = vox_mesh_vcount / 3; // TODO use quads? TODO use indexed rendering?
    printf( "map voxelised: %i vertices\n", vox_mesh_vcount );

    FILE* outputf = fopen( argv[2], "w" );
    assert( outputf );
    {
      fprintf( outputf,
        "ply\nformat ascii 1.0\nelement vertex %i\nproperty float x\nproperty float y\nproperty float z\nproperty float "
        "s\nproperty float t\nproperty float red\nelement face %i\nproperty list uchar int vertex_indices\nend_header\n",
        vox_mesh_vcount, fcount );
      for ( int i = 0; i < vox_mesh_vcount; i++ ) {
        fprintf( outputf, "%f %f %f %f %f %f\n", vox_mesh[i * VOXEL_VERT_ATTRIB_COMPS], vox_mesh[i * VOXEL_VERT_ATTRIB_COMPS + 1],
          vox_mesh[i * VOXEL_VERT_ATTRIB_COMPS + 2], vox_mesh[i * VOXEL_VERT_ATTRIB_COMPS + 3], vox_mesh[i * VOXEL_VERT_ATTRIB_COMPS + 4],
          vox_mesh[i * VOXEL_VERT_ATTRIB_COMPS + 5] );
      }
      for ( int i = 0; i < fcount; i++ ) { fprintf( outputf, "3 %i %i %i\n", i * 3, i * 3 + 1, i * 3 + 2 ); }
    }
    fclose( outputf );
  }
  /////////////////////////////////// display stuff ////////////////////////////////////////////////
  {
    bool ret = start_gfx();
    if ( !ret ) {
      fprintf( stderr, "ERROR: starting gfx\n" );
      return 1;
    }

    // size_t sz   = 3 * 6 * sizeof( float );                                   // vox_mesh_vcount * VOXEL_VERT_ATTRIB_COMPS * sizeof( float );
    // mesh_t mesh = unit_cube_mesh();
    size_t sz   = vox_mesh_floatcount * sizeof( float );
    mesh_t mesh = create_mesh( vox_mesh, sz, MEM_POS_ST_R, vox_mesh_vcount, STATIC_DRAW ); // TODO is a dynamic draw better if modifying?
    texture_t textures[4];
    int ntextures = 4;
    textures[0]   = load_image_to_texture( "floor_grass.png" );
    textures[1]   = load_image_to_texture( "wall_stone.png" );
    textures[2]   = load_image_to_texture( "floor_water.png" );
    textures[3]   = load_image_to_texture( "wall_wood.png" );

    const char* vertex_shader =
      "#version 410\n"
      "in vec3 vp;"
      "in vec2 vt;"
      "in float vc;"
      "uniform mat4 PV;"
      "out float z;"
      "out vec2 st;"
      "flat out  int c;"
      "void main() {"
      "  gl_Position = PV * vec4(vp * 0.1, 1.0);"
      "  z = gl_Position.w;"
      "  st = vt;"
      "  c = int(vc) - 1;"
      "}";
    const char* fragment_shader =
      "#version 410\n"
      "in float z;"
      "in vec2 st;"
      "flat in int c;"
      "uniform sampler2D textures[4];"
      "out vec4 frag_colour;"
      "void main() {"
      "  vec4 texel = texture( textures[c], st );"
      "  frag_colour.rgb = texel.rgb * (1.0 - z * z * 0.02);"
      "}";
    shader_t shader = create_shader( vertex_shader, fragment_shader );

    mat4 P     = perspective( 66.0f, 800.0f / 600.0f, 0.1, 10.0 );
    mat4 V     = look_at( ( vec3 ){-0.5, 1.5, -0.5}, ( vec3 ){2, 0, 2}, ( vec3 ){0, 1, 0} );
    mat4 PV    = mult_mat4_mat4( P, V );
    int PV_loc = uniform_loc( shader, "PV" );
    uniform_mat4( shader, PV_loc, PV.m );

    for ( int i = 0; i < ntextures; i++ ) {
      char tmp[128];
      sprintf( tmp, "textures[%i]", i );
      int texmapb_loc = uniform_loc( shader, tmp );
      uniform_1i( shader, texmapb_loc, i );
    }

    // wireframe_mode();
    buffer_colour( 0.2, 0.2f, 0.2f, 1.0f );
    while ( !should_window_close() ) {
      clear_colour_and_depth_buffers();

      draw_mesh_texturedv( mesh, shader, textures, ntextures );

      swap_buffer_and_poll_events();
    }

    delete_shader( &shader );
    delete_mesh( &mesh );
    stop_gfx();
  }

  printf( "done\n" );
  return 0;
}
