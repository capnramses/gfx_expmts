/*
Wolf Caster 3-D by Anton Gerdelan. 27 May 2025.

This is my first attempt at making my own Wolfenstein 3-D type ray caster.

I read Fabian Sanglard's Game Engine Black Book, and skimmed a few blog posts/videos,
but ultimately decided to follow my intuition and do it all my own way.

The original Wolfenstein 3-D, and a lot of the tutorials around, base everything on
school trigonometry (soh-cah-toa). When I first started playing with 3D graphics I
used a lot of this too. I've since enjoyed solving geometric problems with linear
algebra. This demo is very much "how I would solve this using mathematics I
currently use".

The trickiest thing was getting the perspective right:
- I used vector projection to solve the fish-eye effect problem. I had a typo bug in my projection function
  that was making me think I misunderstood the problem, for ages.

Rendering is still in pixels - I fill out an OpenGL texture that covers the screen.
Although I haven't bothered with emulating 1990s colour palettes.
A more efficient modern raycast renderer would probably lean less on texture updates for the raycasting and debug map.
This could be done directly in a shader and remove that bottleneck entirely.
To do that you'd need to convert fps_view_update_image() into a fragment shader or compute shader.

- The guts of this demo is in the fps.c file - it's about 100 lines of code to do all the mathematics and rendering.
- I put my linear algebra functions in maths.c.
- OpenGL is just used to open a window and stuff - that's in gfx.c.
- I drew vectors and points onto a minimap to debug as I went. It's a big texture, and not very efficient.
  Turn it off with the tab key.
*/

#include "game.h"

rgb_t empty_tile_colour           = { 0x22, 0x22, 0x22 };
rgb_t red_tile_colour             = { 0x99, 0x00, 0x00 };
rgb_t green_tile_colour           = { 0x00, 0x99, 0x00 };
rgb_t blue_tile_colour            = { 0x00, 0x00, 0x99 };
rgb_t error_colour                = { 0xFF, 0x00, 0xFF };
rgb_t grid_line_colour            = { 0x29, 0x29, 0x29 };
rgb_t player_colour               = { 0xFF, 0xFF, 0xFF };
rgb_t fov_colour                  = { 0x88, 0x88, 0xFF };
rgb_t to_nearest_gridlines_colour = { 0xAA, 0xAA, 0xAA };
rgb_t intersect_point_colour_a    = { 0x55, 0xAA, 0x55 };
rgb_t intersect_point_colour_b    = { 0x55, 0x55, 0xAA };
rgb_t intersect_hit_colour        = { 0xFF, 0x00, 0x00 };
rgb_t ceiling_colour              = { 0x27, 0x27, 0x27 };
rgb_t floor_colour                = { 0x33, 0x33, 0x33 };
float dir_line_length_tiles       = 0.25f;
minimap_t mmap;

const uint8_t tiles_ptr[TILES_W * TILES_H] = {
  1, 1, 1, 1, 1, 1, 1, 1, // 0
  1, 0, 1, 0, 1, 0, 0, 1, // 1
  2, 0, 1, 0, 1, 0, 0, 1, // 2
  2, 0, 0, 0, 0, 0, 0, 1, // 3
  3, 0, 0, 0, 1, 1, 1, 1, // 4
  3, 0, 0, 1, 1, 0, 0, 1, // 5
  1, 1, 0, 0, 0, 0, 0, 1, // 6gfx.c.
  if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_UP ) ) {
    // epsilon here prevents player getting into invalid tile coords when pos is truncated.
    p_ptr->pos.x = APG_CLAMP( p_ptr->pos.x + p_ptr->dir.x * speed * elapsed_s, 0.0f, (float)TILES_W - 0.00001f );
    p_ptr->pos.y = APG_CLAMP( p_ptr->pos.y + p_ptr->dir.y * speed * elapsed_s, 0.0f, (float)TILES_H - 0.00001f );
  }
  if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_DOWN ) ) {
    p_ptr->pos.x = APG_CLAMP( p_ptr->pos.x - p_ptr->dir.x * speed * elapsed_s, 0.0f, (float)TILES_W - 0.00001f );
    p_ptr->pos.y = APG_CLAMP( p_ptr->pos.y - p_ptr->dir.y * speed * elapsed_s, 0.0f, (float)TILES_H - 0.00001f );
  }
}

#define DEBUG_W 64
#define DEBUG_H 16
uint8_t* debug_img_ptr;

int main() {
  printf( "Wolfcaster 3-D\n" );
  GLFWwindow* window = gfx_start( WIN_W, WIN_H, "Wolfcaster 3-D" );
  if ( !window ) { return 1; }
  mmap                = mmap_init( MINIMAP_W, MINIMAP_H );
  fps_view_t fps_view = fps_view_init( FPS_W, FPS_H );
  player_t player     = (player_t){ .pos = (vec2_t){ 1.5f, 1.5f }, .heading = 0.0f, .dir = (vec2_t){ 1.0f, 0.0f } };
  debug_img_ptr       = calloc( DEBUG_W * DEBUG_H * 4, 1 );
  texture_t debug_tex = gfx_create_texture_from_mem( DEBUG_W, DEBUG_H, 4, debug_img_ptr );

  bool tab_down = false, r_down = false;
  bool do_minimap_draw = true;
  double _prev_s       = glfwGetTime();
  double fps_cd        = 0.1;
  double accum_s       = 0.0;
  double timestep_s    = 1.0 / 60.0;
  //glfwSwapInterval(0) ;
  while ( !glfwWindowShouldClose( window ) ) {
    double curr_s    = glfwGetTime();
    double elapsed_s = curr_s - _prev_s;
    _prev_s          = curr_s;
    glfwPollEvents();
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_ESCAPE ) ) { break; }
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_R ) ) {
      if ( !r_down ) {
        minimap_draw_rays = !minimap_draw_rays;
        r_down            = true;
      }
    } else {
      r_down = false;
    }
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_TAB ) ) {
      if ( !tab_down ) {
        do_minimap_draw = !do_minimap_draw;
        tab_down        = true;
      }
    } else {
      tab_down = false;
    }
    memset( debug_img_ptr, 0x00, debug_tex.w * debug_tex.h * debug_tex.n );

    // Wipes minimap. Call before FPS drawing as it adds lines.
    if ( do_minimap_draw ) { mmap_update_image( mmap, tiles_ptr, TILES_W, TILES_H, player ); }
    
    accum_s += elapsed_s;
    while ( accum_s >= timestep_s ) {
      accum_s -= timestep_s;
      move_player( window, &player, timestep_s );
    }
      fps_view_update_image( fps_view, tiles_ptr, TILES_W, TILES_H, player );

    glClearColor( 0.2, 0.0, 0.2, 1.0 );
    glClear( GL_COLOR_BUFFER_BIT );

    // 3D FPS viewport
    glViewport( 0, 0, FPS_VIEWPORT_W, FPS_VIEWPORT_H * FPS_VERTICAL_STRETCH );
    fps_view_draw( fps_view );

    // 2D minimap viewport
    if ( do_minimap_draw ) {
      glViewport( WIN_W - MINIMAP_W, WIN_H - MINIMAP_H, MINIMAP_W, MINIMAP_H );
      mmap_draw( mmap );
    }

    // Debugging stuff
    glViewport( 0, WIN_H - debug_tex.h * 2, debug_tex.w * 2, debug_tex.h * 2 );
    fps_cd -= elapsed_s;
    if ( fps_cd <= 0.0 ) {
      fps_cd  = 0.1;
      int fps = (int)ceilf( 1.0 / elapsed_s );
      char fps_str[1024];
      sprintf( fps_str, "FPS %i", fps );
      apg_pixfont_str_into_image( fps_str, debug_img_ptr, debug_tex.w, debug_tex.h, debug_tex.n, 0xFF, 0xFF, 0xFF, 0xFF, 1, 1, APG_PIXFONT_STYLE_REGULAR, 0 );
      gfx_update_texture_from_mem( &debug_tex, debug_img_ptr );
    }

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_BLEND );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, debug_tex.handle );
    glDrawArrays( quad_mesh.primitive, 0, quad_mesh.n_points );
    glDisable( GL_BLEND );

    glfwSwapBuffers( window );
  }

  free( debug_img_ptr );
  fps_view_free( &fps_view );
  mmap_free( &mmap );
  glfwTerminate();
  printf( "normal exit.\n" );
  return 0;
}