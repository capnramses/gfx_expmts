/*
BMP File Reader/Writer Implementation
Licence: see apg_bmp.h
C99
*/

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APG_BMP_FILE_HDR_SZ 14
/* size of a minimal BITMAPINFOHEADER ( without bitmasks )
smallest DIB size is 12 but we always use some stuff from the size-40 version (bpp etc) -> 40 */
#define APG_BMP_MIN_DIB_HDR_SZ 40
/* smallest possible data section in bytes */
#define APG_BMP_MIN_DATA_WIDTH 4

#pragma pack( push, 1 ) // supported on GCC in addition to individual packing attribs
/* all BMP files, regardless of type, start with this file header */
struct bmp_file_header_t {
  char file_type[2];
  uint32_t file_sz;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t image_data_offset;
};

/* following the file header is the BMP type header. this is the most commonly used format */
struct bmp_dib_BITMAPINFOHEADER_t {
  uint32_t this_header_sz;
  int32_t w; // in older headers w & h these are shorts and may be unsigned
  int32_t h;
  uint16_t n_planes; // must be 1
  uint16_t bpp;
  uint32_t compression_method; // 16 and 32-bit images must have a value of 3 here
  uint32_t image_uncompressed_sz;
  int32_t horiz_pixels_per_meter;
  int32_t vert_pixels_per_meter;
  uint32_t n_colours_in_palette;
  uint32_t n_important_colours;
  /* NOTE(Anton) a DIB header may end here at 40-bytes. be careful using sizeof() */
  /* if 'compression' value, above, is set to 3 ie the image is 16 or 32-bit, then these colour channel masks follow the headers.
  these are big-endian order bit masks to assign bits of each pixel to different colours. bits used must be contiguous and not overlap. */
  uint32_t bitmask_r;
  uint32_t bitmask_g;
  uint32_t bitmask_b;
};
#pragma pack( pop )

enum bmp_compression_t { BI_RGB = 0, BI_RLE8, BI_RLE4, BI_BITFIELDS, BI_JPEG, BI_PNG, BI_ALPHABITFIELDS = 6, BI_CMYK = 11, BI_CMYKRLE8 = 12, BI_CMYRLE4 = 13 };

/* convenience struct and file->memory function */
struct entire_file_t {
  void* data;
  size_t sz;
};

/* this could be replaced with a project-specific method */
static bool _read_entire_file( const char* filename, struct entire_file_t* record ) {
  FILE* fp = fopen( filename, "rb" );
  if ( !fp ) { return false; }
  fseek( fp, 0L, SEEK_END );
  record->sz   = (size_t)ftell( fp );
  record->data = malloc( record->sz );
  if ( !record->data ) {
    fclose( fp );
    return false;
  }
  rewind( fp );
  size_t nr = fread( record->data, record->sz, 1, fp );
  fclose( fp );
  if ( 1 != nr ) { return false; } // TODO memleak
  return true;
}

/* NOTE(Anton) this could have ifdef branches on different compilers for the intrinsics versions for perf */
uint32_t bitscan( uint32_t dword ) {
  for ( uint32_t i = 0; i < 32; i++ ) {
    if ( 1 & dword ) { return i; }
    dword = dword >> 1;
  }
  return -1;
}

static bool _validate_bmp_file_header( struct bmp_file_header_t* file_header_ptr, struct entire_file_t record ) {
  if ( 'B' != file_header_ptr->file_type[0] || 'M' != file_header_ptr->file_type[1] ) { return false; }
  if ( file_header_ptr->image_data_offset + APG_BMP_MIN_DATA_WIDTH > record.sz ) { return false; }
  return true;
}

static bool _validate_bmp_dib_header( struct bmp_dib_BITMAPINFOHEADER_t* dib_header_ptr, struct entire_file_t record ) {
  if ( dib_header_ptr->this_header_sz + APG_BMP_FILE_HDR_SZ + APG_BMP_MIN_DATA_WIDTH > record.sz ) {}
  if ( 0 == dib_header_ptr->w || 0 == dib_header_ptr->h ) { return false; }
  if ( ( 32 == dib_header_ptr->bpp || 16 == dib_header_ptr->bpp ) &&
       ( BI_BITFIELDS != dib_header_ptr->compression_method && BI_ALPHABITFIELDS != dib_header_ptr->compression_method ) ) {
    return false;
  }
  if ( BI_RGB != dib_header_ptr->compression_method && BI_BITFIELDS != dib_header_ptr->compression_method && BI_ALPHABITFIELDS != dib_header_ptr->compression_method ) {
    return false;
  }
  // 8k * 16 seems like a reasonable 'hey this is probably an accident' size
  if ( 0 == dib_header_ptr->w || 0 == dib_header_ptr->h || abs( dib_header_ptr->w ) > 7182 * 10 || abs( dib_header_ptr->h ) > 7182 * 16 ) { return false; }
  /* NOTE(Anton) if images reliably used n_colours_in_palette we could have done a palette/file size integrity check here.
  because some always set 0 then we have to check every palette indexing as we read them */
  return true;
}

unsigned char* apg_read_bmp( const char* filename, int* w, int* h, int* n_chans, int vertical_flip ) {
  if ( !filename || !w || !h || !n_chans ) { return NULL; }

  bool vertically_flip = (bool)!vertical_flip;

  // read in the whole file into memory first - much faster than parsing on-the-fly
  struct entire_file_t record;
  if ( !_read_entire_file( filename, &record ) ) { return NULL; }
  if ( record.sz < APG_BMP_FILE_HDR_SZ + APG_BMP_MIN_DIB_HDR_SZ + APG_BMP_MIN_DATA_WIDTH ) {
    free( record.data );
    return NULL;
  }

  // grab and validate the first, file, header
  struct bmp_file_header_t* file_header_ptr = (struct bmp_file_header_t*)record.data;
  bool file_header_valid                    = _validate_bmp_file_header( file_header_ptr, record );
  if ( !file_header_valid ) {
    free( record.data );
    return NULL;
  }

  // grad and validate the second, DIB, header
  struct bmp_dib_BITMAPINFOHEADER_t* dib_header_ptr = (struct bmp_dib_BITMAPINFOHEADER_t*)( (uint8_t*)record.data + APG_BMP_FILE_HDR_SZ );
  bool dib_header_valid                             = _validate_bmp_dib_header( dib_header_ptr, record );
  if ( !dib_header_valid ) {
    free( record.data );
    return NULL;
  }

  // bitmaps can have negative dims to indicate the image should be flipped
  uint32_t width = *w = abs( dib_header_ptr->w );
  uint32_t height = *h = abs( dib_header_ptr->h );
  vertically_flip      = dib_header_ptr->h > 0 ? vertically_flip : !vertically_flip;

  uint32_t palette_offset = APG_BMP_FILE_HDR_SZ + dib_header_ptr->this_header_sz;
  bool has_bitmasks       = false;
  if ( BI_BITFIELDS == dib_header_ptr->compression_method || BI_ALPHABITFIELDS == dib_header_ptr->compression_method ) {
    has_bitmasks = true;
    palette_offset += 12;
  }
  if ( palette_offset > record.sz ) {
    free( record.data );
    return NULL;
  }
  uint8_t* palette_data_ptr = (uint8_t*)record.data + palette_offset;

  // channel count and palette are not well defined in the header so we make a good guess here
  bool has_palette = false;
  int n_src_chans  = 3;
  int n_dst_chans  = 3;

  switch ( dib_header_ptr->bpp ) {
  case 32: n_dst_chans = n_src_chans = 4; break; // technically can be RGB but not supported
  case 24: n_dst_chans = n_src_chans = 3; break; // technically can be RGBA but not supported
  case 8:                                        // seems to always use a BGR0 palette, even for greyscale
    n_dst_chans = 3;
    has_palette = true;
    n_src_chans = 1;
    break;
  case 4: // always has a palette - needed for an MS saved BMP
    n_dst_chans = 3;
    has_palette = true;
    n_src_chans = 1;
    break;
  case 1: // palette has 3 channels in it - monochrome not always black & white
    n_dst_chans = 3;
    has_palette = true;
    n_src_chans = 1;
    break;
  default: // this includes 2bpp and 16bpp
    free( record.data );
    return NULL;
  } // endswitch

  *n_chans = n_dst_chans;
  // NOTE(Anton) some image formats are not allowed a palette - could check for a bad header spec here also
  if ( dib_header_ptr->n_colours_in_palette > 0 ) { has_palette = true; }

  // work out if any padding how much to skip at end of each row
  uint32_t unpadded_row = width * n_src_chans;
  // bit-encoded palette indices have different padding properties
  if ( 4 == dib_header_ptr->bpp ) {
    unpadded_row = width % 2 > 0 ? width / 2 + 1 : width / 2; // find how many whole bytes required for this bit width
  }
  if ( 1 == dib_header_ptr->bpp ) {
    unpadded_row = width % 8 > 0 ? width / 8 + 1 : width / 8; // find how many whole bytes required for this bit width
  }
  uint32_t row_padding = 0 == unpadded_row % 4 ? 0 : 4 - ( unpadded_row % 4 ); // didn't expect operator precidence of - over %

  uint8_t* file_data_ptr  = (uint8_t*)record.data;
  uint8_t* src_pixels_ptr = &file_data_ptr[file_header_ptr->image_data_offset];
  uint32_t src_pixels_idx = 0;

  // another file size integrity check
  // 'image_data_offset' is by row padded to 4 bytes and is either colour data or palette indices.
  if ( file_header_ptr->image_data_offset + ( unpadded_row + row_padding ) * height > record.sz ) {
    free( record.data );
    return NULL;
  }

  // find which bit number each colour channel starts at, so we can separate colours out
  int bitshift_rgba[4] = { 0, 0, 0, 0 };
  uint32_t bitmask_a   = 0;
  if ( has_bitmasks ) {
    bitmask_a        = ~( dib_header_ptr->bitmask_r | dib_header_ptr->bitmask_g | dib_header_ptr->bitmask_b );
    bitshift_rgba[0] = bitscan( dib_header_ptr->bitmask_r );
    bitshift_rgba[1] = bitscan( dib_header_ptr->bitmask_g );
    bitshift_rgba[2] = bitscan( dib_header_ptr->bitmask_b );
    bitshift_rgba[3] = bitscan( bitmask_a );
    if ( bitshift_rgba[0] < 0 || bitshift_rgba[1] < 0 || bitshift_rgba[2] < 0 || bitshift_rgba[3] < 0 ) {
      free( record.data );
      return NULL;
    }
  }

  // allocate memory for the output pixels block
  uint8_t* dst_pixels_ptr = NULL;
  int dst_stride_sz       = width * n_dst_chans;
  size_t dst_pixels_sz    = dst_stride_sz * height;
  dst_pixels_ptr          = (uint8_t*)malloc( dst_pixels_sz );
  if ( !dst_pixels_ptr ) {
    free( record.data );
    return NULL;
  }

  int dst_pixels_idx = 0;
  for ( uint32_t r = 0; r < height; r++ ) {
    uint8_t bit_idx = 0; // used in monochrome
    dst_pixels_idx  = vertically_flip ? ( height - 1 - r ) * dst_stride_sz : r * dst_stride_sz;
    for ( uint32_t c = 0; c < width; c++ ) {
      //  == 32-bpp -> 32-bit RGBA. == 32-bit and 16-bit require bitmasks
      if ( 32 == dib_header_ptr->bpp ) {
        uint32_t pixel = *(uint32_t*)&src_pixels_ptr[src_pixels_idx]; // WARNING ASAN runtime error: load of misaligned address 0x6140000000ca for type 'uint32_t' (aka 'unsigned int'), which requires 4 byte alignment
        // NOTE(Anton) the below assumes 32-bits is always RGBA 1 byte per channel. 10,10,10 RGB exists though and isn't handled.
        dst_pixels_ptr[dst_pixels_idx++] = ( uint8_t )( ( pixel & dib_header_ptr->bitmask_r ) >> bitshift_rgba[0] );
        dst_pixels_ptr[dst_pixels_idx++] = ( uint8_t )( ( pixel & dib_header_ptr->bitmask_g ) >> bitshift_rgba[1] );
        dst_pixels_ptr[dst_pixels_idx++] = ( uint8_t )( ( pixel & dib_header_ptr->bitmask_b ) >> bitshift_rgba[2] );
        dst_pixels_ptr[dst_pixels_idx++] = ( uint8_t )( ( pixel & bitmask_a ) >> bitshift_rgba[3] );
        src_pixels_idx += n_src_chans;

        // == 8-bpp -> 24-bit RGB ==
      } else if ( 8 == dib_header_ptr->bpp && has_palette ) {
        // "most palettes are 4 bytes in RGB0 order but 3 for..." - it was actually BRG0 in old images -- Anton
        uint8_t index = src_pixels_ptr[src_pixels_idx]; // 8-bit index value per pixel

        if ( palette_offset + index * 4 + 2 >= record.sz ) {
          free( record.data );
          return dst_pixels_ptr;
        }
        dst_pixels_ptr[dst_pixels_idx++] = palette_data_ptr[index * 4 + 2];
        dst_pixels_ptr[dst_pixels_idx++] = palette_data_ptr[index * 4 + 1];
        dst_pixels_ptr[dst_pixels_idx++] = palette_data_ptr[index * 4 + 0];
        src_pixels_idx++;

        // == 4-bpp (16-colour) -> 24-bit RGB ==
      } else if ( 4 == dib_header_ptr->bpp && has_palette ) {
        // handle 2 pixels at a time
        uint8_t pixel_duo = src_pixels_ptr[src_pixels_idx];
        uint8_t a_index   = ( 0xFF & pixel_duo ) >> 4;
        uint8_t b_index   = 0xF & pixel_duo;

        if ( palette_offset + a_index * 4 + 2 >= record.sz ) {
          free( record.data );
          return dst_pixels_ptr;
        }
        dst_pixels_ptr[dst_pixels_idx++] = palette_data_ptr[a_index * 4 + 2];
        dst_pixels_ptr[dst_pixels_idx++] = palette_data_ptr[a_index * 4 + 1];
        dst_pixels_ptr[dst_pixels_idx++] = palette_data_ptr[a_index * 4 + 0];
        if ( ++c >= width ) { // advance a column
          c = 0;
          r++;
          dst_pixels_idx = ( height - 1 - r ) * dst_stride_sz;
        }

        if ( palette_offset + b_index * 4 + 2 >= record.sz ) {
          free( record.data );
          return dst_pixels_ptr;
        }
        dst_pixels_ptr[dst_pixels_idx++] = palette_data_ptr[b_index * 4 + 2];
        dst_pixels_ptr[dst_pixels_idx++] = palette_data_ptr[b_index * 4 + 1];
        dst_pixels_ptr[dst_pixels_idx++] = palette_data_ptr[b_index * 4 + 0];
        src_pixels_idx++;

        // == 1-bpp -> 24-bit RGB ==
      } else if ( 1 == dib_header_ptr->bpp && has_palette ) {
        /* encoding method for monochrome is not well documented.
        a 2x2 pixel image is stored as 4 1-bit palette indexes
        the palette is stored as any 2 RGB0 colours (not necessarily B&W)
        so for an image with indexes like so:
        1 1
        0 1
        it is bit-encoded as follows, starting at MSB:
        01000000 00000000 00000000 00000000 (first byte val  64)
        11000000 00000000 00000000 00000000 (first byte val 192)
        data is still split by row and each row padded to 4 byte multiples
         */
        if ( 8 == bit_idx ) { // start reading from the next byte
          src_pixels_idx++;
          bit_idx = 0;
        }
        uint8_t pixel_oct   = src_pixels_ptr[src_pixels_idx];
        uint8_t bit         = 128 >> bit_idx;
        uint8_t masked      = pixel_oct & bit;
        uint8_t palette_idx = masked > 0 ? 1 : 0;

        if ( palette_offset + palette_idx * 4 + 2 >= record.sz ) {
          free( record.data );
          return dst_pixels_ptr;
        }
        dst_pixels_ptr[dst_pixels_idx++] = palette_data_ptr[palette_idx * 4 + 2];
        dst_pixels_ptr[dst_pixels_idx++] = palette_data_ptr[palette_idx * 4 + 1];
        dst_pixels_ptr[dst_pixels_idx++] = palette_data_ptr[palette_idx * 4 + 0];
        bit_idx++;

        // == 24-bpp -> 24-bit RGB ==
      } else {
        // NOTE(Anton) this only supports 1 byte per channel
        if ( n_dst_chans > 3 ) { dst_pixels_ptr[dst_pixels_idx++] = src_pixels_ptr[src_pixels_idx + 3]; }
        if ( n_dst_chans > 2 ) { dst_pixels_ptr[dst_pixels_idx++] = src_pixels_ptr[src_pixels_idx + 2]; }
        if ( n_dst_chans > 1 ) { dst_pixels_ptr[dst_pixels_idx++] = src_pixels_ptr[src_pixels_idx + 1]; }
        dst_pixels_ptr[dst_pixels_idx++] = src_pixels_ptr[src_pixels_idx];
        src_pixels_idx += n_src_chans;
      }

    } // endfor col
    src_pixels_idx += row_padding;
    if ( 1 == dib_header_ptr->bpp ) { src_pixels_idx++; }
  } // endof row

  free( record.data );

  return dst_pixels_ptr;
}

int apg_write_bmp( const char* filename, const unsigned char* pixel_data, int w, int h, int n_chans ) {
  if ( !filename || !pixel_data || w <= 0 || h <= 0 || n_chans <= 0 ) { return 0; }

  struct bmp_file_header_t file_header;
  struct bmp_dib_BITMAPINFOHEADER_t dib_header;
  memset( &file_header, 0, APG_BMP_FILE_HDR_SZ );
  memset( &dib_header, 0, sizeof( struct bmp_dib_BITMAPINFOHEADER_t ) );

  // for simplicity, always write RGB or RGBA without using a palette
  int n_dst_chans = 4 == n_chans ? 4 : 3;

  uint32_t unpadded_row     = w * n_dst_chans;
  uint32_t row_padding      = 0 == unpadded_row % 4 ? 0 : 4 - unpadded_row % 4;
  uint32_t row_sz           = unpadded_row + row_padding;
  uint32_t pixels_padded_sz = row_sz * h;

  dib_header.this_header_sz = 40; // for BITMAPINFOHEADER
  if ( 4 == n_dst_chans ) { dib_header.compression_method = BI_BITFIELDS; }
  dib_header.w        = w;
  dib_header.h        = h;
  dib_header.n_planes = 1; // must be 1
  dib_header.bpp      = n_dst_chans * 8;

  file_header.file_type[0]      = 'B';
  file_header.file_type[1]      = 'M';
  file_header.image_data_offset = APG_BMP_FILE_HDR_SZ + sizeof( struct bmp_dib_BITMAPINFOHEADER_t );
  file_header.file_sz           = file_header.image_data_offset + pixels_padded_sz;

  // big-endian masks. only used in BI_BITFIELDS and BI_ALPHABITFIELDS ( 16 and 32-bit images )
  // important note: GIMP stores BMP data in this array order for 32-bit: [A][B][G][R]
  dib_header.bitmask_r = 0xFF000000;
  dib_header.bitmask_g = 0x00FF0000;
  dib_header.bitmask_b = 0x0000FF00;

  uint8_t* output_pixels = (uint8_t*)malloc( pixels_padded_sz );
  if ( !output_pixels ) { return 0; }
  {
    uint8_t* curr_ptr     = output_pixels;
    uint32_t unpadded_row = w * n_dst_chans;
    uint32_t row_padding  = 0 == unpadded_row % 4 ? 0 : 4 - unpadded_row % 4;
    uint8_t padding[4]    = { 0, 0, 0, 0 };
    uint8_t rgba[4]       = { 0, 0, 0, 0 };
    uint8_t bgra[4]       = { 0, 0, 0, 0 };

    for ( int r = 0; r < h; r++ ) {
      int pix_idx = ( h - 1 - r ) * n_chans * w;
      for ( int c = 0; c < w; c++ ) {
        for ( int i = 0; i < n_chans; i++ ) { rgba[i] = pixel_data[pix_idx++]; }
        if ( 1 == n_chans ) {
          bgra[0] = bgra[1] = bgra[2] = rgba[0];
        } else if ( 2 == n_chans ) {
          bgra[0] = 0;
          bgra[1] = rgba[1];
          bgra[2] = rgba[0];
        } else if ( 3 == n_dst_chans ) {
          bgra[0] = rgba[2];
          bgra[1] = rgba[1];
          bgra[2] = rgba[0];
        } else {
          /* TODO(Anton) RGBA with alpha channel would be better supported with an extended DIB header */
          bgra[0] = rgba[3];
          bgra[1] = rgba[2];
          bgra[2] = rgba[1];
          bgra[3] = rgba[0]; // alpha
        }
        memcpy( curr_ptr, bgra, n_dst_chans ); // warning C6386: Buffer overrun while writing to 'curr_ptr':  the writable size is 'pixels_padded_sz' bytes, but '3' bytes might be written.
        curr_ptr += n_dst_chans;
      }
      if ( row_padding > 0 ) {
        memcpy( curr_ptr, padding, row_padding );
        curr_ptr += row_padding;
      }
    }
  }
  {
    FILE* fp = fopen( filename, "wb" );
    if ( !fp ) {
      free( output_pixels );
      return 0;
    }

    fwrite( &file_header, APG_BMP_FILE_HDR_SZ, 1, fp );
    fwrite( &dib_header, sizeof( struct bmp_dib_BITMAPINFOHEADER_t ), 1, fp );
    fwrite( output_pixels, pixels_padded_sz, 1, fp );
    fclose( fp );
  }
  free( output_pixels );

  return 1;
}
