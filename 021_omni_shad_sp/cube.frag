#version 430 core

in vec2 st;
in vec3 fn;
out vec4 fo;

void main () {
	fo = vec4 (abs (fn) * 0.6 + 0.4, 1.0);
}