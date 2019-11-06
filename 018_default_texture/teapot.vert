#version 120

attribute vec3 vp; // points
attribute vec2 vt;

varying vec2 st;

uniform mat4 M, V, P; // model matrix

void main () {
	gl_Position = P * V * M * vec4 (vp, 1.0);
	st = vt;
}
