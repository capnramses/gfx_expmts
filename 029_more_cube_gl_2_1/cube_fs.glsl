#version 120

varying vec3 texcoords;
uniform samplerCube cube_texture;
uniform vec3 light_pos_wor;

void main () {
	float texel = textureCube (cube_texture, texcoords).r;
	vec3 base = vec3 (0.2, 0.6, 0.1);
	gl_FragColor = vec4 (base, 1.0);
	// note: texcoords are also world position of cube verts/frags

	// TODO for self-shadows
	// if length is CLOSER than own 1d distance to light -->shadow, else not
	// and add bias
	//float l = length (texcoords - light_pos_wor);
	float unpacking_factor = 100.0;
	if (texel * unpacking_factor > 0.0) {
		gl_FragColor.rgb *= 0.5;
	}
}
