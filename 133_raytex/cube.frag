/*
Based on the original paper,
and explanation and code here https://m4xc.dev/articles/amanatides-and-woo/
and Christer Ericson, Real Time Collision Detection, Chapter 7.
NOTES
- this skips the slab AABB test.
- indexing a vec3 like this works: var[1];
*/

#version 410 core

in vec3 v_vp;
in vec3 v_pos_wor;

uniform sampler3D tex;
uniform vec3 u_cam_pos_wor;
uniform int u_n_cells;

out vec4 frag_colour;

const int MAX_STEPS = 128;

vec3 find_nearest2( in vec3 xyz1, in vec3 xyz2 ) {
  float grid_size = 16.0;
  float cell_side = 2.0 / grid_size; // If this was 1.0 and scaled later it would remove a lot of issues.

  ivec3 ijk = ivec3( floor( xyz1 / cell_side ) );
  
  ivec3 ijk_end = ivec3( floor( xyz2 / cell_side ) );

  //ivec3 d_ijk = sign( ijk );
  int d_i = ((xyz1.x < xyz2.x) ? 1 : ((xyz1.x > xyz2.x) ? -1 : 0));
  int d_j = ((xyz1.y < xyz2.y) ? 1 : ((xyz1.y > xyz2.y) ? -1 : 0));
  int d_k = ((xyz1.z < xyz2.z) ? 1 : ((xyz1.z > xyz2.z) ? -1 : 0));
  
  vec3 min_xyz = cell_side * floor( xyz1 / cell_side );
  vec3 max_xyz = min_xyz + cell_side;
  float t_x = ( ( xyz1.x > xyz2.x ) ? ( xyz1.x - min_xyz.x ) : ( max_xyz.x - xyz1.x ) ) / abs( xyz2.x - xyz1.x );
  float t_y = ( ( xyz1.y > xyz2.y ) ? ( xyz1.y - min_xyz.y ) : ( max_xyz.y - xyz1.y ) ) / abs( xyz2.y - xyz1.y );
  float t_z = ( ( xyz1.z > xyz2.z ) ? ( xyz1.z - min_xyz.z ) : ( max_xyz.z - xyz1.z ) ) / abs( xyz2.z - xyz1.z );
  
  vec3 delta_t_xyz = cell_side / abs( xyz2 - xyz1 );

  for ( int steps = 0; steps < MAX_STEPS; ++steps ) {
    // visit cell

    // ijk == v_vp;
    //vec3 rst = 

    vec3 rst = ijk / grid_size ;
    if ( rst.x >= 0.0 && rst.x < 1.0 && rst.y >= 0.0 && rst.y < 1.0 && rst.z >= 0.0 && rst.z < 1.0 ) {
      vec4 texel = texture( tex, vec3( ijk / grid_size) );
      if ( texel.r + texel.g + texel.b > 0.0 ) { return texel.rgb; }
    }

    if ( t_x <= t_y && t_x <= t_z ) {
      if ( ijk.x == ijk_end.x ) { break; }
      t_x += delta_t_xyz.x;
      ijk.x += d_i;
    } else if ( t_y <= t_x && t_y <= t_z ) {
      if ( ijk.y == ijk_end.y ) { break; }
      t_y += delta_t_xyz.y;
      ijk.y += d_j;
    } else {
      if ( ijk.z == ijk_end.z ) { break; }
      t_z += delta_t_xyz.z;
      ijk.z += d_k;
    } // endif
  } // endfor

  return vec3( 0.0 );
}

vec3 find_nearest( in vec3 ro, in vec3 rd, in float t_entry, in int n_cells, in vec3 grid_min, in vec3 grid_max, out float t_end ) {
  vec3 voxels_per_unit = float( n_cells ) / ( grid_max - grid_min );
  vec3 entry_pos       = ( ( ro + rd * t_entry ) - grid_min ) * voxels_per_unit; // BUGFIX: +0.001 was introducing an artifact (line on corners).

  /* Get our traversal constants */
  ivec3 step  = ivec3( sign( rd ) );
  vec3 t_delta = abs( 1.0 / rd );

  /* IMPORTANT: Safety clamp the entry point inside the grid */
  ivec3 pos = clamp( ivec3( floor( entry_pos ) ), ivec3( 0 ), ivec3( n_cells - 1 ) ); // BUGFIX: upper bound from n_cells to n_cells-1.

  /* Initialize the time along the ray when each axis crosses its next cell boundary */
  vec3 t = ( pos - entry_pos + max( step, 0 ) ) / rd;

  vec3 rst = v_vp;

  int axis = 0;
  for ( int steps = 0; steps < MAX_STEPS; ++steps ) {
    /* Fetch the cell at our current position */
    rst = clamp( rst, vec3(0.0), vec3(1.0) );
    vec4 texel = texture( tex, rst );

    /* Check if we hit a voxel which isn't 0 */
    if ( texel.r + texel.b + texel.g > 0.0 ) {
      if ( steps == 0 ) {
        t_end = t_entry;
        return texel.rgb;
      }

      /* Return the time of intersection! */
      t_end = t_entry + ( t[axis] - t_delta[axis] ) / voxels_per_unit[axis];
      return texel.rgb;
    }

    /* Step on the axis where `tmax` is the smallest */
    if ( t.x <= t.y && t.x <= t.z ) {
      pos.x += step.x; // i
      if ( pos.x < 0 || pos.x >= n_cells ) break;
      t.x += t_delta.x;
      axis = 0;
    } else if ( t.y <= t.x && t.y <= t.z ) {
      pos.y += step.y; // j
      if ( pos.y < 0 || pos.y >= n_cells ) break;
      t.y += t_delta.y;
      axis = 1;
    } else {
      pos.z += step.z; // k
      if ( pos.z < 0 || pos.z >= n_cells ) break;
      t.z += t_delta.z;
      axis = 2;
    }

    rst = vec3( pos ) / float( n_cells );
  }

  discard;
  t_end = 100000.0;
  return vec3( 0.0 );
}

void main() {
  vec3 ray_o       = u_cam_pos_wor;
  vec3 ray_dist_3d = v_pos_wor - u_cam_pos_wor;
  float t_entry    = length( ray_dist_3d );
  vec3 ray_d       = normalize( ray_dist_3d );
  float t_end      = 0.0;

  vec3 grid_max   = vec3( 1.0 );
  vec3 grid_min   = vec3( -1.0 );

  vec3 nearest     = find_nearest( ray_o, ray_d, t_entry, u_n_cells, grid_min, grid_max, t_end );
//  vec3 nearest = find_nearest2( v_pos_wor, v_pos_wor + ray_d * 3.0 );
  
  if ( nearest.x + nearest.y + nearest.z == 0.0 ) { discard; }
  frag_colour.rgb = nearest;
  frag_colour.a   = 1.0;

  // vec4 texel = texture( tex, v_vp );
  // frag_colour = vec4( texel.rgb, 1.0 );

  // frag_colour.rgb = ray_d;
}
