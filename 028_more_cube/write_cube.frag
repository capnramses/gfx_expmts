#version 410

in vec3 dist_wor;
out float frag_colour;

void main () {
	float l = length (dist_wor);
	frag_colour = l;
}
