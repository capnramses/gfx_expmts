#version 410 core
// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_shading_language_include.txt
#extension GL_ARB_shading_language_include : require

in vec3 v_p_wor;
in vec3 v_n_wor;

uniform float u_roughness_factor;
uniform float u_metallic_factor;

uniform vec3 u_light_pos_wor;

out vec4 o_frag_colour;

vec3 blinn_phong( vec3 n, vec3 light_pos, vec3 light_rgb ) {
	vec3 dir_to_light = normalize( u_light_pos_wor - v_p_wor );

	vec3 l_a = light_rgb;
	vec3 k_a = vec3( 0.03 );
	vec3 i_a = l_a * k_a;

	vec3 l_d = light_rgb;
	vec3 k_d = vec3( 0.7 );
	float dot_d = clamp( dot( n, dir_to_light ), 0.0, 1.0 );
	vec3 i_d = l_d * k_d * dot_d;

	return i_a + i_d;
}

void srgb_to_linear( inout vec3 rgb ) {
	rgb = pow( rgb, vec3( 2.2 ) );
}

void linear_to_srgb( inout vec3 rgb ) {
	rgb = pow( rgb, vec3( 1.0 / 2.2 ) );
}

void main() {
	vec3 n_wor = normalize( v_n_wor );

	vec3 rgb = blinn_phong( n_wor, u_light_pos_wor, vec3( 0.8 ) );

	o_frag_colour = vec4( vec3( u_roughness_factor, u_metallic_factor, 0.0 ) * rgb, 1.0 );
	linear_to_srgb( o_frag_colour.rgb );
}
