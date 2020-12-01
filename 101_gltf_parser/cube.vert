#version 410 core

in vec3 a_vp;
uniform mat4 u_P, u_V;
out vec3 v_st;

void main () {
	v_st = a_vp;
	gl_Position = u_P * u_V * vec4( a_vp, 1.0 );
}
