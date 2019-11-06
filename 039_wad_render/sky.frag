// WAD Rend - Copyright 2017 Anton Gerdelan <antonofnote@gmail.com>
// C99

#version 120

varying vec3 texcoords;
uniform samplerCube my_cube_texture;

void main () {
	gl_FragColor = textureCube(my_cube_texture, texcoords);
	//gl_FragColor.rgb = texcoords;
	gl_FragColor.a = 1.0;
}
