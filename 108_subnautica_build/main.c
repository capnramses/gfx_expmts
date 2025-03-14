#include "gfx.h"
#include "apg_maths.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

//#define NOISE_IMG "simplex.png"
#define NOISE_IMG "unfiltered_heightmap_out.png"

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
  gfx_texture_t simplex =
    gfx_texture_create_from_file( NOISE_IMG, ( gfx_texture_properties_t ){ .bilinear = true, .has_mips = true, .is_srgb = true, .repeats = true } );

  while ( !gfx_should_window_close() ) {
    double curr_s = gfx_get_time_s();

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
    mat4 V = look_at( ( vec3 ){ .y = 0.5f, .z = 1.5f },
      ( vec3 ){
        .y = 0.5f,
      },
      ( vec3 ){ .y = 1.0f } );
    mat4 M = scale_mat4( ( vec3 ){ scale, scale, scale } );

    gfx_alpha_testing( true );
    gfx_uniform1f( &shader, shader.u_time, (float)curr_s );
    gfx_draw_mesh( GFX_PT_TRIANGLES, &shader, P, V, M, mesh.vao, mesh.n_vertices, &simplex, 1 ); // TODO add shader texture
    gfx_alpha_testing( false );

    gfx_swap_buffer();
  }

  gfx_delete_texture( &simplex );
  gfx_delete_shader_program( &shader );
  gfx_delete_mesh( &mesh );
  gfx_stop();

  printf( "normal exit\n" );

  return 0;
}
