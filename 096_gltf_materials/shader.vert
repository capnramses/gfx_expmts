 #version 410 core

in vec3 a_vp;
in vec2 a_vt;
in vec3 a_vc;

uniform mat4 u_P, u_V, u_M;

out vec2 v_st;
out vec3 v_rgb;

void main () {
  v_st = a_vt;
  v_rgb = a_vc;
  gl_Position = u_P * u_V * u_M * vec4( a_vp * 0.1, 1.0 );
}
