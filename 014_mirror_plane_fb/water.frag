#version 410

out vec4 frag_colour;

void main () {
	// using alpha channel to indicate reflectivity
	frag_colour = vec4 (0.0, 0.0, 1.0, 0.0);
}
