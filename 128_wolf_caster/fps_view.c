#include "game.h"

fps_view_t fps_view_init( int w, int h ) {
  fps_view_t fps_view = (fps_view_t){ .img_ptr = NULL };
  fps_view.img_ptr    = calloc( w * h * 3, 1 );

  uint8_t colour[] = { 0x22, 0x22, 0x22 };
  for ( int i = 0; i < w * h * 3; i += 3 ) { memcpy( &fps_view.img_ptr[i], colour, 3 ); }

  fps_view.tex = gfx_create_texture_from_mem( w, h, 3, fps_view.img_ptr );
  return fps_view;
}

void fps_view_free( fps_view_t* fps_view_ptr ) {
  assert( fps_view_ptr );
  if ( fps_view_ptr->img_ptr ) { free( fps_view_ptr->img_ptr ); }
  glDeleteTextures( 1, &fps_view_ptr->tex.handle );
  *fps_view_ptr = (fps_view_t){ .img_ptr = NULL };
}

void fps_view_update_image( fps_view_t fps_view, const uint8_t* tiles_ptr, int tiles_w, int tiles_h, player_t player ) {
  assert( tiles_ptr );
  { // Draw ceiling and floor.
    for ( int y = 0; y < fps_view.tex.h / 2; y++ ) {
      for ( int x = 0; x < fps_view.tex.w; x++ ) {
        int mem_idx = ( y * fps_view.tex.w + x ) * 3;
        memcpy( &fps_view.img_ptr[mem_idx], &ceiling_colour.r, 3 );
      }
    }
    for ( int y = fps_view.tex.h / 2; y < fps_view.tex.h; y++ ) {
      for ( int x = 0; x < fps_view.tex.w; x++ ) {
        int mem_idx = ( y * fps_view.tex.w + x ) * 3;
        memcpy( &fps_view.img_ptr[mem_idx], &floor_colour.r, 3 );
      }
    }
  }
  vec2_t first_ray = rotate_vec2( player.dir, DEGS_TO_RADS * -FOV_HALF_DEGS );
  for ( int col_x = 0; col_x < fps_view.tex.w; col_x++ ) {
    bool do_plot = ( col_x == fps_view.tex.w / 2 ); // Only plot this ray.

    // Work out ray direction.
    vec2_t curr_ray = rotate_vec2( first_ray, DEGS_TO_RADS * col_x * DEGS_PER_RAY );
    // Distance to closest gridlines.
    float player_to_horiz = player.pos.y - floorf( player.pos.y );
    float player_to_vert  = player.pos.x - floorf( player.pos.x );
    // Use the ray direction vector to determine which if rays are going l/r or u/d.
    vec2_t ray_dir_mod = (vec2_t){ 1.0f, 1.0f };
    if ( curr_ray.x > 0 ) {
      player_to_vert = 1.0f - player_to_vert;
      ray_dir_mod.x  = -1.0f;
    }
    if ( curr_ray.y > 0 ) {
      player_to_horiz = 1.0f - player_to_horiz;
      ray_dir_mod.y   = -1.0f;
    }
    if ( do_plot ) {
      // plot helper lines to show which nearest grid boundary. I use 'horiz,vert' to refer to gridlines as 'x,y' is very easily confused.
      mmap_plot_line( player.pos, (vec2_t){ player.pos.x, player.pos.y - player_to_horiz * ray_dir_mod.y }, to_nearest_gridlines_colour );
      mmap_plot_line( player.pos, (vec2_t){ player.pos.x - player_to_vert * ray_dir_mod.x, player.pos.y }, to_nearest_gridlines_colour );
    }
    vec2_t curr_pos = player.pos;
    // Skip a whole lot of trig by using linear algebra to get ray gradient for both axes.
    float xs_per_y = 0.0f, ys_per_x = 0.0f;
    // TODO - handle error at start here for middle ray at 0.
    if ( 0.0f != curr_ray.y ) { xs_per_y = fabsf( curr_ray.x / curr_ray.y ); }
    if ( 0.0f != curr_ray.x ) { ys_per_x = fabsf( curr_ray.y / curr_ray.x ); }
    // Got position of the 2 intersections.
    float xs_to_horiz = player_to_horiz * xs_per_y;
    vec2_t curr_horiz = { .x = curr_pos.x - xs_to_horiz * ray_dir_mod.x, .y = curr_pos.y - player_to_horiz * ray_dir_mod.y };
    float ys_to_vert  = player_to_vert * ys_per_x;
    vec2_t curr_vert  = { .x = curr_pos.x - player_to_vert * ray_dir_mod.x, .y = curr_pos.y - ys_to_vert * ray_dir_mod.y };

    for ( int step = 0; step < RAY_STEPS_MAX; step++ ) {
      if ( do_plot ) {
        mmap_plot_cross( curr_horiz, intersect_point_colour_a );
        mmap_plot_cross( curr_vert, intersect_point_colour_b );
      }
      float curr_horiz_dist = length_vec2( sub_vec2( curr_horiz, curr_pos ) );
      float curr_vert_dist  = length_vec2( sub_vec2( curr_vert, curr_pos ) );
      // Choose intersection with shortest path from player for first intersection.
      vec2_t prev_pos        = curr_pos; // just used for plots.
      curr_pos               = curr_horiz;
      bool curr_pos_is_horiz = true;
      if ( curr_horiz_dist <= curr_vert_dist ) {
      } else {
        curr_pos          = curr_vert;
        curr_pos_is_horiz = false;
      }

      { // Work out if hit a wall.// Continue to next horiz and vert intersections.
        int curr_tile_x = floorf( curr_pos.x - 0.0001f * ray_dir_mod.x );
        int curr_tile_y = floorf( curr_pos.y - 0.0001f * ray_dir_mod.y );
        if ( curr_tile_x < 0 || curr_tile_x >= TILES_W || curr_tile_y < 0 || curr_tile_y >= TILES_H ) { break; }
        int curr_tile_idx  = curr_tile_y * TILES_W + curr_tile_x;
        int curr_tile_type = tiles_ptr[curr_tile_idx];

        bool hit          = true;
        rgb_t wall_colour = error_colour;
        switch ( curr_tile_type ) {
        case 0: hit = false; break;
        case 1: wall_colour = blue_tile_colour; break;
        case 2: wall_colour = green_tile_colour; break;
        case 3: wall_colour = red_tile_colour; break;
        default: break;
        } // endswith
        if ( hit ) {
          float dist_player_to_viewplane = 0.5f;
          float wall_height              = fps_view.tex.h;
          vec2_t vec_to_hit              = sub_vec2( curr_pos, player.pos );
          // Using ray_dist_to_hit directly gives a fish-eye effect.
          float ray_dist_to_hit  = length_vec2( vec_to_hit );
          float proj_wall_height = ( wall_height / ray_dist_to_hit ); // * dist_player_to_viewplane

          // correct fish-eye
          {
            vec2_t projected = project_vec2( vec_to_hit, normalise_vec2( player.dir ) );
            float corrected  = length_vec2( projected );
            proj_wall_height = ( wall_height / corrected ); // * dist_player_to_viewplane
          }

          int line_height = APG_CLAMP( proj_wall_height, 0, fps_view.tex.h );
          int half_gap    = ( fps_view.tex.h - line_height ) / 2;
          for ( int row_y = half_gap; row_y < fps_view.tex.h - half_gap; row_y++ ) {
            int mem_idx = ( row_y * fps_view.tex.w + col_x ) * 3;
            memcpy( &fps_view.img_ptr[mem_idx], &wall_colour.r, 3 );
          }
          if ( do_plot ) { mmap_plot_line( player.pos, curr_pos, intersect_hit_colour ); }

          continue;
        }
      } // endblock hit

      // Continue to next horiz and vert intersections.
      curr_horiz = curr_pos_is_horiz ? (vec2_t){ curr_pos.x - xs_per_y * ray_dir_mod.x, curr_pos.y - 1.0f * ray_dir_mod.y } : curr_horiz;
      curr_vert  = curr_pos_is_horiz ? curr_vert : (vec2_t){ curr_pos.x - 1.0f * ray_dir_mod.x, curr_pos.y - ys_per_x * ray_dir_mod.y };
    } // endfor ray step
  } // endfor columns

  gfx_update_texture_from_mem( &fps_view.tex, fps_view.img_ptr );
}

void fps_view_draw( fps_view_t fps_view ) {
  glUseProgram( textured_shader.program );
  glBindVertexArray( quad_mesh.vao );
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_2D, fps_view.tex.handle );
  glUniform2f( textured_shader.u_scale, 1.0f, 1.0f );
  glUniform2f( textured_shader.u_pos, 0.0f, 0.0f );
  glDrawArrays( quad_mesh.primitive, 0, quad_mesh.n_points );
}