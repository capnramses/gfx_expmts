#version 410 core
// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_shading_language_include.txt
#extension GL_ARB_shading_language_include : require

in vec3 v_n_wor;
out vec4 o_frag_colour;

void main() {
	vec3 n_wor = normalize( v_n_wor );

	o_frag_colour = vec4( n_wor, 1.0 );
}
