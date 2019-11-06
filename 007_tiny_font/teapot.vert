#version 120

attribute vec3 vp; // points
attribute vec2 vt; // 

uniform mat4 M, V, P; // model matrix

varying vec2 st;

void main () {
	st = vt;
	gl_Position = P * V * M * vec4 (vp, 1.0);
}
