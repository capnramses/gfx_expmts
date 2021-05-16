#include "gfx.h"
#include "apg_maths.h"
#include <stdio.h>
#include <string.h>

int main( int argc, char** argv ) {
  char mesh_filename[256];
  mesh_filename[0] = '\0';
  if ( argc < 2 ) {
    printf( "Usage: %s MYMESH.PLY\n", argv[0] );
    return 0;
  }
  strncat( mesh_filename, argv[1], 255 );

  gfx_start( "shader effect", NULL, false );
  printf( "Loading mesh: %s\n", mesh_filename );
  gfx_mesh_t mesh = gfx_mesh_create_from_ply( mesh_filename );
  printf( "bounding radius=%f\n", mesh.bounding_radius );
  float scale = 1.0f;
  if ( mesh.bounding_radius > 0.0f ) { scale = 1.0f / mesh.bounding_radius; }
  gfx_shader_t shader = gfx_create_shader_program_from_files( "shader.vert", "shader.frag" ); // TODO create shader

  while ( !gfx_should_window_close() ) {
    gfx_poll_events();

    int fb_w = 0, fb_h = 0;
    gfx_framebuffer_dims( &fb_w, &fb_h );
    if ( fb_w == 0 || fb_h == 0 ) {
      // TODO sleep or check for window not in focus
      continue;
    }
    float aspect = (float)fb_w / (float)fb_h;
    gfx_viewport( 0, 0, fb_w, fb_h );
    gfx_clear_colour_and_depth_buffers( 0.2f, 0.2f, 0.2f, 1.0f );

    mat4 P = perspective( 67.0f, aspect, 0.1f, 100.0f );
    mat4 V = look_at( ( vec3 ){ .y = 0.5f, .z = 1.5f }, ( vec3 ){ .y = 0.5f,  }, ( vec3 ){ .y = 1.0f } );
    mat4 M = scale_mat4( ( vec3 ){ scale, scale, scale } );

    gfx_draw_mesh( GFX_PT_TRIANGLES, &gfx_default_textured_shader, P, V, M, mesh.vao, mesh.n_vertices, NULL, 0 ); // TODO add shader texture

    gfx_swap_buffer();
  }

  gfx_delete_shader_program( &shader );
  gfx_delete_mesh( &mesh );
  gfx_stop();

  printf( "normal exit\n" );

  return 0;
}
