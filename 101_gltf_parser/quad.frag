#version 410 core

in vec2 v_st;

uniform sampler2D u_texture_a;

out vec4 o_frag_colour;

void main() {
	vec2 rg = texture( u_texture_a, v_st ).rg;
	o_frag_colour        = vec4( rg, 0.0, 1.0 );
}
