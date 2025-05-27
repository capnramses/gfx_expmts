#include "game.h"
#include "gfx.h"

bool minimap_draw_rays = true;

minimap_t mmap_init( int w, int h ) {
  minimap_t map = (minimap_t){ .img_ptr = NULL };
  map.img_ptr   = calloc( w * h * 3, 1 );

  uint8_t colour[] = { 0x22, 0x22, 0x22 };
  for ( int i = 0; i < w * h * 3; i += 3 ) { memcpy( &map.img_ptr[i], colour, 3 ); }

  map.tex = gfx_create_texture_from_mem( w, h, map.img_ptr );

  return map;
}

void mmap_free( minimap_t* map_ptr ) {
  assert( map_ptr );
  if ( map_ptr->img_ptr ) { free( map_ptr->img_ptr ); }
  glDeleteTextures( 1, &map_ptr->tex.handle );
  *map_ptr = (minimap_t){ .img_ptr = NULL };
}

void mmap_plot_line( vec2_t ori, vec2_t dest, rgb_t rgb ) {
  int tile_px_w = mmap.tex.w / TILES_W;
  int tile_px_h = mmap.tex.h / TILES_H;
  plot_line( ori.x * tile_px_w, ori.y * tile_px_h, dest.x * tile_px_w, dest.y * tile_px_h, &rgb.r, mmap.img_ptr, mmap.tex.w, mmap.tex.h, 3 );
}

void mmap_plot_cross( vec2_t pos, rgb_t rgb ) {
  int tile_px_w = mmap.tex.w / TILES_W;
  int tile_px_h = mmap.tex.h / TILES_H;
  plot_t_cross( pos.x * tile_px_w, pos.y * tile_px_h, 2, mmap.img_ptr, mmap.tex.w, mmap.tex.h, 3, &rgb.r );
}

/*
         |
         +  t1                            note: player heading, h, is from 0 pointing this way --->
        /|                                      so angle, a, in triangle here is 90 degrees - player heading
  t0   /a|                                      replaced, of course, with ray angle rather than player angle.
--------- +---
     /   |         ^
   a/    |         |  player_to_horiz     tOa  o = tan(alpha) * adjacent
  |/h    |         |                           a = opposite / tan(alpha)
  P--    |
         |

   <----> player_to_vert
*/
// float angle_a = PI * 0.25f - fmodf( player.heading, PI * 0.25f );

void mmap_update_image( minimap_t map, const uint8_t* tiles_ptr, int tiles_w, int tiles_h, player_t player ) {
  assert( tiles_ptr );
  int tile_px_w = map.tex.w / tiles_w;
  int tile_px_h = map.tex.h / tiles_h;
  // Draw tiles on a grid first.
  for ( int y = 0; y < map.tex.h; y++ ) {
    for ( int x = 0; x < map.tex.w; x++ ) {
      int mem_idx   = ( y * map.tex.w + x ) * 3;
      rgb_t colour  = error_colour;
      int tile_x    = x / tile_px_w;
      int tile_y    = y / tile_px_h;
      int tile_type = tiles_ptr[tile_y * tiles_w + tile_x];
      switch ( tile_type ) {
      case 0: colour = empty_tile_colour; break;
      case 1: colour = blue_tile_colour; break;
      case 2: colour = green_tile_colour; break;
      case 3: colour = red_tile_colour; break;
      default: break;
      }
      if ( x % tile_px_w == 0 ) { colour = grid_line_colour; }
      if ( y % tile_px_h == 0 ) { colour = grid_line_colour; }
      memcpy( &map.img_ptr[mem_idx], &colour.r, 3 );
    }
  }
  // Draw player.
  int px_x = player.pos.x * tile_px_w;
  int px_y = player.pos.y * tile_px_h;
  {
    plot_t_cross( px_x, px_y, 2, map.img_ptr, map.tex.w, map.tex.h, 3, &player_colour.r );
    plot_circle( px_x, px_y, 4, map.img_ptr, map.tex.w, map.tex.h, 3, &player_colour.r );

    // Dir line
    vec2_t end_pos = add_vec2( player.pos, mul_vec2_f( player.dir, dir_line_length_tiles ) );
    int e_x        = end_pos.x * tile_px_w;
    int e_y        = end_pos.y * tile_px_h;
    plot_line( px_x, px_y, e_x, e_y, &player_colour.r, map.img_ptr, map.tex.w, map.tex.h, 3 );
  }

  // Draw FOV lines
  vec2_t first_ray = rotate_vec2( player.dir, DEGS_TO_RADS * -FOV_HALF_DEGS );
  vec2_t end_pos   = add_vec2( player.pos, mul_vec2_f( first_ray, 1.0f ) );
  int e_x          = end_pos.x * tile_px_w;
  int e_y          = end_pos.y * tile_px_h;
  plot_line( px_x, px_y, e_x, e_y, &fov_colour.r, map.img_ptr, map.tex.w, map.tex.h, 3 );
  vec2_t last_ray = rotate_vec2( first_ray, DEGS_TO_RADS * FPS_W * DEGS_PER_RAY );
  end_pos         = add_vec2( player.pos, mul_vec2_f( last_ray, 1.0f ) );
  e_x             = end_pos.x * tile_px_w;
  e_y             = end_pos.y * tile_px_h;
  plot_line( px_x, px_y, e_x, e_y, &fov_colour.r, map.img_ptr, map.tex.w, map.tex.h, 3 );

  // Other rays etc are drawn in here next from fps view.
}

void mmap_draw( minimap_t map ) {
  gfx_update_texture_from_mem( &map.tex, map.img_ptr );
  glUseProgram( textured_shader.program );
  glUniform2f( textured_shader.u_scale, 1.0f, 1.0f );
  glUniform2f( textured_shader.u_pos, 0.0f, 0.0f );
  glBindVertexArray( quad_mesh.vao );
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, map.tex.handle );
  glDrawArrays( quad_mesh.primitive, 0, quad_mesh.n_points );
}
