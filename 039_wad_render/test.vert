// WAD Rend - Copyright 2017 Anton Gerdelan <antonofnote@gmail.com>
// C99
#version 120

attribute vec3 vertex_position;
attribute vec3 vertex_normal;
uniform mat4 view, proj;
varying vec3 n_eye;

void main() {
	gl_Position = proj * view * vec4 (vertex_position, 1.0);
	n_eye = vec3(view * vec4 (vertex_normal, 0.0));
}
