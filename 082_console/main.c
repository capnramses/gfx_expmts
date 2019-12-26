#include "console.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int main() {
  float a = 0.0f;
  bool ret_a = c_get_var("anton", &a ); // fetch non-existent var
  assert( false == ret_a );
  bool ret_b = c_set_var("anton", a ); // set non-existent var
  assert( false == ret_b );
  bool ret_c = c_create_var( "anton", 123.0f ); // create var
  assert( true == ret_c );
  bool ret_d = c_create_var( "anton", 456.0f ); // double-create extant var
  assert( false == ret_d );
  bool ret_e = c_get_var( "anton", &a ); // fetch var
  assert( ret_e && fabsf(a) >= 123.0f - FLT_EPSILON );
  bool ret_f = c_set_var( "anton", 789.0f ); // change var
  assert( ret_f );
  bool ret_g = c_get_var( "anton", &a ); // fetch var
  assert( ret_g && fabsf(a) >= 789.0f - FLT_EPSILON );
  char completed[C_STR_MAX];
  bool ret_h = c_autocomplete( "ant", completed );
  assert( ret_h );
  printf( "completed `ant` to `%s`\n", completed );
  assert( strcmp( completed, "anton" ) == 0 );

  c_hist_add("first item");
  c_hist_add("second item");
  c_hist_add("3rd item");
  c_hist_add("4th item");
  c_hist_add("5th item");
  c_hist_print();
  printf("---\n");
  for ( int i = 0; i < 30; i++ ) {
    char tmp[256];
    sprintf( tmp, "item +%i", i );
    c_hist_add( tmp );
  }
  c_hist_print();
  printf("---\n");

  printf( "tests DONE\n" );
  return 0;
}