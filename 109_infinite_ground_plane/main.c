// gcc .\main.c .\apg_maths.c .\gfx.c -I ..\common\glad\include\ -I ..\common\include\ ..\common\glad\src\glad.c -I ..\common\include\stb\ ..\common\win64_gcc\libglfw3dll.a

#include "gfx.h"
#include "apg_maths.h"
#include <stdio.h>

int main() {
  printf( "infinite ground plane demo\n" );

  vec3 cam_pos     = ( vec3 ){ .x = 0.0f, .y = 1.0f, .z = 0.0f };
  vec3 targ_pos    = ( vec3 ){ .x = 0.0f, .y = 1.0f, .z = 0.0f };
  vec3 up          = ( vec3 ){ .x = 0.0f, .y = 1.0f, .z = 0.0f };
  mat4 view_matrix = look_at( cam_pos, targ_pos, up );

  if ( !gfx_start( "infinite ground plane demo", NULL, false ) ) { return 1; }

  while (!gfx_should_window_close()) {
    gfx_poll_events();
    gfx_clear_colour_and_depth_buffers( 0.2f, 0.2f, 0.2f, 1.0f );

    gfx_swap_buffer();
  }

  gfx_stop();

  printf( "normal exit\n" );
  return 0;
}
