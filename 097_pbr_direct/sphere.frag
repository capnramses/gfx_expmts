#version 410 core
// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_shading_language_include.txt
#extension GL_ARB_shading_language_include : require

in vec3 v_n_wor;

uniform float u_roughness_factor;
uniform float u_metallic_factor;

out vec4 o_frag_colour;

void main() {
	vec3 n_wor = normalize( v_n_wor );

	o_frag_colour = vec4( vec3( u_roughness_factor, u_metallic_factor, 0.0 ) , 1.0 );
}
