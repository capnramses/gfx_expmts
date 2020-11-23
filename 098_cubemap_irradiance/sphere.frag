#version 410 core

#define PBR

in vec3 v_p_wor;
in vec3 v_n_wor;

uniform float u_roughness_factor;
uniform float u_metallic_factor;

uniform vec3 u_cam_pos_wor;
uniform vec3 u_light_pos_wor;

out vec4 o_frag_colour;

vec3 blinn_phong( vec3 n, vec3 light_pos, vec3 light_rgb ) {
	vec3 dir_to_light = normalize( u_light_pos_wor - v_p_wor );
	vec3 dir_to_viewer = normalize( u_cam_pos_wor - v_p_wor );

	vec3 base_colour = vec3( 1.0, 0.0, 0.0 );

	vec3 l_a = light_rgb;
	vec3 k_a = base_colour * 0.05;
	vec3 i_a = l_a * k_a;

	vec3 l_s = light_rgb;
	vec3 k_s = base_colour * ( 1.0 - u_roughness_factor );
	float specular_exponent = 50.0; // specular 'power'
	vec3 half_way = normalize( dir_to_viewer + dir_to_light );
	float dot_s = clamp( dot( half_way, n ), 0.0, 1.0 );
	float specular_factor = pow( dot_s, specular_exponent );
	vec3 i_s = l_s * k_s * specular_factor * u_metallic_factor;

	vec3 l_d = light_rgb;
	vec3 k_d = ( vec3( 1.0 ) - k_s ) * base_colour; // NOTE(Anton) aping energy conservation here
	float dot_d = clamp( dot( n, dir_to_light ), 0.0, 1.0 );
	vec3 i_d = l_d * k_d * dot_d;

	return i_a + ( i_d + i_s ) * 0.95;
}

void srgb_to_linear( inout vec3 rgb ) {
	rgb = pow( rgb, vec3( 2.2 ) );
}

void linear_to_srgb( inout vec3 rgb ) {
	rgb = pow( rgb, vec3( 1.0 / 2.2 ) );
}

// PBR code based on https://github.com/michal-z/SimpleDirectPBR/blob/master/Source/Shaders/SimpleForward.hlsl
// alternative functions are also listed there
#define GPI 3.14159265359
vec3 fresnel_schlick( float h_dot_n, vec3 f0 ) { // NOTE(Anton) i think this is supposed to be n_dot_h or n_dot_v
	return f0 + ( vec3( 1.0 ) - f0 ) * pow( 1.0 - h_dot_n, 5.0 );
}

// Trowbridge-Reitz GGX normal distribution function.
float distribution_ggx( vec3 n, vec3 h, float alpha ) {
	float alpha2 = alpha * alpha;
	float n_dot_h = clamp( dot( n, h ), 0.0, 1.0 );
	float k = n_dot_h * n_dot_h * ( alpha2 - 1.0 ) + 1.0;
	return alpha2 / ( GPI * k * k ); // NOTE(Anton) if k * GPI is 0 here or alpha2  is zero we have an issue
}

float geometry_schlick_ggx( float x, float k ) {
	return x / ( x * ( 1.0 - k ) + k );
}

float geometry_smith( vec3 n, vec3 v, vec3 l, float k ) {
	float n_dot_v = clamp( dot( n, v ), 0.0, 1.0 );
	float n_dot_l = clamp( dot( n, l ), 0.0, 1.0 );
	return geometry_schlick_ggx( n_dot_v, k ) * geometry_schlick_ggx( n_dot_l, k );
}

void main() {
	vec3 n_wor = normalize( v_n_wor );

#ifdef PBR
	vec3 viewer_to_surface_wor = normalize( u_cam_pos_wor - v_p_wor ); // V
	vec3 albedo = vec3( 1.0, 0.0, 0.0 );
	float k_a = 0.9; // AO
	float metal = clamp( u_metallic_factor, 0.01, 1.0 );
	float roughness = clamp( u_roughness_factor, 0.01, 1.0 ); // @lh0xfb: There is a singularity where roughness = 0
	float alpha_roughness = roughness * roughness;
	float k = alpha_roughness + 1.0;
	k = ( k * k ) / 8.0;
	vec3 f0 = mix( vec3( 0.04 ), albedo, metal );
	vec3 l_o = vec3( 0.0 );

	{ // for each light in n lights...
		vec3 dist3d_to_light_wor = u_light_pos_wor - v_p_wor; // LightVector 
		vec3 dir_to_light_wor = normalize( dist3d_to_light_wor ); // L
		vec3 h = normalize( dir_to_light_wor + viewer_to_surface_wor );

		float distance = length( dist3d_to_light_wor );
		// NOTE(Anton) disabled attenuation factor here -- was killing my light
		float attenuation = 1.0;//1.0 / ( distance * distance );
		vec3 light_colour = vec3( 0.8 );
		vec3 radiance = light_colour * attenuation;

		vec3 f = fresnel_schlick( clamp( dot( h, n_wor ), 0.0, 1.0 ), f0 );
		float ndf = distribution_ggx( n_wor, h, alpha_roughness );
		float g = geometry_smith( n_wor, viewer_to_surface_wor, dir_to_light_wor, k );

		vec3 numerator = ndf * g * f;
		float denominator = 4.0 * clamp( dot( n_wor, viewer_to_surface_wor ), 0.0, 1.0 ) * clamp( dot( n_wor, dir_to_light_wor ), 0.0, 1.0 );
		vec3 specular = numerator / max( denominator, 0.001 );

		vec3 k_s = f;
		vec3 k_d = vec3( 1.0 ) - k_s; // NOTE(Anton) conservation of energy here
		k_d *= 1.0 - metal;

		float n_dot_l = clamp( dot( n_wor, dir_to_light_wor ), 0.0, 1.0 );
		l_o += ( k_d * albedo / GPI + specular ) * radiance * n_dot_l;
	}
	vec3 ambient = vec3( 0.03 ) * albedo * k_a;
	vec3 rgb = ambient + l_o;
#else
	vec3 rgb = blinn_phong( n_wor, u_light_pos_wor, vec3( 0.8 ) );
#endif

	o_frag_colour = vec4( rgb, 1.0 );
	linear_to_srgb( o_frag_colour.rgb );
}
