#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define APG_PDF_OBJ_LIST_MAX 256
#define APG_PDF_LINE_LEN_MAX 1024

typedef struct apg_pdf_obj_t {
  char type_str[APG_PDF_LINE_LEN_MAX];
  char subtype_str[APG_PDF_LINE_LEN_MAX];

  size_t offset;             // byte the `x y obj` starts in the pdf
  size_t meta_line_offs;     // offset of the << >> line
  size_t stream_data_length; // '/Length'
  size_t stream_data_offset;

  int x, y; // first two numbers on line

  int width, height;
  bool subtype_image; // '/Subtype /Image' this would be an enum if i cared about other subtypes
} apg_pdf_obj_t;

// for the web there are 'linear' aka 'optimised' pdf versions which can be read line-by-line. non-linear must be read in a block.
typedef struct apg_pdf_t {
  // HDR (pdf version)
  int version_major;
  int version_minor;

  // BODY ( between obj and endobj keywords ) list of objects
  apg_pdf_obj_t obj_list[APG_PDF_OBJ_LIST_MAX]; // do a stretchy buffer instead for general use
  int obj_list_n;

  // Cross-Reference Table (starts at xref keyword) file offset of each object

  // TRAILER (maybe trailer keyword). dictionary, offset of xref table, %%EOF or file is not valid.
  int xref_offs;

  bool loaded;
} apg_pdf_t;

// build a meta-data scaffold about PDF contents and their memory offsets
apg_pdf_t apg_pdf_read_mem( void* data, size_t sz );
