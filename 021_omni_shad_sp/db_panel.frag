#version 430 core

in vec2 st;
out vec4 fo;

uniform sampler2D my_tex;

void main () {
	fo = texture (my_tex, st);
}