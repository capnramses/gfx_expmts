#version 410

// Pixel art vertex shader.
// by Anton Gerdelan, Feb 2023

in vec2 a_vp;
in vec2 a_st;
in float a_vpal_idx;

uniform mat4 u_M;
uniform vec2 u_fb_dims;
uniform vec2 u_pos, u_scale;

out vec2 v_st;
out float v_palidx;

void main() {
	v_palidx = a_vpal_idx;
	v_st = a_st;
	v_st.t = 1.0 - v_st.t;
	float aspect = u_fb_dims.x / u_fb_dims.y;
	vec2 pos = a_vp;
	pos += vec2( u_pos.x * 0.5, u_pos.y * 0.5 );
	pos *= u_scale;
	pos.x /= aspect;
	gl_Position = u_M * vec4( pos, 0.0, 1.0 );
}
