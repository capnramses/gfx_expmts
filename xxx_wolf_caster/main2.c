#include "game.h"

rgb_t empty_tile_colour           = { 0x22, 0x22, 0x22 };
rgb_t red_tile_colour             = { 0x99, 0x00, 0x00 };
rgb_t green_tile_colour           = { 0x00, 0x99, 0x00 };
rgb_t blue_tile_colour            = { 0x00, 0x00, 0x99 };
rgb_t error_colour                = { 0xFF, 0x00, 0xFF };
rgb_t grid_line_colour            = { 0x29, 0x29, 0x29 };
rgb_t player_colour               = { 0xFF, 0xFF, 0xFF };
rgb_t fov_colour                  = { 0x88, 0x88, 0x88 };
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
  1, 1, 0, 0, 0, 0, 0, 1, // 6
  1, 1, 1, 1, 1, 1, 1, 1  // 7
};

void move_player( GLFWwindow* window, player_t* p_ptr, double elapsed_s ) {
  const float speed = 1.0f, rot_speed = 2.0f;
  if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_LEFT ) ) { p_ptr->heading -= rot_speed * elapsed_s; }
  if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_RIGHT ) ) { p_ptr->heading += rot_speed * elapsed_s; }
  p_ptr->heading = fmodf( p_ptr->heading, 2.0 * PI );
  p_ptr->heading = p_ptr->heading >= 0.0f ? p_ptr->heading : p_ptr->heading + 2.0 * PI;
  p_ptr->dir.x   = cosf( p_ptr->heading );
  p_ptr->dir.y   = sinf( p_ptr->heading );
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

int main() {
  printf( "Wolfcaster 3-D\n" );
  GLFWwindow* window = gfx_start( WIN_W, WIN_H, "Wolfcaster 3-D" );
  if ( !window ) { return 1; }
  mmap                   = mmap_init( MINIMAP_W, MINIMAP_H );
  fps_view_t fps_view    = fps_view_init( FPS_W, FPS_H );
  player_t player        = (player_t){ .pos = (vec2_t){ 1.5f, 1.5f }, .heading = 0.0f, .dir = (vec2_t){ 1.0f, 0.0f } };
  uint8_t* debug_img_ptr = calloc( 512 * 512 * 3, 1 );
  texture_t debug_tex    = gfx_create_texture_from_mem( 512, 512, debug_img_ptr );

  double _prev_s = glfwGetTime();
  while ( !glfwWindowShouldClose( window ) ) {
    double curr_s    = glfwGetTime();
    double elapsed_s = curr_s - _prev_s;
    _prev_s          = curr_s;
    glfwPollEvents();
    if ( GLFW_PRESS == glfwGetKey( window, GLFW_KEY_ESCAPE ) ) { break; }
    move_player( window, &player, elapsed_s );

    glClearColor( 0.2, 0.0, 0.2, 1.0 );
    glClear( GL_COLOR_BUFFER_BIT );

    mmap_update_image( mmap, tiles_ptr, TILES_W, TILES_H, player ); // Wipes minimap. Call before FPS drawing as it adds lines.

    // 3D FPS viewport
    glViewport( 0, 0, FPS_VIEWPORT_W, FPS_VIEWPORT_H * FPS_VERTICAL_STRETCH );
    fps_view_update_image( fps_view, tiles_ptr, TILES_W, TILES_H, player );
    fps_view_draw( fps_view );

    // 2D minimap viewport
    glViewport( WIN_W - MINIMAP_W, WIN_H - MINIMAP_H, MINIMAP_W, MINIMAP_H );
    mmap_draw( mmap );

    float xs_per_y = 0.0f, ys_per_x = 0.0f;
    if ( 0.0f != player.dir.y ) { xs_per_y = fabsf( player.dir.x / player.dir.y ); }
    if ( 0.0f != player.dir.x ) { ys_per_x = fabsf( player.dir.y / player.dir.x ); }

    // Debugging stuff
    glViewport( 0, WIN_H - 512, 512, 512 );
    char str[1024];
    float angle_a = PI * 0.25f - fmodf( player.heading, PI * 0.25f );
    sprintf( str,
      "real pos (%.2f,%.2f) tile pos (%i,%i)\nheading rad=%.4f deg=%.4f dir (%.2f,%.2f)\nxs_per_y=%f ys_per_x=%f\ndegs per ray %f\nangle a rads=%.3f degs=%.3f",
      player.pos.x, player.pos.y, (int)floorf( player.pos.x ), (int)floorf( player.pos.y ), player.heading, RADS_TO_DEGS * player.heading, player.dir.x,
      player.dir.y, xs_per_y, ys_per_x, DEGS_PER_RAY, angle_a, angle_a * RADS_TO_DEGS );
    memset( debug_img_ptr, 0, 512 * 512 * 3 );
    apg_pixfont_str_into_image( str, debug_img_ptr, 512, 512, 3, 0xFF, 0xFF, 0xFF, 0xFF, 1, 1, APG_PIXFONT_STYLE_REGULAR, 256 );
    gfx_update_texture_from_mem( &debug_tex, debug_img_ptr );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, debug_tex.handle );
    glDrawArrays( quad_mesh.primitive, 0, quad_mesh.n_points );

    glfwSwapBuffers( window );
  }

  free( debug_img_ptr );
  fps_view_free( &fps_view );
  mmap_free( &mmap );
  glfwTerminate();
  printf( "normal exit.\n" );
  return 0;
}