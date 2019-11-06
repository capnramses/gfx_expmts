#version 120

attribute vec3 vp;
uniform mat4 P, V;
varying vec3 texcoords;

void main () {
	texcoords = vp;
	gl_Position = P * V * vec4 (vp, 1.0);
}
