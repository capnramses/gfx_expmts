#version 410 core

in vec3 a_vp;
in vec3 a_vn;
in vec2 a_vt;

uniform mat4 u_P, u_V, u_M;

out vec3 v_n;
out vec2 v_st;

void main() {
  v_n = a_vn;
  v_st = a_vt;
  gl_Position = u_P * u_V * u_M * vec4( a_vp, 1.0 );
}
