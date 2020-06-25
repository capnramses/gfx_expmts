#include "apg_gfx.h"
#include "apg_tga.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

int main() {
  gfx_start( "title flourish demo", false );

  gfx_stop();
  return 0;
}
