#version 410

// Pixel art vertex shader.
// by Anton Gerdelan, Feb 2023

in vec3 a_vp;
in int a_instanced;     // codepoint
in int a_instanced_rgb; // spacing

uniform mat4 u_M;

out vec2 v_st;
flat out int v_codepoint;
flat out int v_spacing;

void main() {
	v_st = a_vp.xy * 0.5 + 0.5;
	v_st.t = 1.0 - v_st.t;
	v_codepoint = a_instanced;
	v_spacing   = a_instanced_rgb;
	vec3 vp = a_vp / 16.0;
	vp.x += v_spacing / 200.0;
	gl_Position = u_M * vec4( vp, 1.0 );
}
