/*==============================================================
Quake-style Console mini-library
Language: C99
Author:   Anton Gerdelan - @capnramses
Contact:  <antongdl@protonmail.com>
Website:  https://github.com/capnramses/apg - http://antongerdelan.net/
Licence:  See bottom of header file.
==============================================================*/
#include "apg_console.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define C_VARS_MAX 128
#define C_HIST_MAX 32

typedef struct cvar_t {
  char str[C_STR_MAX];
  float val;
} cvar_t;

static cvar_t cvars[C_VARS_MAX];
static uint32_t n_cvars;

static char c_hist[C_HIST_MAX][C_STR_MAX];
static int c_hist_oldest = -1, c_hist_newest = -1;

// returns index or -1 if did not find
// NOTE(Anton) could replace with a hash table
static int _console_find( const char* str ) {
  assert( str );

  for ( int i = 0; i < n_cvars; i++ ) {
    if ( strncmp( cvars[i].str, str, C_STR_MAX ) == 0 ) { return i; }
  }
  return -1;
}

void c_hist_add( const char* str ) {
  assert( str );

  c_hist_newest = ( c_hist_newest + 1 ) % C_HIST_MAX;
  if ( c_hist_newest == c_hist_oldest ) { c_hist_oldest = ( c_hist_oldest + 1 ) % C_HIST_MAX; }
  if ( -1 == c_hist_oldest ) { c_hist_oldest = c_hist_newest; }
  strncpy( c_hist[c_hist_newest], str, C_STR_MAX - 1 );
}

void c_hist_print() {
  if ( c_hist_oldest < 0 || c_hist_newest < 0 ) { return; }
  int idx = c_hist_oldest;
  for ( int count = 0; count < C_HIST_MAX; count++ ) {
    printf( "%i) %s\n", idx, c_hist[idx] );
    idx = ( idx + 1 ) % C_HIST_MAX;
    if ( idx == c_hist_newest ) { return; }
  }
}

bool c_create_var( const char* str, float val ) {
  assert( str );

  if ( n_cvars >= C_VARS_MAX ) { return false; }
  int idx = _console_find( str );
  if ( idx >= 0 ) { return false; }
  idx = n_cvars++;
  strncpy( cvars[idx].str, str, C_STR_MAX - 1 );
  cvars[idx].val = val;
  return true;
}

bool c_set_var( const char* str, float val ) {
  assert( str );

  int idx = _console_find( str );
  if ( idx < 0 ) { return false; }
  cvars[idx].val = val;
  return true;
}

bool c_get_var( const char* str, float* val ) {
  assert( str && val );

  int idx = _console_find( str );
  if ( idx < 0 ) { return false; }
  *val = cvars[idx].val;
  return true;
}

bool c_autocomplete( const char* substr, char* completed ) {
  assert( substr );
  size_t len = strlen( substr );
  assert( len < C_STR_MAX );

  for ( int i = 0; i < n_cvars; i++ ) {
    char* res = strstr( cvars[i].str, substr );
    if ( cvars[i].str == res ) {
      strncpy( completed, cvars[i].str, C_STR_MAX );
      return true;
    }
  }
  return false;
}
