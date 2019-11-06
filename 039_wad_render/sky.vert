// WAD Rend - Copyright 2017 Anton Gerdelan <antonofnote@gmail.com>
// C99

#version 120

attribute vec3 vertex_position;
uniform mat4 proj, view;
varying vec3 texcoords;

void main () {
	texcoords = vertex_position;
	gl_Position = proj * view * vec4(vertex_position, 1.0);
	gl_Position.y -= 60.0;
}
