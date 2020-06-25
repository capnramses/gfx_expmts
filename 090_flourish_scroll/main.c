#include "apg_gfx.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  gfx_start( "title flourish demo", false );

  gfx_texture_t texture       = ( gfx_texture_t ){ .handle_gl = 0 };
  gfx_texture_t texture_title = ( gfx_texture_t ){ .handle_gl = 0 };
  {
    unsigned int w, h, n;
    uint8_t* img_ptr = stbi_load( "celtic.png", &w, &h, &n, 0 );
    if ( !img_ptr ) {
      printf( "failed loading flourish image\n" );
      return 1;
    }
    texture = gfx_create_texture_from_mem( img_ptr, w, h, n, ( gfx_texture_properties_t ){ .is_bgr = true, .bilinear = true } );
    free( img_ptr );

    img_ptr = stbi_load( "title.png", &w, &h, &n, 0 );
    if ( !img_ptr ) {
      printf( "failed loading title image\n" );
      return 1;
    }
    texture_title = gfx_create_texture_from_mem( img_ptr, w, h, n, ( gfx_texture_properties_t ){ .is_bgr = true, .bilinear = true } );
    free( img_ptr );
  }
  gfx_shader_t shader = gfx_create_shader_program_from_files( "wipe_in.vert", "wipe_in.frag" );

  float pts[]     = { -1, 1, -1, -1, 1, 1, 1, 1, -1, -1, 1, -1 };
  gfx_mesh_t mesh = gfx_create_mesh_from_mem( pts, 2, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, 6, false );

  while ( !gfx_should_window_close() ) {
    gfx_poll_events();
    double curr_time_s = gfx_get_time_s();
    float t            = fmodf( curr_time_s, 1.5f ) * (1.0f/1.5f); // 0 to 1 every 2s

    int ww = 0, wh = 0;
    gfx_window_dims( &ww, &wh );
    gfx_viewport( 0, 0, ww, wh );
    gfx_clear_colour_and_depth_buffers( 0.2, 0.2, 0.2, 1.0 );
    {
      gfx_alpha_testing( true );
      gfx_depth_testing( false );
      int w = 0, h = 0;
      float aspect = 1.0f;
      gfx_framebuffer_dims( &w, &h );
      if ( w > 0 && h > 0 ) { aspect = (float)w / (float)h; }
      float scalef = 1;
      vec2 scale   = ( vec2 ){ .x = scalef * ( (float)texture.w / w ), .y = scalef* ( (float)texture.h / h ) };
      // gfx_draw_textured_quad( texture, scale, ( vec2 ){ .x = 0, .y = 0 } );
      gfx_uniform2f( shader, shader.u_scale, scale.x, scale.y );

      mat4 M = identity_mat4();
      mat4 R = rot_z_deg_mat4( 180 );
      gfx_uniform1f( shader, shader.u_time, t );
      gfx_uniform2f( shader, shader.u_pos, 0, scale.y * 2 );
      gfx_draw_mesh( shader, identity_mat4(), identity_mat4(), M, mesh.vao, 6, &texture, 1 );

      gfx_uniform1f( shader, shader.u_time, 1 );
      gfx_uniform2f( shader, shader.u_pos, 0, 0 ); // NOTE(Anton) rotation includes the offset first
      gfx_draw_mesh( shader, identity_mat4(), identity_mat4(), M, mesh.vao, 6, &texture_title, 1 );

      gfx_uniform1f( shader, shader.u_time, t );
      gfx_uniform2f( shader, shader.u_pos, 0, -scale.y * 2 ); // NOTE(Anton) rotation includes the offset first
      gfx_draw_mesh( shader, identity_mat4(), identity_mat4(), R, mesh.vao, 6, &texture, 1 );

      gfx_alpha_testing( false );
      gfx_depth_testing( true );
    }
    gfx_swap_buffer();
  }

  gfx_delete_texture( &texture );
  gfx_stop();
  return 0;
}
