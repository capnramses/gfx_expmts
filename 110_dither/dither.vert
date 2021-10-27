// Dither experiment
// Anton Gerdelan
// GLSL
// 26 Oct 2021

#version 410 core

in vec2 a_vp;

uniform mat4 u_P, u_V, u_M;

void main() {
	gl_Position = u_P * u_V * u_M * vec4( a_vp, 0.0, 1.0 );
}
