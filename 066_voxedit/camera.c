// DWARF 3-D Source Code
// Copyright Anton Gerdelan <antongdl@protonmail.com>. 2018
#include "camera.h"
#include <assert.h>

bool g_freeze_frustum;
float g_mouse_turn_cam_sensitivity = 500.0f;

void recalc_cam_V( camera_t* cam ) {
  assert( cam );

  versor qx = quat_from_axis_deg( cam->x_degs, cam->right_mv_dir );
  versor qy = quat_from_axis_deg( cam->y_degs, ( vec3 ){ 0, 1, 0 } );
  cam->ori  = mult_quat_quat( qx, qy );

  mat4 iT             = translate_mat4( ( vec3 ){ -cam->pos.x, -cam->pos.y, -cam->pos.z } );
  mat4 iRx            = rot_x_deg_mat4( -cam->x_degs );
  mat4 iRy            = rot_y_deg_mat4( -cam->y_degs );
  mat4 iR             = mult_mat4_mat4( iRx, iRy );
  mat4 Rx             = rot_x_deg_mat4( cam->x_degs );
  mat4 Ry             = rot_y_deg_mat4( cam->y_degs );
  cam->R              = mult_mat4_mat4( Ry, Rx );
  cam->V              = mult_mat4_mat4( iR, iT );
  cam->PV             = mult_mat4_mat4( cam->P, cam->V );
  cam->forward_mv_dir = v3_v4( mult_mat4_vec4( Ry, ( vec4 ){ 0, 0, -1, 0 } ) );
  cam->right_mv_dir   = v3_v4( mult_mat4_vec4( Ry, ( vec4 ){ 1, 0, 0, 0 } ) );
  cam->forward        = v3_v4( mult_mat4_vec4( cam->R, ( vec4 ){ 0, 0, -1, 0 } ) );
  cam->right          = v3_v4( mult_mat4_vec4( cam->R, ( vec4 ){ 1, 0, 0, 0 } ) );

  if ( !g_freeze_frustum ) { re_extract_frustum_planes( cam->fovy_deg, cam->aspect, cam->neard, cam->fard, cam->pos, cam->V ); }
}

void recalc_cam_P( camera_t* cam, float aspect ) {
  assert( cam );

  cam->aspect = aspect;
  cam->P      = perspective( cam->fovy_deg, aspect, cam->neard, cam->fard );
  cam->PV     = mult_mat4_mat4( cam->P, cam->V );
}

void move_cam_fwd( camera_t* cam, double seconds ) {
  assert( cam );

  vec3 displ = mult_vec3_f( cam->forward_mv_dir, cam->speed * seconds );
  cam->pos   = add_vec3_vec3( cam->pos, displ );
}

void move_cam_bk( camera_t* cam, double seconds ) {
  assert( cam );

  vec3 displ = mult_vec3_f( cam->forward_mv_dir, -cam->speed * seconds );
  cam->pos   = add_vec3_vec3( cam->pos, displ );
}

void move_cam_lft( camera_t* cam, double seconds ) {
  assert( cam );

  vec3 displ = mult_vec3_f( cam->right_mv_dir, -cam->speed * seconds );
  cam->pos   = add_vec3_vec3( cam->pos, displ );
}

void move_cam_rgt( camera_t* cam, double seconds ) {
  assert( cam );

  vec3 displ = mult_vec3_f( cam->right_mv_dir, cam->speed * seconds );
  cam->pos   = add_vec3_vec3( cam->pos, displ );
}

void move_cam_up( camera_t* cam, double seconds ) {
  assert( cam );

  vec3 displ = mult_vec3_f( ( vec3 ){ 0, 1, 0 }, cam->speed * seconds );
  cam->pos   = add_vec3_vec3( cam->pos, displ );
}

void move_cam_dn( camera_t* cam, double seconds ) {
  assert( cam );

  vec3 displ = mult_vec3_f( ( vec3 ){ 0, 1, 0 }, -cam->speed * seconds );
  cam->pos   = add_vec3_vec3( cam->pos, displ );
}

void turn_cam_left( camera_t* cam, double seconds ) {
  assert( cam );

  cam->y_degs += seconds * cam->rot_speed;
}

void turn_cam_right( camera_t* cam, double seconds ) {
  assert( cam );

  cam->y_degs -= seconds * cam->rot_speed;
}

void turn_cam_leftright_mouse( camera_t* cam, float factor_delta_x ) {
  assert( cam );

  cam->y_degs += -factor_delta_x * g_mouse_turn_cam_sensitivity;
}

void turn_cam_up( camera_t* cam, double seconds ) {
  assert( cam );

  cam->x_degs += seconds * cam->rot_speed;
  cam->x_degs = CLAMP( cam->x_degs, -90.0f, 90.0f );
}

void turn_cam_down( camera_t* cam, double seconds ) {
  assert( cam );

  cam->x_degs -= seconds * cam->rot_speed;
  cam->x_degs = CLAMP( cam->x_degs, -90.0f, 90.0f );
}

camera_t create_cam( float aspect ) {
  camera_t cam;
  memset( &cam, 0, sizeof( camera_t ) );
  cam.pos           = ( vec3 ){ 3.5f, 4.0f, 8.0f };
  cam.cam_desired_y = 4.0f;
  cam.x_degs        = -45;
  cam.ori           = quat_from_axis_deg( cam.x_degs, ( vec3 ){ 1, 0, 0 } );
  cam.speed         = 1.0f;
  cam.rot_speed     = 100.0f;
  cam.neard         = 0.01f;
  cam.fard          = 100.0f;
  cam.fovy_deg      = 66.0f;
  cam.P             = perspective( cam.fovy_deg, aspect, cam.neard, cam.fard );
  cam.aspect        = aspect;
  cam.animating_y   = false;
  return cam;
}

void cam_set_pos_and_look_at( camera_t* cam, vec3 target ) {
  assert( cam );

  // NOTE(Anton) this +1 assumes the x0.1 scaling of voxels thats only in voxel vert shader
  cam->pos.x = target.x;
  cam->pos.y = target.y; // assumes standard camera angle
  cam->pos.z = target.z + 1;

  cam->x_degs = -45;
  cam->y_degs = 0;
  cam->z_degs = 0;

  recalc_cam_V( cam );
}

void animate_y_transition( camera_t* cam, double seconds ) {
  assert( cam );

  if ( !cam->animating_y ) { return; }
  if ( 0.0 == seconds ) { return; }

  float diff             = cam->cam_desired_y - cam->pos.y;
  const float anim_speed = 1.0f;
  if ( fabs( diff ) < anim_speed * seconds ) {
    cam->pos.y       = cam->cam_desired_y;
    cam->animating_y = false;
    return;
  }
  if ( diff > 0.0f ) {
    cam->pos.y += anim_speed * seconds;
  } else {
    cam->pos.y -= anim_speed * seconds;
  }
}

//////////////////////////////////////////////// frustum ///////////////////////////////////////////////////

// all normals
vec3 g_n_right;
vec3 g_n_left;
vec3 g_n_top;
vec3 g_n_bottom;
vec3 g_n_near;
vec3 g_n_far;
// clip planes
vec3 g_fcp;
vec3 g_ncp;
// get 8 corner points
vec3 ftl;
vec3 ftr;
vec3 fbl;
vec3 fbr;
vec3 ntl;
vec3 ntr;
vec3 nbl;
vec3 nbr;

vec3 g_f_cam_pos;

void re_extract_frustum_planes( float fovy_deg, float aspect, float neard, float fard, vec3 cam_pos, mat4 V ) {
  // TODO(Anton) -- really? - use fast inverse if have to
  V = inverse_mat4( V );

  g_f_cam_pos = cam_pos;
  // forward, right, and up for current orientation
  vec4 fwd_loc   = ( vec4 ){ 0.0f, 0.0f, -1.0f, 0.0f };
  vec4 fwd_worv4 = mult_mat4_vec4( V, fwd_loc );
  vec3 fwd_wor   = ( vec3 ){ fwd_worv4.x, fwd_worv4.y, fwd_worv4.z };
  vec4 up_loc    = ( vec4 ){ 0.0f, 1.0, 0.0f, 0.0f };
  vec4 up_worv4  = mult_mat4_vec4( V, up_loc );
  vec3 up_wor    = ( vec3 ){ up_worv4.x, up_worv4.y, up_worv4.z };
  vec3 right_wor = cross_vec3( fwd_wor, up_wor );

  // get dimensions of planes
  float fov_in_rad = ONE_DEG_IN_RAD * fovy_deg;
  // was fovy * 0.5 but i think 45 degrees already is 1/2
  float near_height = 2.0f * tanf( fov_in_rad * 0.5f ) * neard;
  float near_width  = near_height * aspect;
  float far_height  = 2.0f * tanf( fov_in_rad * 0.5f ) * fard;
  float far_width   = far_height * aspect;

  // get far and near centre points
  g_fcp = add_vec3_vec3( cam_pos, mult_vec3_f( fwd_wor, fard ) );
  g_ncp = add_vec3_vec3( cam_pos, mult_vec3_f( fwd_wor, neard ) );
  // get 8 corner points
  ftl = sub_vec3_vec3( add_vec3_vec3( g_fcp, mult_vec3_f( up_wor, far_height * 0.5 ) ), mult_vec3_f( right_wor, far_width * 0.5f ) );
  ftr = add_vec3_vec3( add_vec3_vec3( g_fcp, mult_vec3_f( up_wor, far_height * 0.5 ) ), mult_vec3_f( right_wor, far_width * 0.5f ) );
  fbl = sub_vec3_vec3( sub_vec3_vec3( g_fcp, mult_vec3_f( up_wor, far_height * 0.5 ) ), mult_vec3_f( right_wor, far_width * 0.5f ) );
  fbr = add_vec3_vec3( sub_vec3_vec3( g_fcp, mult_vec3_f( up_wor, far_height * 0.5 ) ), mult_vec3_f( right_wor, far_width * 0.5f ) );
  ntl = sub_vec3_vec3( add_vec3_vec3( g_ncp, mult_vec3_f( up_wor, near_height * 0.5 ) ), mult_vec3_f( right_wor, near_width * 0.5f ) );
  ntr = add_vec3_vec3( add_vec3_vec3( g_ncp, mult_vec3_f( up_wor, near_height * 0.5 ) ), mult_vec3_f( right_wor, near_width * 0.5f ) );
  nbl = sub_vec3_vec3( sub_vec3_vec3( g_ncp, mult_vec3_f( up_wor, near_height * 0.5 ) ), mult_vec3_f( right_wor, near_width * 0.5f ) );
  nbr = add_vec3_vec3( sub_vec3_vec3( g_ncp, mult_vec3_f( up_wor, near_height * 0.5 ) ), mult_vec3_f( right_wor, near_width * 0.5f ) );
  // get side and top planes' normals
  vec3 fa_hat = normalise_vec3( sub_vec3_vec3( add_vec3_vec3( g_ncp, mult_vec3_f( right_wor, near_width * 0.5f ) ), cam_pos ) );
  vec3 fb_hat = normalise_vec3( sub_vec3_vec3( sub_vec3_vec3( g_ncp, mult_vec3_f( right_wor, near_width * 0.5f ) ), cam_pos ) );
  vec3 fc_hat = normalise_vec3( sub_vec3_vec3( add_vec3_vec3( g_ncp, mult_vec3_f( up_wor, near_height * 0.5f ) ), cam_pos ) );
  vec3 fd_hat = normalise_vec3( sub_vec3_vec3( sub_vec3_vec3( g_ncp, mult_vec3_f( up_wor, near_height * 0.5f ) ), cam_pos ) );
  // all normals
  g_n_right  = normalise_vec3( cross_vec3( up_wor, fa_hat ) );
  g_n_left   = normalise_vec3( cross_vec3( fb_hat, up_wor ) );
  g_n_top    = normalise_vec3( cross_vec3( fc_hat, right_wor ) );
  g_n_bottom = normalise_vec3( cross_vec3( right_wor, fd_hat ) );
  g_n_near   = normalise_vec3( fwd_wor );
  g_n_far    = normalise_vec3( mult_vec3_f( fwd_wor, -1.0f ) );
}

bool is_sphere_in_frustum( vec3 centre, float radius ) {
  bool out_of_planes = false;
  float dist         = 0.0f;

  dist = dot_vec3( g_n_right, centre ) - dot_vec3( g_n_right, g_f_cam_pos );
  if ( dist + radius < 0.0f ) { out_of_planes = true; }

  dist = dot_vec3( g_n_left, centre ) - dot_vec3( g_n_left, g_f_cam_pos );
  if ( dist + radius < 0.0f ) { out_of_planes = true; }

  dist = dot_vec3( g_n_top, centre ) - dot_vec3( g_n_top, g_f_cam_pos );
  if ( dist + radius < 0.0f ) { out_of_planes = true; }

  dist = dot_vec3( g_n_bottom, centre ) - dot_vec3( g_n_bottom, g_f_cam_pos );
  if ( dist + radius < 0.0f ) { out_of_planes = true; }

  dist = dot_vec3( g_n_near, centre ) - dot_vec3( g_n_near, g_ncp );
  if ( dist + radius < 0.0f ) { out_of_planes = true; }

  dist = dot_vec3( g_n_far, centre ) - dot_vec3( g_n_far, g_fcp );
  if ( dist + radius < 0.0f ) { out_of_planes = true; }

  if ( out_of_planes ) { return false; }

  return true;
}

// one plane from AABB v frustum check
static bool _compare_plane_aab( vec3 mins, vec3 maxs, vec3 plane, vec3 point_on_p ) {
  vec3 farthest_point;
  if ( plane.x < 0.0f ) {
    farthest_point.x = mins.x;
  } else {
    farthest_point.x = maxs.x;
  }
  if ( plane.y < 0.0f ) {
    farthest_point.y = mins.y;
  } else {
    farthest_point.y = maxs.y;
  }
  if ( plane.z < 0.0f ) {
    farthest_point.z = mins.z;
  } else {
    farthest_point.z = maxs.z;
  }
  // 3d distance from a point on plane to farthest point of AABB
  vec3 dist = sub_vec3_vec3( farthest_point, point_on_p );
  // component of this distance in direction of plane normal
  float nd = dot_vec3( plane, dist );
  if ( nd < 0.0 ) { return false; }
  return true;
}

/* for each frustum plane
 * assemble farthest x,y, and z box corner point in direction of plane
 * if that point is behind plane - cull (return false)
 * if all 6 farthest xyz points are in front of their planes - return true
 */
bool is_aabb_in_frustum( vec3 mins, vec3 maxs ) {
  vec3 temp = mins;

  // TODO(Anton) SWAP(x,y)
  if ( mins.x > maxs.x ) {
    mins.x = maxs.x;
    maxs.x = temp.x;
  }
  if ( mins.y > maxs.y ) {
    mins.y = maxs.y;
    maxs.y = temp.y;
  }
  if ( mins.z > maxs.z ) {
    mins.z = maxs.z;
    maxs.z = temp.z;
  }

  if ( !_compare_plane_aab( mins, maxs, g_n_near, ntr ) ) { return false; }
  if ( !_compare_plane_aab( mins, maxs, g_n_right, ntr ) ) { return false; }
  if ( !_compare_plane_aab( mins, maxs, g_n_left, ntl ) ) { return false; }
  if ( !_compare_plane_aab( mins, maxs, g_n_top, ntl ) ) { return false; }
  if ( !_compare_plane_aab( mins, maxs, g_n_bottom, nbl ) ) { return false; }
  if ( !_compare_plane_aab( mins, maxs, g_n_far, fbl ) ) { return false; }

  return true;
}
