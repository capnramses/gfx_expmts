#include "apg_console.h"
#include "gfx.h"
#include <assert.h>

int main() {
  bool ret = gfx_start( 1920, 1080, true, "console demo", NULL );
  assert( ret );

  gfx_stop();

  return 0;
}
