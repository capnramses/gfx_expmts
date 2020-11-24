#version 410 core

in vec3 v_st;
uniform samplerCube u_texture_a;
out vec4 o_frag_colour;

void main () {
	vec3 n_wor = normalize( v_st );
	vec3 texel = textureLod( u_texture_a, n_wor, 1.0 ).rgb; 
	o_frag_colour.rgb = texel.rgb;
	o_frag_colour.a = 1.0;
}
