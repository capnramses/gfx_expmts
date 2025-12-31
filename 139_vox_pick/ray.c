#include "ray.h"
#include <assert.h>
#include <float.h>
#include <math.h>

// bool ray_aabb() { return false; }

// bool ray_obb() { return false; }

void ray_uniform_3d_grid( vec3 entry_xyz, vec3 exit_xyz, const float cell_side, bool ( *visit_cell_cb )( int i, int j, int k, void* user_ptr ), void* data_ptr ) {
  // Current grid cell (starts at entry point).
  int i = (int)floorf( entry_xyz.x / cell_side );
  int j = (int)floorf( entry_xyz.y / cell_side );
  int k = (int)floorf( entry_xyz.z / cell_side );

  // Exit point grid cell.
  int i_end = (int)floorf( exit_xyz.x / cell_side );
  int j_end = (int)floorf( exit_xyz.y / cell_side );
  int k_end = (int)floorf( exit_xyz.z / cell_side );

  // Determine primary direction to step.
  int step_dir_i = ( ( entry_xyz.x < exit_xyz.x ) ? 1 : ( ( entry_xyz.x > exit_xyz.x ) ? -1 : 0 ) );
  int step_dir_j = ( ( entry_xyz.y < exit_xyz.y ) ? 1 : ( ( entry_xyz.y > exit_xyz.y ) ? -1 : 0 ) );
  int step_dir_k = ( ( entry_xyz.z < exit_xyz.z ) ? 1 : ( ( entry_xyz.z > exit_xyz.z ) ? -1 : 0 ) );

  // Determine t_x, and t_y; the values of t at which the line segment crosses the first horizontal and vertical cell boundaries, respectively.
  float min_x = cell_side * floorf( entry_xyz.x / cell_side ), max_x = min_x + cell_side;
  float min_y = cell_side * floorf( entry_xyz.y / cell_side ), max_y = min_y + cell_side;
  float min_z = cell_side * floorf( entry_xyz.z / cell_side ), max_z = min_z + cell_side;
  float t_x = ( ( entry_xyz.x > exit_xyz.x ) ? ( entry_xyz.x - min_x ) : ( max_x - entry_xyz.x ) ) / fabs( exit_xyz.x - entry_xyz.x ); //
  float t_y = ( ( entry_xyz.y > exit_xyz.y ) ? ( entry_xyz.y - min_y ) : ( max_y - entry_xyz.y ) ) / fabs( exit_xyz.y - entry_xyz.y ); //
  float t_z = ( ( entry_xyz.z > exit_xyz.z ) ? ( entry_xyz.z - min_z ) : ( max_z - entry_xyz.z ) ) / fabs( exit_xyz.z - entry_xyz.z ); //

  // Determine deltax/deltay; how far, in units of t, one must step along the line segment for the horizontal/vertical movement, respectively, to equal
  // width/height of a cell.
  float delta_x = cell_side / fabs( exit_xyz.x - entry_xyz.x );
  float delta_y = cell_side / fabs( exit_xyz.y - entry_xyz.y );
  float delta_z = cell_side / fabs( exit_xyz.z - entry_xyz.z );

  // Main loop. Visit cells until last cell on segment reached, or supplied visit_cell_cb function returns false.
  for ( ;; ) {
    if ( !visit_cell_cb( i, j, k, data_ptr ) ) { return; }
    if ( t_x < t_y && t_x < t_z ) {
      if ( i == i_end ) { break; }
      t_x += delta_x;
      i += step_dir_i;
    } else if ( t_y < t_x && t_y < t_z ) {
      if ( j == j_end ) { break; }
      t_y += delta_y;
      j += step_dir_j;
    } else {
      if ( k == k_end ) { break; }
      t_z += delta_z;
      k += step_dir_k;
    }
  } // endfor
}

vec3 ray_wor_from_mouse( float mouse_x, float mouse_y, int w, int h, mat4 inv_P, mat4 inv_V ) {
  vec4 ray_eye = mul_mat4_vec4( inv_P, (vec4){ ( 2.0f * mouse_x ) / (float)w - 1.0f, 1.0f - ( 2.0f * mouse_y ) / (float)h, -1.0f, 0.0f } );
  return normalise_vec3( vec3_from_vec4( mul_mat4_vec4( inv_V, (vec4){ ray_eye.x, ray_eye.y, -1.0f, 0.0f } ) ) );
}

bool ray_obb2( obb_t box, vec3 ray_o, vec3 ray_d, float* t, int* face_num, float* t2 ) {
  assert( t );
  *t             = 0.0f;
  float tmin     = -INFINITY;
  float tmax     = INFINITY;
  int slab_min_i = 0, slab_max_i = 0;
  vec3 p = sub_vec3_vec3( box.centre, ray_o );
  for ( int i = 0; i < 3; i++ ) { // 3 "slabs" (pair of front/back planes)
    float e = dot_vec3( box.norm_side_dir[i], p );
    float f = dot_vec3( box.norm_side_dir[i], ray_d );
    if ( fabs( f ) > FLT_EPSILON ) {
      float t1    = ( e + box.half_lengths[i] ) / f; // intersection on front
      float t2    = ( e - box.half_lengths[i] ) / f; // and back side of slab
      int t1_side = 1;                               // t1 is front
      if ( t1 > t2 ) {
        float tmp = t1;
        t1        = t2;
        t2        = tmp;
        t1_side   = -1; // t1 is back face of slab (opposing the slab normal)
      }
      if ( t1 > tmin ) {
        tmin       = t1;
        slab_min_i = ( i + 1 ) * t1_side;
      }
      if ( t2 < tmax ) {
        tmax       = t2;
        slab_max_i = ( i + 1 ) * -t1_side;
      }
      if ( tmin > tmax ) { return false; }
      if ( tmax < 0 ) { return false; }
    } else if ( -e - box.half_lengths[i] > 0 || -e + box.half_lengths[i] < 0 ) {
      return false;
    }
  }
  //*t = tmin > 0 ? tmin : tmax;
  if ( tmin > 0 ) {
    *t  = tmin;
    *t2 = tmax;
  } else {
    // NOTE(Anton) guess. i think this is the case where ray origin is inside the cube and wall 1 is behind us, so we want to start t at origin.
    // this differs from default t (commented out, above).
    *t2 = tmax;
    *t  = 0;
  }
  *face_num = tmin > 0 ? slab_min_i : slab_max_i; // max is back side of slab (opposing face)
  return true;
}