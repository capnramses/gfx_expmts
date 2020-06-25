// left-to-right wipe-in shader
// GLSL
#version 410 core

in vec2 a_vp;

uniform vec2 u_scale, u_pos;
uniform mat4 u_M;

out vec2 v_st;

void main() {
  v_st = a_vp.xy * 0.5 + 0.5;
  v_st.t = 1.0 - v_st.t;
  vec4 pos = u_M * vec4( a_vp * u_scale, 0.0, 1.0 );
  pos.xy += u_pos;
  gl_Position = pos;
}
