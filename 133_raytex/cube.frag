/*
Based on the explanation and code here https://m4xc.dev/articles/amanatides-and-woo/
NOTES
- this skips the slab AABB test.
- indexing a vec3 like this works: var[1];
TODO
- hunt down bug where single-pixel lines appear on edges of space.
  - greater-than-or-equal that should be just greater-than OR
  - floating point rounding
  - or texture clamp issue.
  - it anyway is on the border of a voxel of value 0 and should be discarded.
    maybe it's being skipped as OOB by one check but still being rendered as part of the original cube.
  - yep it was this bit
     pos.x >= grid_size    ->    pos.x > grid_size
     but changing it adds an extra voxel internally eeeh!
     i think there's a slight numerical inaccuracy - perhaps comparison to original algorithm or other examples is key here.


- check if it still works with a model matrix / grid spinning.
- check ms cost/fps.
- think about lighting and shading.
  - the axis should inform which normal to use for shading.
- better cam controls
*/

#version 410 core

in vec3 v_vp;
in vec3 v_pos_wor;

uniform sampler3D tex;
uniform vec3 u_cam_pos_wor;

out vec4 frag_colour;

float grid_size = 16.0;
vec3 grid_max = vec3( 1.0 );
vec3 grid_min = vec3( -1.0 );
int MAX_STEPS = 128;

vec3 find_nearest( in vec3 ro, in vec3 rd, in float t_entry, out float t_end ) {
  vec3 voxels_per_unit = grid_size / ( grid_max - grid_min );
  vec3 entry_pos       = ( ( ro + rd * ( t_entry + 0.0001 ) ) - grid_min ) * voxels_per_unit; // why - grid_min??

  /* Get our traversal constants */
  vec3 step  = sign( rd );
  vec3 delta = abs( 1.0 / rd );

  /* IMPORTANT: Safety clamp the entry point inside the grid */
  vec3 pos = clamp( floor( entry_pos ), vec3( 0 ), vec3( grid_size ) );

  /* Initialize the time along the ray when each axis crosses its next cell boundary */
  vec3 tmax = ( pos - entry_pos + max( step, 0 ) ) / rd;

  vec3 rst = v_vp;

  int axis = 0;
  for ( int steps = 0; steps < MAX_STEPS; ++steps ) {
    /* Fetch the cell at our current position */
    // int i = int(pos.z * float(grid_size) * float(grid_size) + pos.y * float(grid_size) + pos.x);
    // uint voxel = grid[i];
    vec4 texel = texture( tex, rst );

    /* Check if we hit a voxel which isn't 0 */
    if ( texel.r + texel.b + texel.g > 0.0 ) {
      if ( steps == 0 ) {
        t_end = t_entry;
        return texel.rgb;
      }

      /* Return the time of intersection! */
      t_end = t_entry + ( tmax[axis] - delta[axis] ) / voxels_per_unit[axis]; // TODO check [] vs .x
      return texel.rgb;
    }

    /* Step on the axis where `tmax` is the smallest */
    if ( tmax.x < tmax.y ) {
      if ( tmax.x < tmax.z ) {
        pos.x += step.x;
        if ( pos.x < 0.0 || pos.x >= grid_size ) break;
        axis = 0;
        tmax.x += delta.x;
      } else {
        pos.z += step.z;
        if ( pos.z < 0.0 || pos.z >= grid_size ) break;
        axis = 2;
        tmax.z += delta.z;
      }
    } else {
      if ( tmax.y < tmax.z ) {
        pos.y += step.y;
        if ( pos.y < 0.0 || pos.y >= grid_size ) break;
        axis = 1;
        tmax.y += delta.y;
      } else {
        pos.z += step.z;
        if ( pos.z < 0.0 || pos.z >= grid_size ) break;
        axis = 2;
        tmax.z += delta.z;
      }
    }

    rst = pos / grid_size;
  }

  discard;
  t_end = 100000.0;
  return vec3( 0.0 ); // vec3(t_entry) * 0.1;
}

void main() {
  vec3 ray_o       = u_cam_pos_wor;
  vec3 ray_dist_3d = v_pos_wor - u_cam_pos_wor;
  float t_entry    = length( ray_dist_3d );
  vec3 ray_d       = normalize( ray_dist_3d );
  float t_end      = 0.0;
  vec3 nearest     = find_nearest( ray_o, ray_d, t_entry, t_end );
  if ( nearest.x + nearest.y + nearest.z == 0.0 ) { discard; }
  frag_colour.rgb = nearest;
  frag_colour.a = 1.0;

  // vec4 texel = texture( tex, v_vp );
  // frag_colour = vec4( texel.rgb, 1.0 );

  // frag_colour.rgb = ray_d;
}
