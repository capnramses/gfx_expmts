// Dither experiment
// Anton Gerdelan
// GLSL
// 26 Oct 2021

#version 410 core

in vec2 a_vp;

uniform vec2 u_scale, u_pos, u_texcoord_scale;

out vec2 v_st;
out vec4 v_pos_clip;

void main() {
	v_st = a_vp.xy * 0.5 + 0.5;
	v_st.t = 1.0 - v_st.t;
	v_st *= u_texcoord_scale;
	v_pos_clip  = vec4( a_vp * u_scale + u_pos, 0.0, 1.0 );
	gl_Position = v_pos_clip;
}
