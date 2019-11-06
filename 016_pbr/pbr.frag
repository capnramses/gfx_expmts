#version 400 core

in vec3 n;
out vec4 fc;

void main () {
	fc = vec4 (n, 1.0);
}
