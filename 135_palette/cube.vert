#version 410 core

in vec3 a_vp;

uniform mat4 u_P, u_V, u_M;

out vec3 v_pos_wor;

void main() {
  v_pos_wor = ( u_M * vec4( a_vp, 1.0 ) ).xyz;
  gl_Position = u_P * u_V * vec4( v_pos_wor, 1.0 );
}
