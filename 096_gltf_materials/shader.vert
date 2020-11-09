 #version 410 core

in vec3 a_vp;
in vec2 a_vt;
in vec3 a_vc;

uniform mat4 u_P, u_V, u_M;

out vec2 v_st;
out vec3 v_rgb;

void main () {
  v_st = a_vt;
  //v_st.y -= 1.0; // NOTE: in Damaged helmet the range of t is min/max 1.99 to 1.0. weird. i just set texture params to REPEAT. perhaps this is the default sampler for glTF
  v_rgb = a_vc;
  gl_Position = u_P * u_V * u_M * vec4( a_vp, 1.0 );
}
