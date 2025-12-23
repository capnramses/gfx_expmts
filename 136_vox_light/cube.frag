/*
Based on the original paper,
and explanation and code here https://m4xc.dev/articles/amanatides-and-woo/
and Christer Ericson, Real Time Collision Detection, Chapter 7.
NOTES
- this skips the slab AABB test.
- indexing a vec3 like this works: var[1];
*/

#version 410 core

#define DEBUG_DDA

in vec3 v_pos_loc;
in vec3 v_n_loc;
in vec3 v_ray_o_loc;
in vec3 v_ray_dist_3d_loc;

uniform usampler3D u_vol_tex;
uniform sampler1D u_pal_tex;

uniform mat4 u_P, u_V, u_M, u_M_inv;

uniform int u_n_cells;
uniform int u_show_bounding_cube;

out vec4 frag_colour;

const int MAX_STEPS = 128;

int find_nearest( in vec3 ro, in vec3 rd, in float t_entry, in int n_cells, in vec3 grid_min, in vec3 grid_max, out float t_end, out vec3 vox_n ) {
  vec3 voxels_per_unit = float( n_cells ) / ( grid_max - grid_min );
  vec3 entry_pos       = ( ( ro + rd * t_entry ) - grid_min ) * voxels_per_unit; // BUGFIX: +0.001 was introducing an artifact (line on corners).

  /* Get our traversal constants */
  ivec3 step  = ivec3( sign( rd ) );
  vec3 t_delta = abs( 1.0 / rd );

  /* IMPORTANT: Safety clamp the entry point inside the grid */
  ivec3 pos = clamp( ivec3( floor( entry_pos ) ), ivec3( 0 ), ivec3( n_cells - 1 ) ); // BUGFIX: upper bound from n_cells to n_cells-1.

  /* Initialize the time along the ray when each axis crosses its next cell boundary */
  vec3 t = ( pos - entry_pos + max( step, 0 ) ) / rd;

  vox_n = v_n_loc; // TODO(Anton) Encode the normal in the box vertex.

  int axis = 0;
  for ( int steps = 0; steps < MAX_STEPS; ++steps ) {
    /* Fetch the cell at our current position */
    vec3 rst = vec3( pos ) / float( n_cells - 1 ); // BUGFIX: off by 1 was creating extra row on the bottom.
    rst = clamp( rst, vec3(0.0), vec3(1.0) );
    uvec4 itexel = texture( u_vol_tex, vec3( rst.x, 1.0 - rst.y, rst.z ) );

    /* Check if we hit a voxel which isn't 0 */
    if ( itexel.r > 0 ) {                         // Palette index 0 treated as air.
      if ( steps == 0 ) {
        t_end = t_entry;
        return int( itexel.r );
      }

      /* Return the time of intersection! */
      t_end = t_entry + ( t[axis] - t_delta[axis] ) / voxels_per_unit[axis];
      return  int( itexel.r );
    }

    /* Step on the axis where `tmax` is the smallest */
    if ( t.x <= t.y && t.x <= t.z ) {
      pos.x += step.x; // i
      if ( pos.x < 0 || pos.x >= n_cells ) break;
      t.x += t_delta.x;
      axis = 0;
      vox_n = vec3( 1.0, 0.0, 0.0 ) * -step; // The *step is to get -1 on the reverse sides.
    } else if ( t.y <= t.x && t.y <= t.z ) {
      pos.y += step.y; // j
      if ( pos.y < 0 || pos.y >= n_cells ) break;
      t.y += t_delta.y;
      axis = 1;
      vox_n = vec3( 0.0, 1.0, 0.0 ) * -step;
    } else {
      pos.z += step.z; // k
      if ( pos.z < 0 || pos.z >= n_cells ) break;
      t.z += t_delta.z;
      axis = 2;
      vox_n = vec3( 0.0, 0.0, 1.0 ) * -step;
    }

  }

 // discard;
  t_end = 100000.0;
  return 0;
}

// From https://stackoverflow.com/questions/12751080/glsl-point-inside-box-test
// I think step() does still do branching, and this isn't any faster in testing than a big if-clause.
float insideBox3D(vec3 v, vec3 bottomLeft, vec3 topRight) {
    vec3 s = step( bottomLeft, v ) - step( topRight, v );
    return s.x * s.y * s.z; 
}

float insideBoxOriginal( vec3 v, vec3 bottomLeft, vec3 topRight) {
  if ( v.x < topRight.x && v.x > bottomLeft.x &&
    v.y < topRight.y && v.y > bottomLeft.y &&
    v.z < topRight.z && v.z > bottomLeft.z  ) {
    return 1.0;
  }
  return 0.0;
}

// Also just another fooling-yourself becasue all() does the branching I suspect.
float inBox( vec3 p, vec3 lo, vec3 hi ) {
    bvec4 b = bvec4( greaterThan(p, lo), lessThan(p, hi) );
    if( all(b) ) {
      return 1.0;
    }
    return 0.0;
}


struct Light
{
  vec3 pos_wor;
  vec3 diffuse;
  vec3 ambient;
};

vec3 lighting( in vec3 p_loc, in vec3 n_loc, in vec3 k_diffuse, vec3 k_ambient, in Light l ) {

  vec3 i_ambient = l.ambient * k_ambient;

  vec3 l_pos_loc = ( u_M_inv * vec4( l.pos_wor, 1.0 ) ).xyz;
  vec3 dist_p_to_l_pos_loc = l_pos_loc - p_loc;
	vec3 dir_p_to_l_pos_loc = normalize( dist_p_to_l_pos_loc );
  vec3 i_diffuse = l.diffuse * k_diffuse * max( dot( dir_p_to_l_pos_loc, n_loc ), 0.0 );

  return i_diffuse + i_ambient;
}

int fullbrights[256];

void main() {
  const vec3 grid_max_loc = vec3( 1.0 );
  const vec3 grid_min_loc = vec3( -1.0 );
  vec3 ray_o_loc          = v_ray_o_loc;
  vec3 ray_dist_3d_loc    = v_ray_dist_3d_loc; // v_pos_loc - ray_o_loc;
  float t_entry           = length( ray_dist_3d_loc );

  float inside = insideBoxOriginal( ray_o_loc, grid_min_loc, grid_max_loc );
  t_entry *= ( 1.0 - inside );

  vec3 ray_d_loc       = normalize( ray_dist_3d_loc );
  float t_end      = 0.0;

  vec3 vox_n = vec3( 0.0 );
  int pal_idx_of_nearest = find_nearest( ray_o_loc, ray_d_loc, t_entry, u_n_cells, grid_min_loc, grid_max_loc, t_end, vox_n );
  int vis = pal_idx_of_nearest + abs( u_show_bounding_cube );
  
  if ( 0 == vis ) { discard; }

  vec4 texel = texelFetch( u_pal_tex, pal_idx_of_nearest, 0 ); // Note had to convert uvec to int type (uint not okay).

  fullbrights[16*9+8] = 1; // Test concept with rubies on sword.

  Light lights[3] = Light[3](
    Light( vec3( 5.0, 5.0, 10.0 ), vec3( 0.6, 0.1, 0.15 ), vec3( 0.8 ) ),
    Light( vec3( -7.0, 6.0, 9.0 ), vec3( 0.15, 0.5, 0.1 ), vec3( 0.9 ) ),
    Light( vec3( 0.5, 0.0, 12.0 ), vec3( 0.1, 0.05, 0.6 ), vec3( 0.7 ) )
  );

  vec3 k_ambient = vec3( 0.05 );
  vec3 lit_rgb = vec3( 0.0 );
  
  for ( int i = 0; i < 3; i++ ) {
    lit_rgb += lighting( ray_o_loc + ray_d_loc * t_end, vox_n, texel.rgb, k_ambient, lights[i] );
  }

  vec3 rgb = lit_rgb * ( 1 - fullbrights[pal_idx_of_nearest] ) + texel.rgb * ( fullbrights[pal_idx_of_nearest] );

#ifdef DEBUG_DDA
  if ( 0 == pal_idx_of_nearest && u_show_bounding_cube > 0 ) {
    rgb = inside * vec3( 0.2,0.2,0.2 ) + (1.0 - inside) * vec3( 0.2,0.5,0.2 );
  }
#endif

  frag_colour.rgb = rgb;
  //frag_colour.rgb = vox_n;
  frag_colour.a   = 1.0;
}
