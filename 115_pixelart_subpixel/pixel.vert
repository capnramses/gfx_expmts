#version 410

// Pixel art vertex shader.
// by Anton Gerdelan, Feb 2023

in vec3 a_vp;

uniform mat4 u_M;

out vec2 v_st;

void main() {
	v_st = a_vp.xy * 0.5 + 0.5;
	v_st.t = 1.0 - v_st.t;
	gl_Position = u_M * vec4( a_vp, 1.0 );
}
