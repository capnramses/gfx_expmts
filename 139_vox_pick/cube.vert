#version 410 core

in vec3 a_vp;
in vec3 a_vn;

uniform mat4 u_P, u_V, u_M, u_M_inv;
uniform vec3 u_cam_pos_wor;

out vec3 v_pos_loc;
out vec3 v_n_loc;
out vec3 v_ray_o_loc;
out vec3 v_ray_dist_3d_loc;

void main() {
  v_pos_loc         = a_vp;
  v_n_loc = a_vn;
  v_ray_o_loc       = ( u_M_inv * vec4( u_cam_pos_wor, 1.0 ) ).xyz;
  v_ray_dist_3d_loc = v_pos_loc - v_ray_o_loc;
  gl_Position       = u_P * u_V * u_M * vec4( v_pos_loc, 1.0 );
}
