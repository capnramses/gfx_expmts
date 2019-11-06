#include "apg_pdf.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t _skip_whitespace( const char* data, size_t sz, size_t next_offs ) {
  while ( next_offs < sz ) {
    if ( data[next_offs] != '\n' && data[next_offs] != '\r' && data[next_offs] != '\t' && data[next_offs] != ' ' ) { return next_offs; }
    next_offs++;
  }
  return next_offs;
}

// Every line ends with a carriage return, a line feed or a carriage return followed by a line feed ( of course )
static size_t _get_line( const char* data, size_t sz, size_t next_offs, char* line ) {
  int prev_offs = next_offs;
  next_offs     = _skip_whitespace( data, sz, next_offs );
  line[0] = line[APG_PDF_LINE_LEN_MAX - 1] = '\0';
  for ( int i = 0; i < APG_PDF_LINE_LEN_MAX; i++ ) {
    if ( next_offs >= sz || data[next_offs] == '\n' || data[next_offs] == '\r' ) {
      line[i] = '\0';
#ifdef DEBUG_LINES
      printf( "(%u) line =`%s` (%u)\n", (uint32_t)prev_offs, line, (uint32_t)next_offs );
#endif
      return next_offs;
    }
    line[i] = data[next_offs++];
  }
  return next_offs;
}

static bool _apg_pdf_parse_obj( const char* data, size_t sz, size_t prev_offs, apg_pdf_t* pdf, size_t* next_offs ) {
  char line[APG_PDF_LINE_LEN_MAX];
  int obj_idx = pdf->obj_list_n;

  assert( data && sz > 0 && pdf && next_offs );

  size_t meta_line_offs = prev_offs;
  { // get the << >> line
    char* needle_ptr = NULL;

    *next_offs = _get_line( (char*)data, sz, prev_offs, line );
    if ( line[0] != '<' || line[1] != '<' ) { return false; }
    pdf->obj_list[obj_idx].meta_line_offs = meta_line_offs;

    // look for type and subtype
    needle_ptr = strstr( line, "/Type " );
    if ( needle_ptr ) {
      int n = sscanf( needle_ptr, "/Type %s ", pdf->obj_list[obj_idx].type_str );
      if ( 1 != n ) { return false; }
    }

    needle_ptr = strstr( line, "/Subtype " );
    if ( needle_ptr ) {
      int n = sscanf( needle_ptr, "/Subtype %s ", pdf->obj_list[obj_idx].subtype_str );
      if ( 1 != n ) { return false; }
      if ( strcmp( "/Image", pdf->obj_list[obj_idx].subtype_str ) == 0 ) {
        pdf->obj_list[obj_idx].subtype_image = true;

        needle_ptr = strstr( line, "/Height " );
        int n      = sscanf( needle_ptr, "/Height %i ", &pdf->obj_list[obj_idx].height );
        if ( 1 != n ) { return false; }

        needle_ptr = strstr( line, "/Width " );
        n          = sscanf( needle_ptr, "/Width %i ", &pdf->obj_list[obj_idx].width );
        if ( 1 != n ) { return false; }

        printf( "Image found %ix%i\n", pdf->obj_list[obj_idx].width, pdf->obj_list[obj_idx].height );
      }
    }
  }

  while ( *next_offs < sz ) {
    prev_offs  = *next_offs;
    *next_offs = _get_line( (char*)data, sz, prev_offs, line );

    if ( strncmp( "endobj", line, 6 ) == 0 ) {
      pdf->obj_list_n++;
      return true;
    }

    if ( strncmp( "stream", line, 6 ) == 0 ) {
      // get stream byte size from the << >> line and record it in obj
      char* needle_ptr = strstr( &data[meta_line_offs], "/Length " );
      if ( !needle_ptr ) { return false; }
      uint32_t length = 0;
      int n           = sscanf( needle_ptr, "/Length %u ", &length );
      if ( 1 != n ) { return false; }
      pdf->obj_list[obj_idx].stream_data_length = length;
      *next_offs                                = _skip_whitespace( (char*)data, sz, *next_offs );
      pdf->obj_list[obj_idx].stream_data_offset = *next_offs;
      // offset next_offs by that byte size
      // make sure endstream is next
      prev_offs  = *next_offs + (size_t)length;
      *next_offs = _get_line( (char*)data, sz, prev_offs, line );
      if ( strncmp( "endstream", line, 9 ) != 0 ) { return false; }
    }
  }

  return false;
}

apg_pdf_t apg_pdf_read_mem( void* data, size_t sz ) {
  apg_pdf_t pdf;
  size_t prev_offs = 0, next_offs = 0;
  // NOTE(Anton) pdf lines have max length 255 apparently, but i found a few << >> that were much longer
  char line[APG_PDF_LINE_LEN_MAX];

  if ( !data || !sz ) { return pdf; }
  memset( &pdf, 0, sizeof( apg_pdf_t ) );

  { // HEADER
    next_offs = _get_line( (char*)data, sz, prev_offs, line );
    if ( strncmp( "%PDF-", line, 5 ) != 0 ) {
      return pdf; // NOT a pdf doc
    }
    int n = sscanf( line, "%%PDF-%i.%i", &pdf.version_major, &pdf.version_minor );
    if ( 2 != n ) { return pdf; }
    printf( "PDF version %i.%i\n", pdf.version_major, pdf.version_minor );
  }
  {
    // skip any comments
    while ( next_offs < sz ) {
      int x = 0, y = 0;
      char o = 0, b = 0, j = 0;
      prev_offs = next_offs;
      next_offs = _get_line( (char*)data, sz, prev_offs, line );

      // TRAILER stuff
      if ( strncmp( "%%EOF", line, 5 ) == 0 ) {
        pdf.loaded = true;
        return pdf;
      }
      if ( strncmp( "startxref", line, 9 ) == 0 ) {
        prev_offs = next_offs;
        next_offs = _get_line( (char*)data, sz, prev_offs, line );
        int i     = sscanf( line, "%i", &pdf.xref_offs );
        if ( 1 != i ) { return pdf; }
        continue;
      }

      // comment
      if ( '%' == line[0] ) { continue; }

      // BODY objects
      { // 2 0 obj
        int n = sscanf( line, "%i %i %c%c%c\n", &x, &y, &o, &b, &j );
        if ( 5 != n || 'o' != o || 'b' != b || 'j' != j ) { break; }
        assert( pdf.obj_list_n < APG_PDF_OBJ_LIST_MAX );
        pdf.obj_list[pdf.obj_list_n].offset = prev_offs;
        pdf.obj_list[pdf.obj_list_n].x      = x;
        pdf.obj_list[pdf.obj_list_n].x      = y;
        if ( !_apg_pdf_parse_obj( data, sz, next_offs, &pdf, &next_offs ) ) { return pdf; }
      }
    }
  }
  return pdf;
}
