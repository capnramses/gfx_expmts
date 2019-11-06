#version 410

in float d;
out vec4 frag_colour;

void main () {
	frag_colour = vec4 (0.0, 0.0, 0.0, 1.0);
	frag_colour.x = 1.0 - d;
}
