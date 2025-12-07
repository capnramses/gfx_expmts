/*
Based on the original paper,
and explanation and code here https://m4xc.dev/articles/amanatides-and-woo/
and Christer Ericson, Real Time Collision Detection, Chapter 7.
NOTES
- this skips the slab AABB test.
- indexing a vec3 like this works: var[1];
*/

#version 410 core

in vec3 v_pos_wor;

uniform sampler3D tex;
uniform vec3 u_cam_pos_wor;
uniform int u_n_cells;
uniform int u_show_bounding_cube;
uniform vec3 u_grid_max;
uniform vec3 u_grid_min;
uniform mat4 u_M;

out vec4 frag_colour;

const int MAX_STEPS = 128;


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

  int axis = 0;
  for ( int steps = 0; steps < MAX_STEPS; ++steps ) {
    /* Fetch the cell at our current position */
    vec3 rst = vec3( pos ) / float( n_cells );
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

  }

 // discard;
  t_end = 100000.0;
  return vec3( 0.0 );
}

void main() {
  vec3 grid_max   = vec3( 1.0 );
  vec3 grid_min   = vec3( -1.0 );

  vec3 ray_o       = u_cam_pos_wor;
  vec3 ray_dist_3d = v_pos_wor - ray_o;
  float t_entry    = length( ray_dist_3d );

float inside = 0.0;
  // If inside cube start ray t at camera position.
  if ( u_cam_pos_wor.x < grid_max.x && u_cam_pos_wor.x > grid_min.x &&
    u_cam_pos_wor.y < grid_max.y && u_cam_pos_wor.y > grid_min.y &&
    u_cam_pos_wor.z < grid_max.z && u_cam_pos_wor.z > grid_min.z  ) {
    t_entry = 0.0;
    inside = 1.0;
  }

  vec3 ray_d       = normalize( ray_dist_3d );
  float t_end      = 0.0;


  vec3 nearest     = find_nearest( ray_o, ray_d, t_entry, u_n_cells, u_grid_min, u_grid_max, t_end );
  
  frag_colour.rgb = nearest * 0.33 + nearest * 0.66 * ( 1.0 - clamp(abs(t_end) * 0.25, 0.0, 1.0 ) );
  frag_colour.a   = 1.0;


  if ( nearest.x + nearest.y + nearest.z == 0.0 ) {
  
    frag_colour = vec4(inside * 0.2,0.2,0.2,1.0);
    if ( u_show_bounding_cube > 0 ) { discard; }
  }
}
