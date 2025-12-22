#version 410 core

in vec3 a_vp;

uniform mat4 u_P, u_V, u_M;

out vec3 v_pos_loc;

void main() {
  v_pos_loc = a_vp;
  gl_Position = u_P * u_V * u_M * vec4( v_pos_loc, 1.0 );
}
