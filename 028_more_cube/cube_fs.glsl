#version 410

in vec3 texcoords;
uniform samplerCube cube_texture;
uniform vec3 light_pos_wor;
out vec4 frag_colour;

void main () {
	float texel = texture (cube_texture, texcoords).r;
	vec3 base = vec3 (0.2, 0.6, 0.1);
	frag_colour = vec4 (base, 1.0);
	// note: texcoords are also world position of cube verts/frags

	// TODO for self-shadows
	// if length is CLOSER than own 1d distance to light -->shadow, else not
	// and add bias
	float l = length (texcoords - light_pos_wor);
	if (texel > 0.0) {
		frag_colour.rgb *= 0.5;
	}

	//frag_colour.rgb = vec3 (texel,texel,texel);
	//frag_colour.rgb = vec3 (l/14.0,l/14.0,l/14.0);
}
