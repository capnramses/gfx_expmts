/*==============================================================
Quake-style Console mini-library
Language: C99
Author:   Anton Gerdelan - @capnramses
Contact:  <antongdl@protonmail.com>
Website:  https://github.com/capnramses/apg - http://antongerdelan.net/
Licence:  See bottom of header file.
==============================================================*/
#include "apg_console.h"
#include "apg_pixfont.h" // used for adding glyphs to image output
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct c_var_t {
  char str[APG_C_STR_MAX];
  float val;
} c_var_t;

static c_var_t c_vars[APG_C_VARS_MAX];
static uint32_t n_c_vars;

static char c_output_lines[APG_C_OUTPUT_LINES_MAX][APG_C_STR_MAX];
static int c_output_lines_oldest = -1, c_output_lines_newest = -1, c_n_output_lines = 0;

static int c_font_height_px = 16;

static char _c_user_entered_text[APG_C_STR_MAX];

// returns index or -1 if did not find
// NOTE(Anton) could replace with a hash table
static int _console_find_var( const char* str ) {
  assert( str );

  for ( int i = 0; i < n_c_vars; i++ ) {
    if ( strncmp( str, c_vars[i].str, APG_C_STR_MAX ) == 0 ) { return i; }
  }
  return -1;
}

static bool _parse_user_entered_instruction( const char* str ) {
  assert( str );

  char one[APG_C_STR_MAX], two[APG_C_STR_MAX], three[APG_C_STR_MAX], four[APG_C_STR_MAX];
  char tmp[APG_C_STR_MAX];
  one[0] = two[0] = three[0] = '\0';
  int n                      = sscanf( str, "%s %s %s %s", one, two, three, four );
  switch ( n ) {
  case 0: return true; // this would be simply '\n'
  case 1: {
    char tmp[APG_C_STR_MAX];

    // search for command match
    if ( strncmp( one, "clear", APG_C_STR_MAX ) == 0 ) {
      apg_c_output_clear();
      return true;
    }

    { // then variable. equivalent to 'get myvariable' but no 'get' command required in this console.
      float val   = 0;
      bool got_it = apg_c_get_var( one, &val );
      if ( got_it ) {
        sprintf( tmp, "%s %.2f", one, val );
        apg_c_print( tmp );
        return true;
      }
    }

    // give up
    sprintf( tmp, "ERROR: `%s` is not a recognised command or variable name.", one );
    apg_c_print( tmp );
    return false;
  } break;

  case 2: {
    // assume this is equiv to "set myvariable value" with an implied "set"
    float val   = (float)atof( two );
    bool set_it = apg_c_set_var( one, val );
    if ( set_it ) {
      sprintf( tmp, "`%s %.2f`", one, val );
      apg_c_print( tmp );
      return true;
    } else {
      sprintf( tmp, "ERROR: `%s` is not a recognised variable name. To create a new variable use `var`.", one );
      apg_c_print( tmp );
      return false;
    }
  } break;

  case 3: {
    // "var myvariable value"
    if ( strncmp( one, "var", APG_C_STR_MAX ) != 0 ) {
      sprintf( tmp, "ERROR: `%s` is not a recognised command for a 3-token instruction. Did you mean `var`?", one );
      apg_c_print( tmp );
      return false;
    }
    float val      = (float)atof( three );
    bool create_it = apg_c_create_var( two, val );
    if ( create_it ) {
      sprintf( tmp, "`var %s %.2f`", two, val );
      apg_c_print( tmp );
      return true;
    } else {
      sprintf( tmp, "ERROR: Symbol `%s` already exists.", two );
      apg_c_print( tmp );
      return false;
    }
  } break;

  default: {
    sprintf( tmp, "ERROR: too many tokens in instruction." );
    apg_c_print( tmp );
    return false;
  } break;
  } // endswitch

  // shouldn't get here
  assert( false );
  return false;
}

bool apg_c_append_user_entered_text( const char* str ) {
  assert( str );

  // check for buffer overflow
  int uet_len   = strnlen( _c_user_entered_text, APG_C_STR_MAX );
  int len       = strnlen( str, APG_C_STR_MAX );
  int total_len = uet_len + len;
  if ( total_len > APG_C_STR_MAX ) {
    apg_c_clear_user_entered_text();
    return false;
  }

  // append
  strncat( _c_user_entered_text, str, APG_C_STR_MAX );

  // check for line break and if so parse and then delete the rest of the string - no complex carry-on stuff.
  for ( int i = uet_len; i < total_len; i++ ) {
    if ( _c_user_entered_text[i] == '\n' ) {
      _c_user_entered_text[i] = '\0';
      bool parsed             = _parse_user_entered_instruction( _c_user_entered_text );
      _c_user_entered_text[0] = '\0';
      return parsed;
    }
  }

  return true;
}

void apg_c_clear_user_entered_text( void ) { _c_user_entered_text[0] = '\0'; }

void apg_c_output_clear( void ) {
  c_output_lines_oldest = c_output_lines_newest = -1;
  c_n_output_lines                              = 0;
}

int apg_c_count_lines( void ) { return c_n_output_lines; }

char* apg_c_alloc_and_concat_str( void ) {
  int n_lines = apg_c_count_lines();
  if ( n_lines < 1 ) { return NULL; }
  // add a byte for line breaks
  char* p = malloc( n_lines * ( APG_C_STR_MAX + 1 ) );
  p[0]    = '\0';
  for ( int i = 0, idx = c_output_lines_oldest; i < n_lines; i++, idx++ ) {
    idx = idx % APG_C_OUTPUT_LINES_MAX;
    if ( 0 != i ) { strcat( p, "\n" ); }
    strncat( p, c_output_lines[idx], APG_C_STR_MAX );
  }
  return p;
}

void apg_c_print( const char* str ) {
  assert( str );

  c_output_lines_newest = ( c_output_lines_newest + 1 ) % APG_C_OUTPUT_LINES_MAX;
  c_n_output_lines      = c_n_output_lines < APG_C_OUTPUT_LINES_MAX ? c_n_output_lines + 1 : APG_C_OUTPUT_LINES_MAX;
  if ( c_output_lines_newest == c_output_lines_oldest ) { c_output_lines_oldest = ( c_output_lines_oldest + 1 ) % APG_C_OUTPUT_LINES_MAX; }
  if ( -1 == c_output_lines_oldest ) { c_output_lines_oldest = c_output_lines_newest; }
  strncpy( c_output_lines[c_output_lines_newest], str, APG_C_STR_MAX - 1 );
}

void apg_c_dump_to_stdout( void ) {
  if ( c_output_lines_oldest < 0 || c_output_lines_newest < 0 ) { return; }
  int idx = c_output_lines_oldest;
  for ( int count = 0; count < APG_C_OUTPUT_LINES_MAX; count++ ) {
    printf( "%i) %s\n", idx, c_output_lines[idx] );
    if ( idx == c_output_lines_newest ) { return; }
    idx = ( idx + 1 ) % APG_C_OUTPUT_LINES_MAX;
  }
}

bool apg_c_create_var( const char* str, float val ) {
  assert( str );

  if ( n_c_vars >= APG_C_VARS_MAX ) { return false; }
  int idx = _console_find_var( str );
  if ( idx >= 0 ) { return false; }
  idx = n_c_vars++;
  strncpy( c_vars[idx].str, str, APG_C_STR_MAX - 1 );
  c_vars[idx].val = val;
  return true;
}

bool apg_c_set_var( const char* str, float val ) {
  assert( str );

  int idx = _console_find_var( str );
  if ( idx < 0 ) { return false; }
  c_vars[idx].val = val;
  return true;
}

bool apg_c_get_var( const char* str, float* val ) {
  assert( str && val );

  int idx = _console_find_var( str );
  if ( idx < 0 ) { return false; }
  *val = c_vars[idx].val;
  return true;
}

int apg_c_autocomplete_var( const char* substr, char* completed ) {
  assert( substr );
  size_t len = strlen( substr );
  assert( len < APG_C_STR_MAX );

  // check variables
  int n_matching        = 0;
  int last_matching_idx = -1;
  for ( int i = 0; i < n_c_vars; i++ ) {
    char* res = strstr( c_vars[i].str, substr );
    if ( c_vars[i].str == res ) {
      n_matching++;
      last_matching_idx = i;
      apg_c_print( c_vars[i].str );
    }
  }
  if ( 1 == n_matching ) { strncpy( completed, c_vars[last_matching_idx].str, APG_C_STR_MAX ); }
  return n_matching;
}

// NOTE(Anton) - don't need to create the whole buffer as one string.
//               could print line-by-line, avoiding the malloc() call, using known line height as offset.
//               this would also allow unique colours per line for eg error messages, debug output, and user-entered text.
bool apg_c_get_required_image_dims( int* w, int* h ) {
  assert( w && h );

  *w                  = 0;
  *h                  = 0;
  const int thickness = 1;
  const int outlines  = 0;
  char* str_ptr       = apg_c_alloc_and_concat_str();
  if ( !str_ptr ) { return false; }
  if ( !apg_pixfont_image_size_for_str( str_ptr, w, h, thickness, outlines ) ) { return false; }
  free( str_ptr );
  return true;
}

bool apg_c_draw_to_image_mem( uint8_t* img_ptr, int w, int h, int n_channels ) {
  assert( img_ptr );

  char* str_ptr = apg_c_alloc_and_concat_str();
  if ( !str_ptr ) { return false; }

  const int thickness = 1;
  const int outlines  = 0;
  const int v_flip    = 0;
  int result          = apg_pixfont_str_into_image( str_ptr, img_ptr, w, h, n_channels, 0xFF, 0xFF, 0xFF, 0xFF, thickness, outlines, v_flip );

  free( str_ptr );
  if ( APG_PIXFONT_FAILURE == result ) { return false; }
  return true;
}
