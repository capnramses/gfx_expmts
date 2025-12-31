/*
By Anton Gerdelan, 2025.
Based on the original paper,
and explanation and code here https://m4xc.dev/articles/amanatides-and-woo/
and Christer Ericson, Real Time Collision Detection, Chapter 7.
NOTES
- this skips the slab AABB test.
- indexing a vec3 like this works: var[1];
*/

#version 420 core

in vec3 v_pos_loc;
in vec3 v_n_loc;
in vec3 v_ray_o_loc;
in vec3 v_ray_dist_3d_loc;

uniform vec3 u_shape;
uniform usampler3D u_vol_tex;
uniform sampler1D u_pal_tex;

uniform mat4 u_P, u_V, u_M, u_M_inv;

uniform ivec3 u_n_cells;

out vec4 frag_colour;

//layout(early_fragment_tests) in; // More recent hardware can force early depth tests, using a special fragment shader layout qualifier: 
// https://registry.khronos.org/OpenGL/extensions/ARB/ARB_conservative_depth.txt
//layout (depth_greater) out float gl_FragDepth; // Hint so GL doesn't disable early-z test when we write to fragdepth.

const int MAX_STEPS = 512;

// Impl from C.Ericson's RTCD Ch. 7 pg 326.
void visit_cells_overlapped( in vec3 entry_pos, in vec3 exit_pos ) {
  // vec3 entry_pos = ro + rd * t_entry;
  
  // Side dimensions of square cell.
  const float CELL_SIDE = 16.0; // 32 / (grid max - grid_min);

  // Determine start grid cell coords (i,j,k).
  ivec3 cell_ijk = ivec3( floor( entry_pos / CELL_SIDE ) );

  // TODO slabs method would derive this.
  
  // Determine end grid cell coords (iend, jend)
  ivec3 end_cell_ijk = ivec3( floor( exit_pos / CELL_SIDE ) );

  // Determine in which primary direction to step.
  ivec3 step_dir_ijk;
  step_dir_ijk.x = ( ( entry_pos.x < exit_pos.x ) ? 1 : ( ( entry_pos.x > exit_pos.x ) ? -1 : 0 ) );

  // Determine tx and ty, the values of t at which the directed line segment (entrypos-exitpos) crosses the first horiz an vertical cell boundaries, respectively.
  // Min(tx, ty) indicates how for one can travel along the line segment and still remain in the current cell.
  vec3 min_xyz = CELL_SIDE * floor( entry_pos / CELL_SIDE );
  vec3 max_xyz = min_xyz + CELL_SIDE;
  float t_x = ( ( entry_pos.x > exit_pos.x ) ? ( entry_pos.x - min_xyz.x ) : ( max_xyz.x - entry_pos.x ) ) / abs( exit_pos.x - entry_pos.x );
  float t_y = ( ( entry_pos.y > exit_pos.y ) ? ( entry_pos.y - min_xyz.y ) : ( max_xyz.y - entry_pos.y ) ) / abs( exit_pos.y - entry_pos.y );
  float t_z = ( ( entry_pos.z > exit_pos.z ) ? ( entry_pos.z - min_xyz.z ) : ( max_xyz.z - entry_pos.z ) ) / abs( exit_pos.z - entry_pos.z );

  // Determine delta_x/delta_y; how far, in units of t, one must step along the directed line segment for the horiz/vertical movement, respectively, to equal the width/height of a a cell.
  vec3 delta_t_xyz = CELL_SIDE / abs( exit_pos - entry_pos );

  // Main loop. Visits cells until last cell reached.
  for ( int steps = 0; steps < MAX_STEPS; ++steps ) {
    // TODO Visit Cell (i,j,k)

    if ( t_x <= t_y && t_x <= t_z ) {
      if ( cell_ijk.x == end_cell_ijk.x ) { break; }
      t_x += delta_t_xyz.x;
      cell_ijk.x += step_dir_ijk.x;
    } else if ( t_y <= t_x && t_y <= t_z ) {
      if ( cell_ijk.y == end_cell_ijk.y ) { break; }
      t_y += delta_t_xyz.y;
      cell_ijk.y += step_dir_ijk.y;
    } else {
      if ( cell_ijk.z == end_cell_ijk.z ) { break; }
      t_z += delta_t_xyz.z;
      cell_ijk.z += step_dir_ijk.z;
    }
  }
}

int find_nearest( in vec3 entry_pos, in vec3 rd, in float t_entry, in ivec3 n_cells, out float t_end, out vec3 vox_n ) {
  const float CELL_SIDE = 16.0; // 32 / (grid max - grid_min) 
  entry_pos = entry_pos * CELL_SIDE;
  
  
  ivec3 cell_ijk = clamp( ivec3( floor( entry_pos ) ), ivec3( 0 ), ivec3( n_cells - 1 ) ); // NOTE(Anton) scaling difference btw this and 


  /* Get our traversal constants */
  ivec3 step  = ivec3( sign( rd ) );
  vec3 t_delta = abs( 1.0 / rd );

  /* IMPORTANT: Safety clamp the entry point inside the grid */
  //ivec3 pos = clamp( ivec3( floor( entry_pos ) ), ivec3( 0 ), ivec3( n_cells - 1 ) ); // BUGFIX: upper bound from n_cells to n_cells-1.

  /* Initialize the time along the ray when each axis crosses its next cell boundary */
  vec3 t = ( cell_ijk - entry_pos + max( step, 0 ) ) / rd;

  vox_n = v_n_loc; // TODO(Anton) Encode the normal in the box vertex.

  int axis = 0;
  for ( int steps = 0; steps < MAX_STEPS; ++steps ) {
    /* Fetch the cell at our current position */
    vec3 rst = vec3( cell_ijk ) / vec3( n_cells - 1 ); // BUGFIX: off by 1 was creating extra row on the bottom.
    rst = clamp( rst, vec3(0.0), vec3(1.0) );
    uvec4 itexel = texture( u_vol_tex, vec3( rst.x, 1.0 - rst.y, rst.z ) );

    /* Check if we hit a voxel which isn't 0 */
    if ( itexel.r > 0 ) {                         // Palette index 0 treated as air.
      if ( steps == 0 ) {
        t_end = t_entry;
        return int( itexel.r );
      }

      /* Return the time of intersection! */
      t_end = t_entry + ( t[axis] - t_delta[axis] ) / CELL_SIDE;
      return  int( itexel.r );
    }

    /* Step on the axis where `tmax` is the smallest */
    if ( t.x <= t.y && t.x <= t.z ) {
      cell_ijk.x += step.x; // i
      if ( cell_ijk.x < 0 || cell_ijk.x >= n_cells.x ) break;
      t.x += t_delta.x;
      axis = 0;
      vox_n = vec3( 1.0, 0.0, 0.0 ) * -step; // The *step is to get -1 on the reverse sides.
    } else if ( t.y <= t.x && t.y <= t.z ) {
      cell_ijk.y += step.y; // j
      if ( cell_ijk.y < 0 || cell_ijk.y >= n_cells.y ) break;
      t.y += t_delta.y;
      axis = 1;
      vox_n = vec3( 0.0, 1.0, 0.0 ) * -step;
    } else {
      cell_ijk.z += step.z; // k
      if ( cell_ijk.z < 0 || cell_ijk.z >= n_cells.z ) break;
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

vec3 lighting( in vec3 p_wor, in vec3 n_wor, in vec3 k_diffuse, vec3 k_ambient, in Light l ) {

  vec3 i_ambient = l.ambient * k_ambient;

  vec3 dist_p_to_l_pos_wor = l.pos_wor - p_wor;
	vec3 dir_p_to_l_pos_wor = normalize( dist_p_to_l_pos_wor );
  vec3 i_diffuse = l.diffuse * k_diffuse * max( dot( dir_p_to_l_pos_wor, n_wor ), 0.0 );

  return i_diffuse + i_ambient;
}

void main() {
  vec3 grid_max_loc = vec3( 1.0 ) * u_shape;
  vec3 grid_min_loc = vec3( -1.0 ) * u_shape;
  vec3 ray_o_loc          = v_ray_o_loc;
  vec3 ray_dist_3d_loc    = v_ray_dist_3d_loc; // v_pos_loc - ray_o_loc;
  float t_entry           = length( ray_dist_3d_loc );

  float inside = insideBoxOriginal( ray_o_loc, grid_min_loc, grid_max_loc );
  t_entry *= ( 1.0 - inside );

  vec3 ray_d_loc       = normalize( ray_dist_3d_loc );
  float t_end      = 0.0;

  vec3 vox_n = vec3( 0.0 );
  vec3 entry_pos       = ( ray_o_loc + ray_d_loc * t_entry ) - grid_min_loc; // BUGFIX: +0.001 was introducing an artifact (line on corners).
  int pal_idx_of_nearest = find_nearest( entry_pos, ray_d_loc, t_entry, u_n_cells, t_end, vox_n );
  int vis = pal_idx_of_nearest ;
  
  // Note that even conditional use of discard will mean that the FS will turn off early depth tests. 
  if ( 0 == vis ) {
    //gl_FragDepth = 0.0;
    discard; // faster without.
  }

  // NOTE(Anton) off-by-one error for palette index for Magicavoxel models. Not sure if my bug (e.g. fetch at cell border) or an unexpected convention where first index=1 instead of 0.
  vec4 texel = texelFetch( u_pal_tex, pal_idx_of_nearest - 1, 0 ); // Note had to convert uvec to int type (uint not okay).
  texel = vec4( pow( texel.rgb, vec3( 2.2 ) ), texel.a );

  Light lights[3] = Light[3](
    Light( vec3( 2.0, 6.0, 10.0 ), vec3( 0.65 ), vec3( 0.7 ) ),
    Light( vec3( -2.0, 5.0, -20.0 ), vec3( 0.7 ), vec3( 0.5 ) ),
    Light( vec3( -1, 20.0, 1.0 ), vec3( 0.8 ), vec3( 0.5 ) )
  );

  vec3 k_ambient = vec3( texel.rgb * 0.1 );
  vec3 lit_rgb = vec3( 0.0 );
  
  vec4 p_wor = u_M * vec4( ray_o_loc + ray_d_loc * t_end, 1.0 );
  vec4 n_wor = u_M * vec4( vox_n, 0.0 );
  for ( int i = 0; i < 3; i++ ) {
    lit_rgb += lighting( p_wor.xyz, n_wor.xyz, texel.rgb, k_ambient, lights[i] );
  }

  vec3 rgb = lit_rgb;
  rgb = clamp( rgb, vec3( 0.01 ), vec3( 1.0 ) );

  vec4 p_clip = u_P * u_V * p_wor;
  gl_FragDepth = p_clip.z / p_clip.w;

  frag_colour.rgb = rgb;
  frag_colour.rgb = pow( frag_colour.rgb, vec3( 1.0 / 2.2 ) ); // gamma correct.
  frag_colour.a   = 1.0;

  //float depth = gl_FragDepth;
  //float zfar = 1000.0;
  //float znear = 0.01;
  //float linear_depth = (2.0 * znear * zfar)/(zfar + znear - (depth * 2.0 - 1.0) * (zfar-znear)) / zfar;
 // frag_colour.rgb = vec3( 1.0 - linear_depth );
 // frag_colour.rgb = n_wor.xyz;
 //frag_colour.rgb = texel.rgb;
}
