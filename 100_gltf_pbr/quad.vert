#version 410 core

in vec2 a_vp;

out vec2 v_st;

void main() {
	v_st   = a_vp.xy * 0.5 + 0.5;
	v_st.t = 1.0 - v_st.t;
	gl_Position = vec4( a_vp, 0.0, 1.0);
}
