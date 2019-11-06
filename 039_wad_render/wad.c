// WAD Rend - Copyright 2017 Anton Gerdelan <antonofnote@gmail.com>
// C99
#include "wad.h"
//#include "apg_data_structs.h"
#include "gl_utils.h"
#include "linmath.h"
#include <alloca.h> // malloc.h for win32?
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char wad_filename[1024];

typedef struct dir_entry_t {
  int addr;
  int sz;
  char name[9];
} dir_entry_t;

typedef struct linedef_t {
  int16_t start_vertex_idx;
  int16_t end_vertex_idx;
  uint16_t flags;
  int16_t special_type;
  int16_t sector_tag;    // which height to use if 2-sided?? TODO
  int16_t right_sidedef; // or -1
  int16_t left_sidedef;  // or -1
} linedef_t;

typedef struct sidedef_t {
  int16_t x_offset;
  int16_t y_offset;
  char upper_texture_name[9];
  char lower_textere_name[9];
  char middle_texture_name[9];
  int16_t sector;
} sidedef_t;

// storted in VERTEXES lump as sequence of raw xy pairs (16-bit ints)
typedef struct vertex_t { int16_t x, y; } vertex_t;

typedef struct sector_t {
  int16_t floor_height;
  int16_t ceil_height;
  char floor_texture_name[9]; // actually 8 + \0
  char ceil_texture_name[9];  // actually 8 + \0
  int16_t light_level;
  int16_t type;
  int16_t tag;                // corresp. to linedef tag
  int linedefs[128];          // i added this
  int nlinedefs;              // and this
  GLuint floor_vao, ceil_vao; // and this
  GLuint floor_vbo, ceil_vbo; // and this
  int nverts;                 // and this
  GLuint gl_floor_texture;    // and this
  GLuint gl_ceil_texture;     // and this
} sector_t;

static char wad_type[5];
static int ndir_entries;
static int dir_addr;
static dir_entry_t *dir_entries;
static int our_map_dir_idx = -1;
static int nlinedefs;
static linedef_t *linedefs;
static int nvertices;
static int nsidedefs;
static sidedef_t *sidedefs;
static vertex_t *vertices;
static int nsectors;
static sector_t *sectors;

int sector_count() { return nsectors; }

// custom strcmp to avoid commonly-made ==0 bracket soup bugs
// returns true if true so far and one string shorter e.g. "ANT" "ANTON"
bool apg_strmatchy( const char *a, const char *b ) {
  int len = MAX( strlen( a ), strlen( b ) );
  for ( int i = 0; i < len; i++ ) {
    if ( a[i] != b[i] ) {
      return false;
    }
  }
  return true;
}

// for convenience
typedef struct rgb_t { byte_t r, g, b; } rgb_t;

// for type safety
typedef enum pal_t {
  PAL_DEFAULT = 0,
  PAL_1,
  PAL_2,
  PAL_3,
  PAL_4,
  PAL_5,
  PAL_6,
  PAL_7,
  PAL_8,
  PAL_9,
  PAL_10,
  PAL_11,
  PAL_12,
  PAL_13,
  PAL_MAX
} pal_t;

byte_t *palettes[PAL_MAX];

// for convenience/safety
rgb_t rgb_from_palette( byte_t colour_idx, pal_t pal ) {
  assert( colour_idx < 256 && colour_idx >= 0 );
  assert( pal < PAL_MAX && pal >= PAL_DEFAULT );
  assert( palettes[pal] );

  int pal_idx = (int)pal;
  assert( palettes[pal_idx] );

  rgb_t rgb;
  rgb.r = palettes[pal_idx][colour_idx * 3];
  rgb.g = palettes[pal_idx][colour_idx * 3 + 1];
  rgb.b = palettes[pal_idx][colour_idx * 3 + 2];

  return rgb;
}

/* --> going to assume these are always in this order (DOOM registered v ~1993)
E1M1 THINGS LINEDEFS SIDEDEFS VERTEXES SEG
S SSECTORS NODES SECTORS REJECT BLOCKMAP E1M2

hexen has diff format: http://doom.wikia.com/wiki/Linedef

every linedef is
  - between 2 VERTICES
  - contains one or two SIDEDEFS (contain wall textures)
    - one sidedef is one-sided - void on other side
    - two sidedefs between sectors with any diff properties e.g. height, texture...
  - divide map into SECTORS
  - trigger action specials (action depends on linedef type number, object based on
tag e.g. sector number to affect except doors - always sector on other side of line)
*/

bool get_wad_picture_dims( const char *name, int *_width, int *_height ) {
  for ( int i = 0; i < ndir_entries; i++ ) {
    if ( apg_strmatchy( dir_entries[i].name, name ) ) {

      FILE *f = fopen( wad_filename, "rb" );
      if ( !f ) {
        fprintf( stderr, "ERROR: could not open file `%s`\n", wad_filename );
        return false;
      }

      // 8-byte HEADER of 4 short ints
      short int width = 0, height = 0, left_offset = 0, top_offset = 0;
      {
        int ret = fseek( f, (long)dir_entries[i].addr, SEEK_SET );
        assert( ret != -1 );
        size_t sz = sizeof( short int );
        size_t ritems = fread( &width, sz, 1, f );
        assert( ritems == 1 );
        ritems = fread( &height, sz, 1, f );
        assert( ritems == 1 );
        ritems = fread( &left_offset, sz, 1, f );
        assert( ritems == 1 );
        ritems = fread( &top_offset, sz, 1, f );
        assert( ritems == 1 );
      }

      fclose( f );

      *_width = (int)width;
      *_height = (int)height;
      return true;
    }
  }
  return false;
}

int get_wad_dir_index( const char *name ) {
  for ( int i = 0; i < ndir_entries; i++ ) {
    if ( apg_strmatchy( dir_entries[i].name, name ) ) {
      return i;
    }
  }
  return -1;
}

// allocate 4 bytes * width * height beforehand
bool load_picture_data( const char *lump_name, byte_t *pixel_buff, int width,
                        int height ) {

  // look up palette RGB for each bytes and draw with stb_image
  const size_t img_sz = 131072; // 256*256=65536
  byte_t *index_buff = alloca( img_sz );
  assert( index_buff );
  assert( pixel_buff );

  int dir_idx = get_wad_dir_index( lump_name );
  if ( dir_idx < 0 ) {
    fprintf( stderr, "ERROR: could find lump_name for image `%s` in WAD dir\n",
             lump_name );
    return false;
  }
  int lump_location = dir_entries[dir_idx].addr;

  { // read from WAD
    FILE *f = fopen( wad_filename, "rb" );
    if ( !f ) {
      fprintf( stderr, "ERROR: could not open file `%s`\n", wad_filename );
      return false;
    }

    size_t header_sz = 8;
    int ret = fseek( f, lump_location + header_sz, SEEK_SET );

    // ptrs to data for each column
    // i guess width*sizeof(int) block here
    // these are offsets of each column from first byte of picture lump
    int *column_offsets = (int *)calloc( width, 4 );
    assert( column_offsets );
    size_t ritems = fread( column_offsets, 4, width, f );
    assert( ritems == width );

    // columns of bytes
    
    for ( int col_idx = 0; col_idx < width; col_idx++ ) {
      int ptr = column_offsets[col_idx];
      int ret = fseek( f, (long)( ptr + lump_location ), SEEK_SET );
      assert( ret != -1 );

      // row start being non-zero allows columns to be drawn skipping
      // transparent bits at the top
      while ( true ) {
        byte_t row_start = 0;
        size_t ritems = fread( &row_start, 1, 1, f );
        if ( ritems != 1 ) {
          printf( "ERROR: not valid picture %s\n", lump_name );
          assert( 0 );
        }
        if ( row_start == 255 ) {
          break;
        }

        byte_t npixels_down = 0;
        ritems = fread( &npixels_down, 1, 1, f );
        assert( ritems == 1 );

        byte_t dead_byte = 0;
        ritems = fread( &dead_byte, 1, 1, f );
        assert( ritems == 1 );

        for ( int post_idx = 0; post_idx < npixels_down; post_idx++ ) {
          byte_t colour_idx = 0;
          ritems = fread( &colour_idx, 1, 1, f );
          assert( ritems == 1 );
          rgb_t rgb = rgb_from_palette( colour_idx, PAL_DEFAULT );
          int x = col_idx;
          int y = (int)row_start + post_idx;
          int pb_idx = width * y + x;

          if ( npixels_down > height ) {
            //printf( "npixels_down %i height %i\n", npixels_down, height );
            assert( 0 );
          }

          pixel_buff[pb_idx * 4] = rgb.r;
          pixel_buff[pb_idx * 4 + 1] = rgb.g;
          pixel_buff[pb_idx * 4 + 2] = rgb.b;
          pixel_buff[pb_idx * 4 + 3] = 255;
        } // endfor down posts
        ritems = fread( &dead_byte, 1, 1, f );
        assert( ritems == 1 );
      } // endwhile rowstart
    }   // endfor across columns
    fclose( f );

    free( column_offsets );
  }
  printf( "loaded img data `%s`\n", lump_name );
  return true;
}

// TODO if already loaded return same index
GLuint load_wad_texture( const char *texture_name ) {
  GLuint tex = 0;
  if ( apg_strmatchy( texture_name, "F_SKY1" ) ) {
    return 0;
  }
  // HACK workaround
  if ( apg_strmatchy( texture_name, "FLTLAVA1" ) ) {
    return tex;
  }

  // look up palette RGB for each bytes and draw with stb_image
  const size_t flat_sz = 4096;
  byte_t *index_buff = alloca( flat_sz );
  assert( index_buff );
  byte_t *pixel_buff = alloca( flat_sz * 3 ); // rgb
  assert( pixel_buff );

  int dir_idx = get_wad_dir_index( texture_name );
  if ( dir_idx < 0 ) {
    fprintf( stderr, "ERROR: could find texture for flat `%s` in WAD dir\n",
             texture_name );
    return tex;
  }

  { // read from WAD
    FILE *f = fopen( wad_filename, "rb" );
    if ( !f ) {
      fprintf( stderr, "ERROR: could not open file `%s`\n", wad_filename );
      return tex;
    }
    int ret = fseek( f, dir_entries[dir_idx].addr, SEEK_SET );
    assert( ret != -1 );
    // these are colour indices into the rgb palette
    size_t ritems = fread( index_buff, 1, flat_sz, f );
    if ( ritems != dir_entries[dir_idx].sz ) {
      printf( "%s\n", texture_name );
      assert( 0 );
    }
    fclose( f );
  }

  // use pixel in palette[0] (normal view) as o/p pixel RGB
  for ( int j = 0; j < flat_sz; j++ ) {
    rgb_t rgb = rgb_from_palette( index_buff[j], PAL_DEFAULT );
    pixel_buff[j * 3] = rgb.r;
    pixel_buff[j * 3 + 1] = rgb.g;
    pixel_buff[j * 3 + 2] = rgb.b;
  }
  {

    glActiveTexture( GL_TEXTURE0 );
    glGenTextures( 1, &tex );
    glBindTexture( GL_TEXTURE_2D, tex );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE,
                  pixel_buff );
    glGenerateMipmap( GL_TEXTURE_2D );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                     GL_LINEAR_MIPMAP_LINEAR );
    GLfloat max_aniso = 0.0f;
    glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso );
    // set the maximum!
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso );
  }

  printf( "loaded texture %s as handle %i\n", texture_name, tex );

  return tex;
}

bool open_wad( const char *filename, const char *map_name ) {
  FILE *f = fopen( filename, "rb" );
  if ( !f ) {
    fprintf( stderr, "ERROR: could not open file `%s`\n", filename );
    return false;
  }
  strcpy( wad_filename, filename );
  { // header
    fread( wad_type, 1, 4, f );
    printf( "%s\n", wad_type );
    fread( &ndir_entries, 4, 1, f );
    printf( "%i directory entries\n", ndir_entries );
    fread( &dir_addr, 4, 1, f );
    printf( "%i directory address\n", dir_addr );
    dir_entries = (dir_entry_t *)calloc( sizeof( dir_entry_t ), ndir_entries );
  }
  { // directory
    fseek( f, dir_addr, SEEK_SET );
    for ( int dir_idx = 0; dir_idx < ndir_entries; dir_idx++ ) {
      fread( &dir_entries[dir_idx].addr, 4, 1, f );
      fread( &dir_entries[dir_idx].sz, 4, 1, f );
      fread( &dir_entries[dir_idx].name, 1, 8, f );
      if ( apg_strmatchy( map_name, dir_entries[dir_idx].name ) ) {
        printf( "map found in directory at index %i\n", dir_idx );
        our_map_dir_idx = dir_idx;
      }
      // printf( "%s ", dir_entries[dir_idx].name );
    }
  }
  { // extract map stuff
    {
      int dir_idx = get_wad_dir_index( "PLAYPAL" );
      assert( dir_idx > -1 );
      int ret = fseek( f, (long)dir_entries[dir_idx].addr, SEEK_SET );

      assert( ret != -1 );
      // i think we can write them out as images
      for ( int palette_idx = 0; palette_idx < PAL_MAX; palette_idx++ ) {
        const size_t palette_sz = 768;
        palettes[palette_idx] = (byte_t *)malloc( palette_sz );
        assert( palettes[palette_idx] );

        size_t ritems = fread( palettes[palette_idx], 1, palette_sz, f );
        assert( ritems == palette_sz );
      }
    }
    { // THINGS
      ;
    }
    { // SECTORS -- do this first so can be populated by sidedefs later
      int sectors_lump_idx = our_map_dir_idx + 8;
      nsectors = dir_entries[sectors_lump_idx].sz / 26;
      printf( "%i sectors\n", nsectors );
      sectors = (sector_t *)calloc( sizeof( sector_t ), nsectors );
      fseek( f, dir_entries[sectors_lump_idx].addr, SEEK_SET );
      for ( int sidx = 0; sidx < nsectors; sidx++ ) {
        fread( &sectors[sidx].floor_height, 2, 1, f );
        fread( &sectors[sidx].ceil_height, 2, 1, f );
        fread( sectors[sidx].floor_texture_name, 1, 8, f );
        fread( sectors[sidx].ceil_texture_name, 1, 8, f );
        fread( &sectors[sidx].light_level, 2, 1, f );
        fread( &sectors[sidx].type, 2, 1, f );
        fread( &sectors[sidx].tag, 2, 1, f );

        // load the texture -- TODO dont reload the same textures
        // NOTE: F_SKY1 (F_SKY--1 in strife) is just 'use the sky texture for this
        // episode/level'
        // it could be SKY1 SKY2 SKY3 or SKY4 depending
        // these are a 128 units high _wall_ texture. top is pegged to top of
        // view window, zero column at due east. 256px wide and this tiles 4x
        // around e.g. due east->north=first 256px

        sectors[sidx].gl_floor_texture =
          load_wad_texture( sectors[sidx].floor_texture_name );
        sectors[sidx].gl_ceil_texture =
          load_wad_texture( sectors[sidx].ceil_texture_name );
      }
    }
    { // LINEDEFS
      int linedefs_lump_idx = our_map_dir_idx + 2;
      nlinedefs = dir_entries[linedefs_lump_idx].sz / 14;
      printf( "%i linedefs\n", nlinedefs );
      linedefs = (linedef_t *)calloc( sizeof( linedef_t ), nlinedefs );
      fseek( f, dir_entries[linedefs_lump_idx].addr, SEEK_SET );
      for ( int ldidx = 0; ldidx < nlinedefs; ldidx++ ) {
        fread( &linedefs[ldidx].start_vertex_idx, 2, 1, f );
        fread( &linedefs[ldidx].end_vertex_idx, 2, 1, f );
        fread( &linedefs[ldidx].flags, 2, 1, f );
        fread( &linedefs[ldidx].special_type, 2, 1, f );
        fread( &linedefs[ldidx].sector_tag, 2, 1, f );
        fread( &linedefs[ldidx].right_sidedef, 2, 1, f );
        fread( &linedefs[ldidx].left_sidedef, 2, 1, f );
      }
    }
    { // SIDEDEFS
      int sidedefs_lump_idx = our_map_dir_idx + 3;
      nsidedefs = dir_entries[sidedefs_lump_idx].sz / 30;
      printf( "%i sidedefs\n", nsidedefs );
      sidedefs = (sidedef_t *)calloc( sizeof( sidedef_t ), nsidedefs );
      fseek( f, dir_entries[sidedefs_lump_idx].addr, SEEK_SET );
      for ( int sdidx = 0; sdidx < nsidedefs; sdidx++ ) {
        fread( &sidedefs[sdidx].x_offset, 2, 1, f );
        fread( &sidedefs[sdidx].y_offset, 2, 1, f );
        fread( sidedefs[sdidx].upper_texture_name, 8, 1, f );
        fread( sidedefs[sdidx].lower_textere_name, 8, 1, f );
        fread( sidedefs[sdidx].middle_texture_name, 8, 1, f );
        fread( &sidedefs[sdidx].sector, 2, 1, f );
      }
    }
    { // VERTEXES
      int vertexes_lump_idx = our_map_dir_idx + 4;
      nvertices = dir_entries[vertexes_lump_idx].sz / sizeof( vertex_t );
      printf( "%i vertices\n", nvertices );
      vertices = (vertex_t *)calloc( sizeof( vertex_t ), nvertices );
      fseek( f, dir_entries[vertexes_lump_idx].addr, SEEK_SET );
      for ( int vidx = 0; vidx < nvertices; vidx++ ) {
        fread( &vertices[vidx].x, 2, 1, f );
        fread( &vertices[vidx].y, 2, 1, f );
      }
    }
    { // FLATS
    }

  } // endgeomblock
  fclose( f );
  return true;
}

// returns num bytes added
int fill_geom( float *geom_buff ) {
  int comp_idx = 0;
  // for each linedef, fetch x,y and x,y from both its verts
  // then extrude up into 2 triangles
  for ( int ldidx = 0; ldidx < nlinedefs; ldidx++ ) {
    int16_t start_vidx = linedefs[ldidx].start_vertex_idx;
    int16_t end_vidx = linedefs[ldidx].end_vertex_idx;
    int16_t start_x = vertices[start_vidx].x;
    int16_t start_y = -vertices[start_vidx].y;
    int16_t end_x = vertices[end_vidx].x;
    int16_t end_y = -vertices[end_vidx].y;
    int16_t left_sidedef = linedefs[ldidx].right_sidedef;
    int16_t right_sidedef = linedefs[ldidx].left_sidedef;
    int16_t left_sector = -1;
    int16_t right_sector = -1;
    int16_t left_ceil = 0;
    int16_t right_ceil = 0;
    int16_t left_floor = 0;
    int16_t right_floor = 0;
    if ( right_sidedef > -1 ) {
      right_sector = sidedefs[right_sidedef].sector;
      if ( right_sector > -1 ) {
        right_ceil = sectors[right_sector].ceil_height;
        right_floor = sectors[right_sector].floor_height;
        // tell the sector to use our line vertices
        sectors[right_sector].linedefs[sectors[right_sector].nlinedefs++] = ldidx;
      }
    }
    if ( left_sidedef > -1 ) {
      left_sector = sidedefs[left_sidedef].sector;
      if ( left_sector > -1 ) {
        left_ceil = sectors[left_sector].ceil_height;
        left_floor = sectors[left_sector].floor_height;
        // tell the sector to use our line vertices
        sectors[left_sector].linedefs[sectors[left_sector].nlinedefs++] = ldidx;
      }
    }
    // printf( "left floor %i left ceil %i\n", left_floor, left_ceil );
    if ( left_sidedef > -1 ) {
      vec3 a = ( vec3 ){ start_x, left_ceil, start_y };
      vec3 b = ( vec3 ){ start_x, left_floor, start_y };
      vec3 c = ( vec3 ){ end_x, left_floor, end_y };
      vec3 normal = normal_from_triangle( a, b, c );
      // two-sided - gets more complex
      if ( right_sidedef > -1 ) {
        // TODO if middle texture also...
        { ; }
        // a lip shows and we draw upper wall
        if ( right_ceil < left_ceil ) {
          int16_t top = left_ceil;
          int16_t bottom = right_ceil;
          geom_buff[comp_idx++] = (float)start_x;
          geom_buff[comp_idx++] = (float)bottom;
          geom_buff[comp_idx++] = (float)start_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)end_x;
          geom_buff[comp_idx++] = (float)bottom;
          geom_buff[comp_idx++] = (float)end_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)end_x;
          geom_buff[comp_idx++] = (float)top;
          geom_buff[comp_idx++] = (float)end_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)end_x;
          geom_buff[comp_idx++] = (float)top;
          geom_buff[comp_idx++] = (float)end_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)start_x;
          geom_buff[comp_idx++] = (float)top;
          geom_buff[comp_idx++] = (float)start_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)start_x;
          geom_buff[comp_idx++] = (float)bottom;
          geom_buff[comp_idx++] = (float)start_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
        }
        // a lip shows and we draw lower wall
        if ( right_floor > left_floor ) {
          int16_t top = right_floor;
          int16_t bottom = left_floor;
          geom_buff[comp_idx++] = (float)start_x;
          geom_buff[comp_idx++] = (float)bottom;
          geom_buff[comp_idx++] = (float)start_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)end_x;
          geom_buff[comp_idx++] = (float)bottom;
          geom_buff[comp_idx++] = (float)end_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)end_x;
          geom_buff[comp_idx++] = (float)top;
          geom_buff[comp_idx++] = (float)end_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)end_x;
          geom_buff[comp_idx++] = (float)top;
          geom_buff[comp_idx++] = (float)end_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)start_x;
          geom_buff[comp_idx++] = (float)top;
          geom_buff[comp_idx++] = (float)start_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)start_x;
          geom_buff[comp_idx++] = (float)bottom;
          geom_buff[comp_idx++] = (float)start_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
        }
        // regular one-piece wall with void behind
      } else {
        int16_t top = left_ceil;
        int16_t bottom = left_floor;
        geom_buff[comp_idx++] = (float)start_x;
        geom_buff[comp_idx++] = (float)bottom;
        geom_buff[comp_idx++] = (float)start_y;
        geom_buff[comp_idx++] = normal.x;
        geom_buff[comp_idx++] = normal.y;
        geom_buff[comp_idx++] = normal.z;
        geom_buff[comp_idx++] = (float)end_x;
        geom_buff[comp_idx++] = (float)bottom;
        geom_buff[comp_idx++] = (float)end_y;
        geom_buff[comp_idx++] = normal.x;
        geom_buff[comp_idx++] = normal.y;
        geom_buff[comp_idx++] = normal.z;
        geom_buff[comp_idx++] = (float)end_x;
        geom_buff[comp_idx++] = (float)top;
        geom_buff[comp_idx++] = (float)end_y;
        geom_buff[comp_idx++] = normal.x;
        geom_buff[comp_idx++] = normal.y;
        geom_buff[comp_idx++] = normal.z;
        geom_buff[comp_idx++] = (float)end_x;
        geom_buff[comp_idx++] = (float)top;
        geom_buff[comp_idx++] = (float)end_y;
        geom_buff[comp_idx++] = normal.x;
        geom_buff[comp_idx++] = normal.y;
        geom_buff[comp_idx++] = normal.z;
        geom_buff[comp_idx++] = (float)start_x;
        geom_buff[comp_idx++] = (float)top;
        geom_buff[comp_idx++] = (float)start_y;
        geom_buff[comp_idx++] = normal.x;
        geom_buff[comp_idx++] = normal.y;
        geom_buff[comp_idx++] = normal.z;
        geom_buff[comp_idx++] = (float)start_x;
        geom_buff[comp_idx++] = (float)bottom;
        geom_buff[comp_idx++] = (float)start_y;
        geom_buff[comp_idx++] = normal.x;
        geom_buff[comp_idx++] = normal.y;
        geom_buff[comp_idx++] = normal.z;
      }
    }

    if ( right_sidedef > -1 ) {
      vec3 a = ( vec3 ){ end_x, right_ceil, end_y };
      vec3 b = ( vec3 ){ end_x, right_floor, end_y };
      vec3 c = ( vec3 ){ start_x, right_floor, start_y };
      vec3 normal = normal_from_triangle( a, b, c );
      // two-sided with upper and/or lower lip and optional middle
      if ( left_sidedef > -1 ) {
        // TODO if middle texture also...
        { ; }
        if ( left_ceil < right_ceil ) {
          int16_t top = right_ceil;
          int16_t bottom = left_ceil;
          geom_buff[comp_idx++] = (float)start_x;
          geom_buff[comp_idx++] = (float)bottom;
          geom_buff[comp_idx++] = (float)start_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)start_x;
          geom_buff[comp_idx++] = (float)top;
          geom_buff[comp_idx++] = (float)start_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)end_x;
          geom_buff[comp_idx++] = (float)top;
          geom_buff[comp_idx++] = (float)end_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)end_x;
          geom_buff[comp_idx++] = (float)top;
          geom_buff[comp_idx++] = (float)end_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)end_x;
          geom_buff[comp_idx++] = (float)bottom;
          geom_buff[comp_idx++] = (float)end_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)start_x;
          geom_buff[comp_idx++] = (float)bottom;
          geom_buff[comp_idx++] = (float)start_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
        }
        if ( left_floor > right_floor ) {
          int16_t top = left_floor;
          int16_t bottom = right_floor;
          geom_buff[comp_idx++] = (float)start_x;
          geom_buff[comp_idx++] = (float)bottom;
          geom_buff[comp_idx++] = (float)start_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)start_x;
          geom_buff[comp_idx++] = (float)top;
          geom_buff[comp_idx++] = (float)start_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)end_x;
          geom_buff[comp_idx++] = (float)top;
          geom_buff[comp_idx++] = (float)end_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)end_x;
          geom_buff[comp_idx++] = (float)top;
          geom_buff[comp_idx++] = (float)end_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)end_x;
          geom_buff[comp_idx++] = (float)bottom;
          geom_buff[comp_idx++] = (float)end_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
          geom_buff[comp_idx++] = (float)start_x;
          geom_buff[comp_idx++] = (float)bottom;
          geom_buff[comp_idx++] = (float)start_y;
          geom_buff[comp_idx++] = normal.x;
          geom_buff[comp_idx++] = normal.y;
          geom_buff[comp_idx++] = normal.z;
        }
        // regular wall with void behind
      } else {
        int16_t top = right_ceil;
        int16_t bottom = right_floor;
        geom_buff[comp_idx++] = (float)start_x;
        geom_buff[comp_idx++] = (float)bottom;
        geom_buff[comp_idx++] = (float)start_y;
        geom_buff[comp_idx++] = normal.x;
        geom_buff[comp_idx++] = normal.y;
        geom_buff[comp_idx++] = normal.z;
        geom_buff[comp_idx++] = (float)start_x;
        geom_buff[comp_idx++] = (float)top;
        geom_buff[comp_idx++] = (float)start_y;
        geom_buff[comp_idx++] = normal.x;
        geom_buff[comp_idx++] = normal.y;
        geom_buff[comp_idx++] = normal.z;
        geom_buff[comp_idx++] = (float)end_x;
        geom_buff[comp_idx++] = (float)top;
        geom_buff[comp_idx++] = (float)end_y;
        geom_buff[comp_idx++] = normal.x;
        geom_buff[comp_idx++] = normal.y;
        geom_buff[comp_idx++] = normal.z;
        geom_buff[comp_idx++] = (float)end_x;
        geom_buff[comp_idx++] = (float)top;
        geom_buff[comp_idx++] = (float)end_y;
        geom_buff[comp_idx++] = normal.x;
        geom_buff[comp_idx++] = normal.y;
        geom_buff[comp_idx++] = normal.z;
        geom_buff[comp_idx++] = (float)end_x;
        geom_buff[comp_idx++] = (float)bottom;
        geom_buff[comp_idx++] = (float)end_y;
        geom_buff[comp_idx++] = normal.x;
        geom_buff[comp_idx++] = normal.y;
        geom_buff[comp_idx++] = normal.z;
        geom_buff[comp_idx++] = (float)start_x;
        geom_buff[comp_idx++] = (float)bottom;
        geom_buff[comp_idx++] = (float)start_y;
        geom_buff[comp_idx++] = normal.x;
        geom_buff[comp_idx++] = normal.y;
        geom_buff[comp_idx++] = normal.z;
      }
    }
  } // endfor linedefs
  printf( "%i bytes vertex buffer\n", comp_idx * 4 );
  return comp_idx * 4;
}

float sign( vec2 p1, vec2 p2, vec2 p3 ) {
  return ( p1.x - p3.x ) * ( p2.y - p3.y ) - ( p2.x - p3.x ) * ( p1.y - p3.y );
}

// this is using a Hessian normal form test -- page 176
bool PointInTriangle( vec2 pt, vec2 v1, vec2 v2, vec2 v3 ) {
  bool b1, b2, b3;

  b1 = sign( pt, v1, v2 ) < 0.0f;
  b2 = sign( pt, v2, v3 ) < 0.0f;
  b3 = sign( pt, v3, v1 ) < 0.0f;

  return ( ( b1 == b2 ) && ( b2 == b3 ) );
}

float floor_buffer[4096], ceil_buffer[4096];
int flat_points;
int flat_comps;

#define PHASE_B
#define PHASE_C
void earclip( int sector_idx ) {
  flat_comps = 0;
  flat_points = 0;
#ifdef PHASE_A
  { // phase 1 - just draw outlines. start with first edge
    for ( int i = 0; i < sectors[sector_idx].nlinedefs; i++ ) {
      int first_linedef_idx = sectors[sector_idx].linedefs[i];
      int first_vertex_idx = linedefs[first_linedef_idx].start_vertex_idx;
      int second_vertex_idx = linedefs[first_linedef_idx].end_vertex_idx;
      floor_buffer[flat_comps++] = (float)vertices[first_vertex_idx].x;
      floor_buffer[flat_comps++] = (float)sectors[sector_idx].floor_height;
      floor_buffer[flat_comps++] = -(float)vertices[first_vertex_idx].y;
      floor_buffer[flat_comps++] = (float)0;
      floor_buffer[flat_comps++] = (float)1;
      floor_buffer[flat_comps++] = (float)0;
      floor_buffer[flat_comps++] = (float)vertices[second_vertex_idx].x;
      floor_buffer[flat_comps++] = (float)sectors[sector_idx].floor_height;
      floor_buffer[flat_comps++] = -(float)vertices[second_vertex_idx].y;
      floor_buffer[flat_comps++] = (float)0;
      floor_buffer[flat_comps++] = (float)1;
      floor_buffer[flat_comps++] = (float)0;
      flat_points += 2;
    }
    sectors[sector_idx].nverts = sectors[sector_idx].nlinedefs * 2;
  }
#endif
#ifdef PHASE_B
  // build adjacent vertex list like this:
  // START - (END/START) - (END/START) -....
  // NOTE ASSUMPTION TO SIMPLIFY -- only 2 linedefs share a vert per sector
  int vert_adjacency[4096] = { -1 };
  int hole_adjacency[4096] = { -1 };
  // bool slinedef_added[1024] = { false };
  int nlinedefs_added = 0;
  int num_adjacency = 0;
  int nhole_adjacency = 0;
  { // phase 2 - order the vertices
    int first_linedef_idx = sectors[sector_idx].linedefs[0];
    int first_vertex_idx = linedefs[first_linedef_idx].start_vertex_idx;
    int next_vertex_idx = linedefs[first_linedef_idx].end_vertex_idx;

    // reverse if other-sided
    int right_sidedef_idx = linedefs[first_linedef_idx].right_sidedef;
    if ( right_sidedef_idx < 0 ||
         sidedefs[right_sidedef_idx].sector != sector_idx ) {
      first_vertex_idx = linedefs[first_linedef_idx].end_vertex_idx;
      next_vertex_idx = linedefs[first_linedef_idx].start_vertex_idx;
    }

    int last_vertex_idx = first_vertex_idx;
    vert_adjacency[num_adjacency++] = first_vertex_idx;
    //  slinedef_added[0] = true;
    nlinedefs_added++;
    int previous_ld = 0;
    int countdown = 1024;
    bool slinedef_claimed[2048] = { false };
    slinedef_claimed[0] = true;
    while ( nlinedefs_added < sectors[sector_idx].nlinedefs ) {
      if ( last_vertex_idx == next_vertex_idx ) {

        // hole finder -- E1M1 problems are not due to this chunk of code
        // can i assume just one hole and CUT INTO the original?
        if ( nlinedefs_added < sectors[sector_idx].nlinedefs - 2 ) {
          // find first unclaimed point to start forming hole
          for ( int i = 0; i < sectors[sector_idx].nlinedefs; i++ ) {
            if ( !slinedef_claimed[i] ) {
              int c_linedef_idx = sectors[sector_idx].linedefs[i];
              // note: should have reversed loop direction but sector on same side
              // e.h. right
              int first_vertex_idx = linedefs[c_linedef_idx].start_vertex_idx;
              next_vertex_idx = linedefs[c_linedef_idx].end_vertex_idx;

              int right_sidedef_idx = linedefs[c_linedef_idx].right_sidedef;
              if ( right_sidedef_idx < 0 ||
                   sidedefs[right_sidedef_idx].sector != sector_idx ) {
                first_vertex_idx = linedefs[c_linedef_idx].end_vertex_idx;
                next_vertex_idx = linedefs[c_linedef_idx].start_vertex_idx;
              }

              previous_ld = i;

              last_vertex_idx = first_vertex_idx;
              hole_adjacency[nhole_adjacency++] = first_vertex_idx;
              nlinedefs_added++;
              slinedef_claimed[i] = true;
              break;
            }
          }

          while ( nlinedefs_added < sectors[sector_idx].nlinedefs ) {
            // look for hole points to add
            for ( int i = 0; i < sectors[sector_idx].nlinedefs; i++ ) {
              if ( slinedef_claimed[i] ) {
                continue;
              }
              int linedef_idx = sectors[sector_idx].linedefs[i];
              int start_v_i = linedefs[linedef_idx].start_vertex_idx;
              int end_v_i = linedefs[linedef_idx].end_vertex_idx;
              if ( i == previous_ld ) {
                continue;
              }
              if ( start_v_i == next_vertex_idx ) {
                hole_adjacency[nhole_adjacency++] = start_v_i;
                next_vertex_idx = end_v_i;
                previous_ld = i;
                nlinedefs_added++;
                slinedef_claimed[i] = true;
                break;
              } else if ( end_v_i == next_vertex_idx ) {
                hole_adjacency[nhole_adjacency++] = end_v_i;
                next_vertex_idx = start_v_i;
                previous_ld = i;
                nlinedefs_added++;
                slinedef_claimed[i] = true;
                break;
              }
            } // endfor

            // append hole and joining line
            if ( last_vertex_idx == next_vertex_idx ) {
              printf( "=================before:==================\n" );
              for ( int i = 0; i < num_adjacency; i++ ) {
                printf( "%i,", vert_adjacency[i] );
              }
              printf( "\n" );

              for ( int i = 0; i < nhole_adjacency; i++ ) {
                printf( "%i,", hole_adjacency[i] );
              }
              printf( "\n" );

              // 1. find hole point with maximum x value
              int max_pt_idx = 0;
              int max_x_val = vertices[hole_adjacency[0]].x;
              int max_y_val = vertices[hole_adjacency[0]].y;
              for ( int i = 1; i < nhole_adjacency; i++ ) {
                if ( vertices[hole_adjacency[i]].x > max_x_val ) {
                  max_x_val = vertices[hole_adjacency[i]].x;
                  max_y_val = vertices[hole_adjacency[i]].y;
                  max_pt_idx = i;
                }
              }
              printf( "--max hole x point is %i with %i\n", max_pt_idx, max_x_val );

              // 2. CHEATING (should cast a ray and blah blah)
              int closest_pt_idx = 0;
              int xd = vertices[vert_adjacency[0]].x - max_x_val;
              int yd = vertices[vert_adjacency[0]].y - max_y_val;
              int closest_pt_dist = ( xd * xd + yd * yd );

              for ( int i = 1; i < num_adjacency; i++ ) {
                int xd = vertices[vert_adjacency[i]].x - max_x_val;
                int yd = vertices[vert_adjacency[i]].y - max_y_val;
                int dist = ( xd * xd + yd * yd );
                // printf( "candidate idx %i x %i dist %i\n", i, x, dist );
                if ( closest_pt_dist < 0 ||
                     ( dist > 0 && dist < closest_pt_dist ) ) {
                  closest_pt_dist = dist;
                  closest_pt_idx = i;
                }
              }

              // 3. shuffle then snake in
              // BACKUP
              int tmp_array[4096];
              int backup_length = 0;
              for ( int i = closest_pt_idx; i < num_adjacency; i++ ) {
                tmp_array[backup_length] = vert_adjacency[i];
                backup_length++;
              }

              // insert reversed hole at point AFTER closest_pt_idx
              for ( int i = 0; i < nhole_adjacency; i++ ) {
                int at = closest_pt_idx + 1;
                int hole_idx = loopmod( max_pt_idx + i, nhole_adjacency );
                vert_adjacency[at + i] = hole_adjacency[hole_idx];
              }
              num_adjacency += nhole_adjacency;

              // add start of hole segment again
              int at = closest_pt_idx + nhole_adjacency + 1;
              vert_adjacency[at] = hole_adjacency[max_pt_idx];
              num_adjacency++;
              // add rest or array (starting with duplicating earlier end)
              for ( int i = 0; i < backup_length; i++ ) {
                vert_adjacency[closest_pt_idx + nhole_adjacency + 2 + i] =
                  tmp_array[i];
              }
              num_adjacency = closest_pt_idx + nhole_adjacency + 2 + backup_length;

              printf( "after:\n" );
              for ( int i = 0; i < num_adjacency; i++ ) {
                printf( "%i,", vert_adjacency[i] );
              }
              printf( "\n" );

              break;
            }
            if ( nlinedefs_added == sectors[sector_idx].nlinedefs ) {
              printf( "WARNING: only %i pts added to hole - probably a mistake. "
                      "deleting\n",
                      nhole_adjacency );
              nhole_adjacency = 0;
            }
          } // endwhile adding hole pts

          // TODO find additional holes
          if ( nlinedefs_added < sectors[sector_idx].nlinedefs ) {
            int leftovers = sectors[sector_idx].nlinedefs - nlinedefs_added;
            printf( "WARNING: additional hole loop(s) (%i points) unclaimed in "
                    "sector %i\n",
                    leftovers, sector_idx );
          }
        } // end hole finder

        break;
      }
      countdown--;
      if ( countdown == 0 ) {
        break;
      }

      for ( int i = 0; i < sectors[sector_idx].nlinedefs; i++ ) {
        if ( slinedef_claimed[i] ) {
          continue;
        }
        int linedef_idx = sectors[sector_idx].linedefs[i];
        int start_v_i = linedefs[linedef_idx].start_vertex_idx;
        int end_v_i = linedefs[linedef_idx].end_vertex_idx;
        if ( i == previous_ld ) {
          continue;
        }

        if ( start_v_i == next_vertex_idx ) {
          vert_adjacency[num_adjacency++] = start_v_i;
          next_vertex_idx = end_v_i;
          previous_ld = i;
          nlinedefs_added++;
          slinedef_claimed[i] = true;
          break;
        } else if ( end_v_i == next_vertex_idx ) {
          vert_adjacency[num_adjacency++] = end_v_i;
          next_vertex_idx = start_v_i;
          previous_ld = i;
          nlinedefs_added++;
          slinedef_claimed[i] = true;
          break;
        }

      } // endfor
    }   // endwhile
  }     // endblock phase 2
#endif
#ifdef PHASE_C // find ears
  {
    flat_points = 0;
    flat_comps = 0;
    int remaining_verts = num_adjacency;

    vec2 *vlist = (vec2 *)malloc( sizeof( vec2 ) * num_adjacency );
    for ( int i = 0; i < remaining_verts; i++ ) {
      int vertex_idx = vert_adjacency[i];
      vlist[i].x = vertices[vertex_idx].x;
      vlist[i].y = vertices[vertex_idx].y;
    }
    // >2 rather than >3 makes this also add the last triangle
    while ( remaining_verts > 2 ) {
      int ear_idx = find_ear_in_simple_polygon( vlist, remaining_verts );
      if ( ear_idx == -1 ) {
        printf( "WARNING: no ear found - sector %i\n", sector_idx );
        break;
      }
      int prev_idx = loopmod( ear_idx - 1, remaining_verts );
      int next_idx = loopmod( ear_idx + 1, remaining_verts );
      // trim ear
      { // copy ear into buffer for triangles
        vec2 prev_vertex = vlist[prev_idx];
        vec2 next_vertex = vlist[next_idx];
        vec2 curr_vertex = vlist[ear_idx];
        // note i switch winding order to CCW here
        floor_buffer[flat_comps++] = next_vertex.x;
        floor_buffer[flat_comps++] = sectors[sector_idx].floor_height;
        floor_buffer[flat_comps++] = -next_vertex.y;
        floor_buffer[flat_comps++] = (float)0;
        floor_buffer[flat_comps++] = (float)1;
        floor_buffer[flat_comps++] = (float)0;
        flat_points++;
        floor_buffer[flat_comps++] = curr_vertex.x;
        floor_buffer[flat_comps++] = sectors[sector_idx].floor_height;
        floor_buffer[flat_comps++] = -curr_vertex.y;
        floor_buffer[flat_comps++] = (float)0;
        floor_buffer[flat_comps++] = (float)1;
        floor_buffer[flat_comps++] = (float)0;
        flat_points++;
        floor_buffer[flat_comps++] = prev_vertex.x;
        floor_buffer[flat_comps++] = sectors[sector_idx].floor_height;
        floor_buffer[flat_comps++] = -prev_vertex.y;
        floor_buffer[flat_comps++] = (float)0;
        floor_buffer[flat_comps++] = (float)1;
        floor_buffer[flat_comps++] = (float)0;
        flat_points++;
      }
      { // copy ear into buffer for triangles
        vec2 prev_vertex = vlist[prev_idx];
        vec2 next_vertex = vlist[next_idx];
        vec2 curr_vertex = vlist[ear_idx];
        // note i switch winding order to CCW here
        ceil_buffer[flat_comps++] = prev_vertex.x;
        ceil_buffer[flat_comps++] = sectors[sector_idx].ceil_height;
        ceil_buffer[flat_comps++] = -prev_vertex.y;
        ceil_buffer[flat_comps++] = (float)0;
        ceil_buffer[flat_comps++] = (float)1;
        ceil_buffer[flat_comps++] = (float)0;
        flat_points++;
        ceil_buffer[flat_comps++] = curr_vertex.x;
        ceil_buffer[flat_comps++] = sectors[sector_idx].ceil_height;
        ceil_buffer[flat_comps++] = -curr_vertex.y;
        ceil_buffer[flat_comps++] = (float)0;
        ceil_buffer[flat_comps++] = (float)1;
        ceil_buffer[flat_comps++] = (float)0;
        flat_points++;
        ceil_buffer[flat_comps++] = next_vertex.x;
        ceil_buffer[flat_comps++] = sectors[sector_idx].ceil_height;
        ceil_buffer[flat_comps++] = -next_vertex.y;
        ceil_buffer[flat_comps++] = (float)0;
        ceil_buffer[flat_comps++] = (float)1;
        ceil_buffer[flat_comps++] = (float)0;
        flat_points++;
      }
      { // shuffle array down
        for ( int i = ear_idx; i < remaining_verts - 1; i++ ) {
          vlist[i] = vlist[i + 1];
        }
        remaining_verts--;
      }
    }
    free( vlist );
  }
  sectors[sector_idx].nverts = flat_points;

#endif
  {
    glGenVertexArrays( 1, &sectors[sector_idx].floor_vao );
    glBindVertexArray( sectors[sector_idx].floor_vao );
    glGenBuffers( 1, &sectors[sector_idx].floor_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, sectors[sector_idx].floor_vbo );
    glBufferData( GL_ARRAY_BUFFER, 6 * flat_points * sizeof( GLfloat ),
                  floor_buffer, GL_STATIC_DRAW );
    GLintptr vertex_normal_offset = 3 * sizeof( float );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( float ), NULL );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( float ),
                           (GLvoid *)vertex_normal_offset );
    glEnableVertexAttribArray( 0 );
    glEnableVertexAttribArray( 1 );

    glGenVertexArrays( 1, &sectors[sector_idx].ceil_vao );
    glBindVertexArray( sectors[sector_idx].ceil_vao );
    glGenBuffers( 1, &sectors[sector_idx].ceil_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, sectors[sector_idx].ceil_vbo );
    glBufferData( GL_ARRAY_BUFFER, 6 * flat_points * sizeof( GLfloat ), ceil_buffer,
                  GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( float ), NULL );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof( float ),
                           (GLvoid *)vertex_normal_offset );
    glEnableVertexAttribArray( 0 );
    glEnableVertexAttribArray( 1 );
  }
}

// nsectors
void fill_sectors() {
  for ( int sectidx = 0; sectidx < nsectors; sectidx++ ) {
    earclip( sectidx );
  }
}

void draw_sectors( int verts, int sectidx ) {
  // printf( "sector_idx=%i (%i verts)\n", sectidx, sectors[sectidx].nverts );

  // glDisable( GL_CULL_FACE );

  for ( int sectidx = 0; sectidx < nsectors; sectidx++ ) {
    int usev = verts;
    if ( usev > sectors[sectidx].nverts ) {
      usev = sectors[sectidx].nverts;
    }

    // temp
    usev = sectors[sectidx].nverts;

    if ( sectors[sectidx].gl_floor_texture ) {
      glBindVertexArray( sectors[sectidx].floor_vao );

      glActiveTexture( GL_TEXTURE0 );
      glBindTexture( GL_TEXTURE_2D, sectors[sectidx].gl_floor_texture );

      glDrawArrays( GL_TRIANGLES, 0, usev );
    }
    if ( sectors[sectidx].gl_ceil_texture ) {
      glBindVertexArray( sectors[sectidx].ceil_vao );

      glActiveTexture( GL_TEXTURE0 );
      glBindTexture( GL_TEXTURE_2D, sectors[sectidx].gl_ceil_texture );
    }
    glDrawArrays( GL_TRIANGLES, 0, usev );
  }

  // glEnable( GL_CULL_FACE );
}
