// unit tests for apg_console
// C99

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "apg_console.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_VARS

int main() {
  int n_lines = apg_c_count_lines();
  assert( 0 == n_lines );

  float a    = 0.0f;
  bool ret_a = apg_c_get_var( "anton", &a ); // fetch non-existent var
  assert( false == ret_a );
  bool ret_b = apg_c_set_var( "anton", a ); // set non-existent var
  assert( false == ret_b );
  n_lines = apg_c_count_lines();
  assert( 0 == n_lines );
  bool ret_c = apg_c_create_var( "anton", 123.0f ); // create var
  assert( true == ret_c );
  bool ret_d = apg_c_create_var( "anton", 456.0f ); // double-create extant var
  assert( false == ret_d );
  bool ret_e = apg_c_get_var( "anton", &a ); // fetch var
  assert( ret_e && fabsf( a ) >= 123.0f - FLT_EPSILON );
  bool ret_f = apg_c_set_var( "anton", 789.0f ); // change var
  assert( ret_f );
  bool ret_g = apg_c_get_var( "anton", &a ); // fetch var
  assert( ret_g && fabsf( a ) >= 789.0f - FLT_EPSILON );
  char completed[APG_C_STR_MAX];
  int ret_h = apg_c_autocomplete_var( "ant", completed );
  assert( ret_h == 1 );

  n_lines = apg_c_count_lines();
  printf( "n_lines=%i\n", n_lines );
  assert( 1 == n_lines );

  apg_c_dump_to_stdout();
  printf( "---\n" );
  printf( "completed `ant` to `%s`\n", completed );
  assert( strcmp( completed, "anton" ) == 0 );
  int ret_i = apg_c_autocomplete_var( "not_a_var", completed );
  assert( ret_i == 0 );
  apg_c_dump_to_stdout();
  printf( "---\n" );
  bool ret_j = apg_c_create_var( "antonio", 101112.0f ); // set another far that would match substring "ant"
  assert( ret_j );
  int ret_k = apg_c_autocomplete_var( "ant", completed );
  assert( ret_k == 2 );

  apg_c_dump_to_stdout();
  printf( "---\n" );

  n_lines = apg_c_count_lines();
  printf( "n_lines=%i\n", n_lines );
  assert( 3 == n_lines );

  apg_c_print( "first item" );
  apg_c_print( "second item" );
  apg_c_print( "3rd item" );
  apg_c_print( "4th item" );
  apg_c_print( "5th item" );
  apg_c_dump_to_stdout();
  printf( "---\n" );
  for ( int i = 0; i < 30; i++ ) {
    char tmp[256];
    sprintf( tmp, "item +%i", i );
    apg_c_print( tmp );
  }
  apg_c_dump_to_stdout();
  printf( "---\n" );

  n_lines = apg_c_count_lines();
  assert( n_lines == APG_C_OUTPUT_LINES_MAX );

  int w = 0, h = 0, n_chans = 3;
  bool retdims = apg_c_get_required_image_dims( &w, &h );
  assert( retdims );
  uint8_t* img_ptr = calloc( w * h * n_chans, 1 );
  bool retimg = apg_c_draw_to_image_mem( img_ptr, w, h, n_chans );
  assert( retimg );

  stbi_write_png( "testoutput.png", w, h, n_chans, img_ptr, w * n_chans );


  apg_c_output_clear();

  retimg = apg_c_draw_to_image_mem( img_ptr, w, h, n_chans );
  assert( retimg == false ); // empty image

  free( img_ptr );

  printf( "tests DONE\n" );
  return 0;
}
