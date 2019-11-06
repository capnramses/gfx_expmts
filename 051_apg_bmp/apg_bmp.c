// BMP File Reader implementation
// Copyright Dr Anton Gerdelan <antongdl@protonmail.com> 2019
// C99

#include "apg_bmp.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* BMP file structure:
1. BMP Header
2. DIB (info) header
3. Colour Table (semi-optional)
4. GAP1 (optional)
5. Pixel data array
6. GAP2 (optional)
7. ICC Color Profile or path to file (only with BITMAPV5HEADER)
*/

// contents of 14-byte mandatory header at top of file
struct bmp_file_header_t {
  char file_type[2];
  uint32_t file_sz;
  uint32_t offset_of_pixel_data;
};

enum dib_header_types_t {
  BITMAPCOREHEADER          = 0, // Windows 2.0 or later. Width and Height are signed integers.
  OS21XBITMAPHEADER         = 1, // OS/2 1.x. Width and Height are unsigned integers. OS/2 1.x bitmaps are uncompressed and cannot be 16 or 32 bpp.
  OS22XBITMAPHEADER         = 2, // Adds halftoning. Adds RLE and Huffman 1D compression.
  OS22XBITMAPHEADER_VARIANT = 3, // This variant of the previous header contains only the first 16 bytes and the remaining bytes are assumed to be zero values.
  BITMAPINFOHEADER          = 4, // Windows NT, 3.1x or later. Adds 16 bpp and 32 bpp formats. Adds RLE compression.
  BITMAPV2INFOHEADER        = 5, // Adds RGB bit masks. Undocumented. ( Photoshop )
  BITMAPV3INFOHEADER        = 6, // Adds alpha channel bit mask. Not officially documented. ( Photoshop )
  BITMAPV4HEADER            = 7, // Windows NT 4.0, 95 or later. Adds color space type and gamma correction.
  BITMAPV5HEADER            = 8, // Windows NT 5.0, 98 or later. Adds ICC color profiles.
  HEADER_NAMES_MAX
};

// matches dib_header_types_t enum to go from size to type or vice versa
const int header_szs[] = { 12, 12, 64, 16, 40, 52, 56, 108, 124 };

enum compression_methods_t {
  BI_RGB                   = 0,  // no compression. most common.
  BI_RLE8                  = 1,  // RLE 8-bit/pixel
  BI_RLE4                  = 2,  // RLE 4-bit/pixel
  BI_BITFIELDS             = 3,  // Huffman 1D
  BI_JPEG                  = 4,  // RLE-24
  BI_PNG                   = 5,  //
  BI_ALPHABITFIELDS        = 6,  // RGBA bit field masks
  COMPRESSION_UNDEFINED_7  = 7,  //
  COMPRESSION_UNDEFINED_8  = 8,  //
  COMPRESSION_UNDEFINED_9  = 9,  //
  COMPRESSION_UNDEFINED_10 = 10, //
  BI_CMYK                  = 11, // none.  Only Windows metafile CMYK.
  BI_CMYKRLE8              = 12, // RLE-8. Only Windows metafile CMYK.
  BI_CMYKRLE4              = 13, // RLE-4. Only Windows metafile CMYK.
  COMPRESSION_METHODS_MAX
};

// contents of info header
struct dib_header_t {
  enum dib_header_types_t type; // can use to index into header_szs to get size in bytes
  int32_t w, h;                 // BITMAPINFOHEADER uses signed 4-byte integers
  uint16_t n_planes, bpp;
  // fields below from BITMAPINFOHEADER
  enum compression_methods_t compression_method;
  uint32_t image_uncompressed_sz;                         // 0 can be a dummy value for BI_RGB (raw) bitmaps
  uint32_t horiz_pixels_per_meter, vert_pixels_per_meter; // "resolution"
  uint32_t n_palette_colours;                             // 0 defaults to 2^n.
  uint32_t n_important_colours_used;                      // 0 means every colour important. generally ignored.
};

static bool _read_file_header( struct bmp_file_header_t* header, FILE* fp ) {
  assert( header && fp );

  // printf( "File Header:\n" );

  // read first 2 bytes to identify BMP/DIB file
  if ( 2 != fread( header->file_type, 1, 2, fp ) ) { return false; }
  if ( 'B' != header->file_type[0] || 'M' != header->file_type[1] ) {
    fprintf( stderr, "ERROR: File type is unhandled type %c%c\n", header->file_type[0], header->file_type[1] );
    return false;
  }
  // read size of BMP file in bytes. note: could add an internal test of this versus ftell() file sz
  if ( 1 != fread( &header->file_sz, 4, 1, fp ) ) { return false; }
  // printf( "- file size is %u bytes\n", header->file_sz );
  // skip past app-reserved 4 bytes and read the offset bytes
  fseek( fp, 4, SEEK_CUR ); // could also use fsetpos() here
  if ( 1 != fread( &header->offset_of_pixel_data, 4, 1, fp ) ) { return false; }
  // printf( "- pixel data array is at byte %u\n", header->offset_of_pixel_data );

  return true;
}

static bool _read_DIB_header( struct dib_header_t* dib_header, FILE* fp ) {
  assert( dib_header && fp );

  uint32_t header_size = 0;

  // printf( "DIB Header:\n" );
  { // read the header size/type
    size_t n = fread( &header_size, 4, 1, fp );
    if ( n != 1 ) {
      fprintf( stderr, "ERROR: Header DIB header_size size too small %i\n", (int)n );
      return false;
    }
    // work out which spec to use based on the size of the header
    bool matched = false;
    for ( int i = 0; i < HEADER_NAMES_MAX; i++ ) {
      if ( header_szs[i] == header_size ) {
        dib_header->type = (enum dib_header_types_t)i;
        matched          = true;
        //   printf( "- type is %i - %u bytes\n", i, header_size );
        break;
      }
    }
    if ( !matched ) {
      fprintf( stderr, "ERROR: BMP DIB header type matching size %u bytes is not defined\n", header_size );
      return false;
    }
  }

  // based on type of header all the sizes, signed/unsigned types and number ( but not order of ) fields differ.
  if ( BITMAPINFOHEADER == dib_header->type ) {
    { // read width, height, number of colour planes, and bpp
      // NOTE(Anton) other formats use 2-byte signed or unsigned ints for w,h
      if ( 1 != fread( &dib_header->w, 4, 1, fp ) ) { return false; }
      if ( 1 != fread( &dib_header->h, 4, 1, fp ) ) { return false; }
      if ( 1 != fread( &dib_header->n_planes, 2, 1, fp ) ) { return false; }
      if ( 1 != fread( &dib_header->bpp, 2, 1, fp ) ) { return false; }
      //  printf( "- image is %ux%u @%u bpp with %u colour planes\n", dib_header->w, dib_header->h, dib_header->bpp, dib_header->n_planes );
    }
    { // read additional BITMAPINFOHEADER info - compression, sizes, dims
      int compression_method = -1;
      size_t n               = fread( &compression_method, 4, 1, fp );
      if ( n != 1 ) { return false; }
      if ( compression_method < 0 || compression_method >= COMPRESSION_METHODS_MAX ) { return false; }
      if ( compression_method >= COMPRESSION_UNDEFINED_7 && compression_method <= COMPRESSION_UNDEFINED_10 ) { return false; }
      dib_header->compression_method = (enum compression_methods_t)compression_method;
      if ( 1 != fread( &dib_header->image_uncompressed_sz, 4, 1, fp ) ) { return false; }
      if ( 1 != fread( &dib_header->horiz_pixels_per_meter, 4, 1, fp ) ) { return false; }
      if ( 1 != fread( &dib_header->vert_pixels_per_meter, 4, 1, fp ) ) { return false; }
      if ( 1 != fread( &dib_header->n_palette_colours, 4, 1, fp ) ) { return false; }
      if ( 1 != fread( &dib_header->n_important_colours_used, 4, 1, fp ) ) { return false; }
      //  printf( "- compression method %i\n", compression_method );
      //  printf( "- pixels_per_meter %ix%i\n", dib_header->horiz_pixels_per_meter, dib_header->vert_pixels_per_meter );
      //  printf( "- n_palette_colours %i\n", dib_header->n_palette_colours );
      // printf( "- n_important_colours_used %i\n", dib_header->n_important_colours_used );
    }
  } else {
    fprintf( stderr, "ERROR: header type %i (size %u) is currently unsupported\n", (int)dib_header->type, header_size );
    return false;
  }

  return true;
}

static bool _read_pixels( FILE* fp, unsigned char* pixel_data, int w, int h, int n_chans ) {
  assert( fp && w > 0 && h > 0 && n_chans > 0 );

  // printf( "Read Pixels:\n" );

  int n_cols       = abs( w );
  int n_rows       = abs( h ); // because image height can be negative
  int unpadded_row = n_cols * n_chans;
  int row_padding  = 0 == unpadded_row % 4 ? 0 : 4 - unpadded_row % 4;
  int row_sz       = unpadded_row + row_padding;
  // printf( "- n rows = %i\n", n_rows );
  // printf( "- unpadded row sz = %i\n", unpadded_row );
  // printf( "- row padding = %i\n", row_padding );

  // data stored by row and BGR order and end-padded to make rows a multiple of 4 bytes.
  // this code reads and copies into an unpadded RGB buffer.
  // most formats are stored from bottom row to top row ( so need to flip )
  uint8_t bgra[4] = { 0, 0, 0, 0 };
  for ( int r = 0; r < n_rows; r++ ) {
    size_t i = ( n_rows - 1 - r ) * unpadded_row; // this vertically flips the image as it is read
    for ( int c = 0; c < n_cols; c++ ) {
      // NOTE(Anton) this only supports 1 byte per channel
      if ( n_chans != fread( bgra, 1, n_chans, fp ) ) { return false; }
      if ( n_chans > 2 ) { pixel_data[i++] = bgra[2]; }
      if ( n_chans > 1 ) { pixel_data[i++] = bgra[1]; }
      pixel_data[i++] = bgra[0];
      if ( n_chans > 3 ) { pixel_data[i++] = bgra[3]; } // Alpha at end of both
    }
    // seek padding
    fseek( fp, row_padding, SEEK_CUR );
  }
  return true;
}

unsigned char* apg_read_bmp( const char* filename, int* w, int* h, int* n ) {
  assert( filename );
  assert( w && h && n );

  struct bmp_file_header_t header;
  struct dib_header_t dib_header;
  memset( &header, 0, sizeof( struct bmp_file_header_t ) );
  memset( &dib_header, 0, sizeof( struct dib_header_t ) );

  FILE* fp = fopen( filename, "rb" );
  if ( !fp ) {
    fprintf( stderr, "ERROR: could not open file for reading\n" );
    return NULL;
  }

  // Section (1/7) File Header
  if ( !_read_file_header( &header, fp ) ) {
    fprintf( stderr, "ERROR reading file header\n" );
    fclose( fp );
    return NULL;
  }
  // Section (2/7) DIB Header
  if ( !_read_DIB_header( &dib_header, fp ) ) {
    fprintf( stderr, "ERROR reading DIB header\n" );
    fclose( fp );
    return NULL;
  }
  *w = dib_header.w;
  *h = dib_header.h;
  // clang-format off
	switch ( dib_header.bpp ) {
		case 1: { *n = 1; } break;
		case 4: { *n = 1; } break;
		case 8: { *n = 1; } break;
		case 16: { *n = 2; } break;
		case 24: { *n = 3; } break; // this is from a BGR pixel store with 1 byte per channel
		case 32: { *n = 4; } break;
		default: {
			fprintf( stderr, "ERROR: invalid bpp of %i\n", dib_header.bpp );
			fclose( fp );
			return NULL;
		} break;
	}
  // clang-format on

  // Section (3/7) ~optional Colour Table. for colour depths <= 8 bpp a colour table must be scanned here
  // Section (4/7) optional GAP1

  // Section (5/7) Pixel Array
  if ( dib_header.bpp != 24 || (int)dib_header.compression_method != 0 ) {
    fprintf( stderr, "ERROR: only 24 bpp uncompressed images are currently supported\n" );
    fclose( fp );
    return NULL;
  }

  size_t buffer_sz          = dib_header.w * dib_header.h * *n;
  unsigned char* pixel_data = (unsigned char*)malloc( buffer_sz );
  assert( pixel_data );

  if ( !_read_pixels( fp, pixel_data, *w, *h, *n ) ) {
    free( pixel_data );
    fclose( fp );
    return NULL;
  }

  // Section (6/7) optional GAP2
  // Section (7/7) optional ICC color profile

  fclose( fp );
  return pixel_data; // TODO(Anton) return pixel_data sz
}

static bool _write_file_header( int w, int h, int n_chans, FILE* fp ) {
  uint32_t unpadded_row        = w * n_chans;
  uint32_t row_padding         = 0 == unpadded_row % 4 ? 0 : 4 - unpadded_row % 4;
  uint32_t row_sz              = unpadded_row + row_padding;
  uint32_t pixels_padded_sz    = row_sz * h;
  uint32_t file_header_sz      = 14; // fixed
  const uint32_t dib_header_sz = 40; // for BITMAPINFOHEADER
  // the following assumes no colour table, gaps, or other optional stuff
  uint32_t file_sz    = file_header_sz + dib_header_sz + pixels_padded_sz;
  uint32_t pix_offset = file_header_sz + dib_header_sz;

  // printf( "Writing file sz %u\n", file_sz );

  char file_type[2] = { 'B', 'M' };
  char res1[2]      = { 0, 0 };
  if ( 2 != fwrite( file_type, 1, 2, fp ) ) { return false; }
  if ( 1 != fwrite( &file_sz, 4, 1, fp ) ) { return false; }
  if ( 1 != fwrite( res1, 2, 1, fp ) ) { return false; }
  if ( 1 != fwrite( res1, 2, 1, fp ) ) { return false; }
  if ( 1 != fwrite( &pix_offset, 4, 1, fp ) ) { return false; }
  return true;
}

static bool _write_dib_header( int w, int h, int n_chans, FILE* fp ) {
  const uint32_t dib_header_sz = 40; // for BITMAPINFOHEADER
  uint32_t pixels_raw_sz       = w * h * n_chans;
  if ( 1 != fwrite( &dib_header_sz, 4, 1, fp ) ) { return false; }
  if ( 1 != fwrite( &w, 4, 1, fp ) ) { return false; }
  if ( 1 != fwrite( &h, 4, 1, fp ) ) { return false; }
  uint16_t n_planes = 1;
  if ( 1 != fwrite( &n_planes, 2, 1, fp ) ) { return false; }
  uint16_t bpp = n_chans * 8;
  if ( 1 != fwrite( &bpp, 2, 1, fp ) ) { return false; }
  uint32_t compression = 0;
  if ( 1 != fwrite( &compression, 4, 1, fp ) ) { return false; }
  if ( 1 != fwrite( &pixels_raw_sz, 4, 1, fp ) ) { return false; }
  // I have no idea where this comes from or what it's used for. Just copied GIMP's output.
  uint32_t resolution = 2835;
  if ( 1 != fwrite( &resolution, 4, 1, fp ) ) { return false; } // horiz
  if ( 1 != fwrite( &resolution, 4, 1, fp ) ) { return false; } // vert
  uint32_t n_palette_colours = 0, n_important_colours = 0;
  if ( 1 != fwrite( &n_palette_colours, 4, 1, fp ) ) { return false; }
  if ( 1 != fwrite( &n_important_colours, 4, 1, fp ) ) { return false; }
  return true;
}

static bool _write_pixel_section( const unsigned char* pixel_data, int w, int h, int n_chans, FILE* fp ) {
  uint32_t unpadded_row = w * n_chans;
  uint32_t row_padding  = 0 == unpadded_row % 4 ? 0 : 4 - unpadded_row % 4;
  // printf( "row padding is %u\n", row_padding );
  char padding[4] = { 0, 0, 0, 0 };
  char rgba[4]    = { 0, 0, 0, 0 };
  char bgra[4]    = { 0, 0, 0, 0 };

  for ( int r = 0; r < h; r++ ) {
    int pix_idx = ( h - 1 - r ) * n_chans * w;
    for ( int c = 0; c < w; c++ ) {
      for ( int i = 0; i < n_chans; i++ ) { rgba[i] = pixel_data[pix_idx++]; }
      bgra[0] = rgba[2];
      bgra[1] = rgba[1];
      bgra[2] = rgba[0];
      bgra[3] = rgba[3]; // alpha
      if ( 1 != fwrite( bgra, n_chans, 1, fp ) ) { return false; }
    }
    if ( row_padding > 0 ) {
      if ( 1 != fwrite( padding, row_padding, 1, fp ) ) { return false; }
    }
  }

  return true;
}

int apg_write_bmp( const char* filename, const unsigned char* pixel_data, int w, int h, int n_chans ) {
  assert( filename && pixel_data && w > 0 && h > 0 && n_chans > 0 );

  FILE* fp = fopen( filename, "wb" );
  if ( !fp ) {
    fprintf( stderr, "ERROR: could not open file `%s` for writing\n", filename );
    return 0;
  }
  // Section (1/7) File Header
  if ( !_write_file_header( w, h, n_chans, fp ) ) {
    fclose( fp );
    return 0;
  }
  // Section (2/7) DIB Header
  if ( !_write_dib_header( w, h, n_chans, fp ) ) {
    fclose( fp );
    return 0;
  }
  // Section (3/7) ~optional Colour Table. for colour depths <= 8 bpp a colour table must be scanned here
  // Section (4/7) optional GAP1
  // Section (5/7) Pixel Array
  if ( !_write_pixel_section( pixel_data, w, h, n_chans, fp ) ) {
    fclose( fp );
    return 0;
  }
  // Section (6/7) optional GAP2
  // Section (7/7) optional ICC color profile

  fclose( fp );
  return 1;
}
