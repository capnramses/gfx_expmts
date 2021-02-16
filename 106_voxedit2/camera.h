#pragma once
#include "apg_maths.h"
#include <stdbool.h>

typedef struct camera_t {
  mat4 P, V, PV, R; // NOTE(Anton): be careful to always recalc PV whenever P or V changes
  versor ori;
  vec3 pos;
  // actual camera axes
  vec3 forward, right;
  // RTS-style crane-shot movement axes
  vec3 forward_mv_dir, right_mv_dir;

  float speed;
  float rot_speed;
  float x_degs, y_degs, z_degs;
  float neard, fard;
  float fovy_deg;
  float aspect;

  float cam_desired_y; // for animating between level transitions
  bool animating_y;
} camera_t;

// =================================================================================================------------------
// MAIN INTERFACE
// =================================================================================================------------------
camera_t create_cam( float aspect );

void cam_set_pos_and_look_at( camera_t* cam, vec3 target );

void move_cam_fwd( camera_t* cam, double seconds );
void move_cam_bk( camera_t* cam, double seconds );
void move_cam_lft( camera_t* cam, double seconds );
void move_cam_rgt( camera_t* cam, double seconds );
void move_cam_up( camera_t* cam, double seconds );
void move_cam_dn( camera_t* cam, double seconds );
void turn_cam_left( camera_t* cam, double seconds );
void turn_cam_right( camera_t* cam, double seconds );
void turn_cam_leftright_mouse( camera_t* cam, float factor_delta_x );
void turn_cam_up( camera_t* cam, double seconds );
void turn_cam_down( camera_t* cam, double seconds );
void animate_y_transition( camera_t* cam, double seconds );

// call once after any turns etc during frame
void recalc_cam_V( camera_t* cam );
void recalc_cam_P( camera_t* cam, float aspect );

// =================================================================================================------------------
// FRUSTUM
// =================================================================================================------------------
void re_extract_frustum_planes( float fovy_deg, float aspect, float neard, float fard, vec3 cam_pos, mat4 V );
bool is_sphere_in_frustum( vec3 centre, float radius );
bool is_aabb_in_frustum( vec3 mins, vec3 maxs );

// =================================================================================================------------------
// EXTERNAL GLOBALS
// =================================================================================================------------------
extern bool g_freeze_frustum;
extern float g_mouse_turn_cam_sensitivity;
