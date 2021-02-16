#version 410 core

in vec3 a_vp;
in vec3 a_vn;
in vec2 a_vt;
in vec3 a_instanced_pos; // world positions
uniform mat4 u_P, u_V, u_M;
out vec2 v_st;

void main() {
  v_st = a_vt;
  gl_Position = u_P * u_V *  u_M * vec4( a_vp + a_instanced_pos * 2.0, 1.0  );
}
