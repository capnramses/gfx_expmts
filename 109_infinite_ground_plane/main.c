// gcc -g .\main.c .\apg_maths.c .\gfx.c -I ..\common\glad\include\ -I ..\common\include\ ..\common\glad\src\glad.c -I
// ..\common\include\stb\ ..\common\win64_gcc\libglfw3dll.a

#include "gfx.h"
#include "input.h"
#include "apg_maths.h"
#include <stdio.h>

int main() {
  printf( "infinite ground plane demo\n" );

  vec3 cam_pos = ( vec3 ){ .x = 0.0f, .y = 1.0f, .z = 0.0f };
  vec3 up      = ( vec3 ){ .x = 0.0f, .y = 1.0f, .z = 0.0f };

  if ( !gfx_start( "infinite ground plane demo", NULL, false ) ) { return 1; }
  input_init();

  gfx_shader_t shader = gfx_create_shader_program_from_files( "plane.vert", "plane.frag" );
  gfx_texture_t texture = gfx_texture_create_from_file( "grid.png", ( gfx_texture_properties_t ){ .bilinear = true, .has_mips = true, .is_srgb = true, .repeats = true } );

  double pt = gfx_get_time_s();
  while ( !gfx_should_window_close() ) {
    int win_w = 0, win_h = 0;
    gfx_window_dims( &win_w, &win_h );
    gfx_viewport( 0, 0, win_w, win_h );
    int fb_w = 0, fb_h = 0;
    gfx_framebuffer_dims( &fb_w, &fb_h );
    if ( 0 == fb_w || 0 == fb_h ) { continue; }

    double t  = gfx_get_time_s();
    double et = t - pt;

    input_reset_last_polled_input_states();
    gfx_poll_events();

    double camspeed = 0.01f;
    if ( input_is_key_held( 'W' ) ) { cam_pos.z -= et * camspeed; }
    if ( input_is_key_held( 'S' ) ) { cam_pos.z += et * camspeed; }
    if ( input_is_key_held( 'A' ) ) { cam_pos.x -= et * camspeed; }
    if ( input_is_key_held( 'D' ) ) { cam_pos.x += et * camspeed; }
    if ( input_is_key_held( 'Q' ) ) { cam_pos.y -= et * camspeed; }
    if ( input_is_key_held( 'E' ) ) { cam_pos.y += et * camspeed; }

    gfx_clear_colour_and_depth_buffers( 0.2f, 0.2f, 0.2f, 1.0f );

    float aspect      = (float)fb_w / (float)fb_h;
    float near_plane  = 0.1f;
    float far_plane   = 1000.0f;
    mat4 proj_matrix  = perspective( 66.6f, aspect, near_plane, far_plane );
    vec3 targ_pos     = ( vec3 ){ .x = cam_pos.x, .y = cam_pos.y, .z = cam_pos.z - 1.0f };
    mat4 view_matrix  = look_at( cam_pos, targ_pos, up );
    mat4 R            = rot_x_deg_mat4( -90.0f );
    mat4 T            = translate_mat4( ( vec3 ){ cam_pos.x, 0.0f, cam_pos.z } );
    mat4 S            = scale_mat4( ( vec3 ){ 1500.0f, 1500.0f, 1500.0f } );
    mat4 model_matrix = mult_mat4_mat4( T, mult_mat4_mat4( R, S ) );

    gfx_alpha_testing( true );
    gfx_draw_mesh( GFX_PT_TRIANGLE_STRIP, &shader, proj_matrix, view_matrix, model_matrix, gfx_ss_quad_mesh.vao, gfx_ss_quad_mesh.n_vertices, &texture, 1 );
    gfx_alpha_testing( false );

    gfx_swap_buffer();
  }

  gfx_stop();

  printf( "normal exit\n" );
  return 0;
}
