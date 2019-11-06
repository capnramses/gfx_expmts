#version 120

attribute vec3 vp; // positions from mesh
attribute vec3 vn; // normals from mesh

uniform mat4 P, V, M; // proj, view, model matrices

varying vec3 texcoords;
varying vec3 n_wor;
varying vec3 p_wor;

void main () {
	gl_Position = P * V * M * vec4 (vp, 1.0);
	texcoords = vec3 (M * vec4 (vp, 1.0));
	n_wor = vec3 (M * vec4 (vn, 0.0));
	p_wor = vec3 (M * vec4 (vp, 1.0));
}
