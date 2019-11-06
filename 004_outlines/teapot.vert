#version 120

attribute vec3 vp; // points
attribute vec2 vt; // points
attribute vec3 vn; // points

uniform mat4 M, V, P; // model matrix
uniform float ol_mode;
uniform float sm_shaded;

varying vec3 n;
varying vec2 st;

void main () {
	if (ol_mode > 0.5) {
		if (sm_shaded > 0.5) {
			vec3 pos = vp + vn * 0.04;
			gl_Position = P * V * M * vec4 (pos, 1.0);
		} else {
			vec3 pos = vp * 1.02;
			gl_Position = P * V * M * vec4 (pos, 1.0);
		}
	} else {
		gl_Position = P * V * M * vec4 (vp, 1.0);
	}
	n = vec3 (V * M * vec4 (vn, 0.0));
	st = vt;
}
