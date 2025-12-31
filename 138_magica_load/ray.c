#include "ray.h"
#include "apg_maths.h"

// bool ray_aabb() { return false; }

// bool ray_obb() { return false; }

bool ray_voxel_grid( vec3 ro, vec3 rd, float t_entry, vec3 grid_whd, vec3 grid_min, vec3 grid_max, float* t_end_ptr, vec3* vox_n_ptr ) {
  vec3 voxels_per_unit = div_vec3_vec3( grid_whd, sub_vec3_vec3( grid_max, grid_min ) );
  // 0. local coords

  // 1. inside/outside check

  // 2.
  /*
  int find_nearest( in vec3 ro, in vec3 rd, in float t_entry, in int n_cells, in vec3 grid_min, in vec3 grid_max, out float t_end, out vec3 vox_n ) {
  vec3 voxels_per_unit = float( n_cells ) / ( grid_max - grid_min );
  vec3 entry_pos       = ( ( ro + rd * t_entry ) - grid_min ) * voxels_per_unit; // BUGFIX: +0.001 was introducing an artifact (line on corners).

  // Get our traversal constants
  ivec3 step   = ivec3( sign( rd ) );
  vec3 t_delta = abs( 1.0 / rd );

  // IMPORTANT: Safety clamp the entry point inside the grid
  ivec3 pos = clamp( ivec3( floor( entry_pos ) ), ivec3( 0 ), ivec3( n_cells - 1 ) ); // BUGFIX: upper bound from n_cells to n_cells-1.

  // Initialize the time along the ray when each axis crosses its next cell boundary
  vec3 t = ( pos - entry_pos + max( step, 0 ) ) / rd;

  vox_n = v_n_loc; // TODO(Anton) Encode the normal in the box vertex.

  int axis = 0;
  for ( int steps = 0; steps < MAX_STEPS; ++steps ) {
    // Fetch the cell at our current position
    vec3 rst     = vec3( pos ) / float( n_cells - 1 ); // BUGFIX: off by 1 was creating extra row on the bottom.
    rst          = clamp( rst, vec3( 0.0 ), vec3( 1.0 ) );
    uvec4 itexel = texture( u_vol_tex, vec3( rst.x, 1.0 - rst.y, rst.z ) );

    // Check if we hit a voxel which isn't 0
    if ( itexel.r > 0 ) { // Palette index 0 treated as air.
      if ( steps == 0 ) {
        t_end = t_entry;
        return int( itexel.r );
      }

      // Return the time of intersection!
      t_end = t_entry + ( t[axis] - t_delta[axis] ) / voxels_per_unit[axis];
      return int( itexel.r );
    }

    // Step on the axis where `tmax` is the smallest
    if ( t.x <= t.y && t.x <= t.z ) {
      pos.x += step.x; // i
      if ( pos.x < 0 || pos.x >= n_cells ) break;
      t.x += t_delta.x;
      axis  = 0;
      vox_n = vec3( 1.0, 0.0, 0.0 ) * -step; // The *step is to get -1 on the reverse sides.
    } else if ( t.y <= t.x && t.y <= t.z ) {
      pos.y += step.y; // j
      if ( pos.y < 0 || pos.y >= n_cells ) break;
      t.y += t_delta.y;
      axis  = 1;
      vox_n = vec3( 0.0, 1.0, 0.0 ) * -step;
    } else {
      pos.z += step.z; // k
      if ( pos.z < 0 || pos.z >= n_cells ) break;
      t.z += t_delta.z;
      axis  = 2;
      vox_n = vec3( 0.0, 0.0, 1.0 ) * -step;
    }
  }

  // discard;
  t_end = 100000.0;
  return 0;
}
*/

  return false;
}

vec3 ray_wor_from_mouse( float mouse_x, float mouse_y, int w, int h, mat4 inv_P, mat4 inv_V ) {
  vec4 ray_eye = mul_mat4_vec4( inv_P, (vec4){ ( 2.0f * mouse_x ) / (float)w - 1.0f, 1.0f - ( 2.0f * mouse_y ) / (float)h, -1.0f, 0.0f } );
  return normalise_vec3( vec3_from_vec4( mul_mat4_vec4( inv_V, (vec4){ ray_eye.x, ray_eye.y, -1.0f, 0.0f } ) ) );
}
