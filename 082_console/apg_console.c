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
#include <string.h>

#define APG_C_VARS_MAX 128
#define APG_C_HIST_MAX 32

typedef struct c_var_t {
  char str[APG_C_STR_MAX];
  float val;
} c_var_t;

static c_var_t c_vars[APG_C_VARS_MAX];
static uint32_t n_c_vars;

static char c_hist[APG_C_HIST_MAX][APG_C_STR_MAX];
static int c_hist_oldest = -1, c_hist_newest = -1;

static int c_font_height_px = 16;

// returns index or -1 if did not find
// NOTE(Anton) could replace with a hash table
static int _console_find( const char* str ) {
  assert( str );

  for ( int i = 0; i < n_c_vars; i++ ) {
    if ( strncmp( str, c_vars[i].str, APG_C_STR_MAX ) == 0 ) { return i; }
  }
  return -1;
}

void apg_c_print( const char* str ) {
  assert( str );

  c_hist_newest = ( c_hist_newest + 1 ) % APG_C_HIST_MAX;
  if ( c_hist_newest == c_hist_oldest ) { c_hist_oldest = ( c_hist_oldest + 1 ) % APG_C_HIST_MAX; }
  if ( -1 == c_hist_oldest ) { c_hist_oldest = c_hist_newest; }
  strncpy( c_hist[c_hist_newest], str, APG_C_STR_MAX - 1 );
}

void apg_c_dump_to_stdout() {
  if ( c_hist_oldest < 0 || c_hist_newest < 0 ) { return; }
  int idx = c_hist_oldest;
  for ( int count = 0; count < APG_C_HIST_MAX; count++ ) {
    printf( "%i) %s\n", idx, c_hist[idx] );
    if ( idx == c_hist_newest ) { return; }
    idx = ( idx + 1 ) % APG_C_HIST_MAX;
  }
}

bool apg_c_create_var( const char* str, float val ) {
  assert( str );

  if ( n_c_vars >= APG_C_VARS_MAX ) { return false; }
  int idx = _console_find( str );
  if ( idx >= 0 ) { return false; }
  idx = n_c_vars++;
  strncpy( c_vars[idx].str, str, APG_C_STR_MAX - 1 );
  c_vars[idx].val = val;
  return true;
}

bool apg_c_set_var( const char* str, float val ) {
  assert( str );

  int idx = _console_find( str );
  if ( idx < 0 ) { return false; }
  c_vars[idx].val = val;
  return true;
}

bool apg_c_get_var( const char* str, float* val ) {
  assert( str && val );

  int idx = _console_find( str );
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

// TODO(Anton) --> replace "my_string" with the entire string with line breaks
bool apg_c_get_required_image_dims( int* w, int* h ) {
  assert( w && h );

  const int thickness = 1;
  const int outlines  = 0;
  if ( !apg_pixfont_image_size_for_str( "my_string", w, h, thickness, outlines ) ) { return false; }
  
  return true;
}

void apg_c_draw_to_image_mem() {

}
