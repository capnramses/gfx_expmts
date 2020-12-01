// cubemap frag shader that convolutes an environment map into a prefiltered environment map for specular contribution - each mip stores a roughness level
// based on: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/2.1.2.ibl_irradiance/2.1.2.irradiance_convolution.fs
#version 410 core

in vec3 v_st;
uniform samplerCube u_texture_a;
uniform float u_roughness_factor; // TODO(Anton) hook up
out vec4 o_frag_colour;

const float PI = 3.14159265359;
const float ONE_DEG_IN_RAD = 0.01745329;

void main () {
	vec3 n_wor              = normalize( v_st );

	vec3 texel = texture( u_texture_a, n_wor ).rgb;
	vec3 final = vec3( 0.0 );
	{
		float fac = 2.0;
		vec3 a = normalize( vec3( n_wor.x + u_roughness_factor / 4.0 * fac, n_wor.yz ) );
		vec3 b = normalize( vec3( n_wor.x - u_roughness_factor / 4.0 * fac, n_wor.yz ) );
		vec3 c = normalize( vec3( n_wor.x, n_wor.y + u_roughness_factor / 4.0 * fac, n_wor.z ) );
		vec3 d = normalize( vec3( n_wor.x, n_wor.y - u_roughness_factor / 4.0 * fac, n_wor.z ) );
		vec3 e = normalize( vec3( n_wor.xy, n_wor.z + u_roughness_factor / 4.0 * fac ) );
		vec3 f = normalize( vec3( n_wor.xy, n_wor.z - u_roughness_factor / 4.0 * fac ) );


		vec3 texel_a = texture( u_texture_a, a ).rgb;
		vec3 texel_b = texture( u_texture_a, b ).rgb;
		vec3 texel_c = texture( u_texture_a, c ).rgb;
		vec3 texel_d = texture( u_texture_a, d ).rgb;
		vec3 texel_e = texture( u_texture_a, e ).rgb;
		vec3 texel_f = texture( u_texture_a, f ).rgb;
		final = ( texel_a + texel_b + texel_c + texel_d + texel_e + texel_f + texel ) / 7.0;
	}

	o_frag_colour.rgba = vec4( final, 1.0 );
	
}
