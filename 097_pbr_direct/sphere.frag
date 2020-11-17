#version 410 core

in vec3 v_n_wor;
out vec4 o_frag_colour;

void main() {
	vec3 n_wor = normalize( v_n_wor );

	o_frag_colour = vec4( n_wor, 1.0 );
}
