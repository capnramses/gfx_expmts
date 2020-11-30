#version 410 core

in vec3 a_vp;
in vec2 a_vt;
in vec3 a_vn;
in vec3 a_vc;

uniform mat4 u_P, u_V, u_M;

out vec3 v_p_wor;
out vec2 v_st;
out vec3 v_n_wor;

void main() {

	v_p_wor = ( u_M * vec4( a_vp, 1.0 ) ).xyz;
	v_st = a_vt;
	v_n_wor = ( u_M * vec4( a_vn, 0.0 ) ).xyz;

	gl_Position = u_P * u_V * vec4( v_p_wor, 1.0 );
}
