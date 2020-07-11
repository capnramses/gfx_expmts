#version 410 core

in vec3 a_pos;
in vec3 a_instanced; // initial velocities

uniform mat4 u_P, u_V, u_M;
uniform float u_time;

const float mesh_scale = 0.05;

void main() {
  // v_f = v_i^2 + 2 * a * d
  // v_f = v_i + a * t -- eg gravity
  // determine peak height: y = v_iy * t + 0.5 * g * t^2
  // d = v_i * t + 0.5a * t^2 -- displacement of ~horizontally launched projectile
  const vec3 g = vec3( 0.0, -10.0, 0.0 );
  vec3 d = a_instanced * u_time + g * 0.5 * u_time * u_time;
  vec3 pos_loc = a_pos * mesh_scale + d;
  gl_Position = u_P * u_V * u_M * vec4( pos_loc, 1.0 );
}
