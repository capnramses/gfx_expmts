// cubemap frag shader that convolutes an environment map into a prefiltered environment map for specular contribution - each mip stores a roughness level
// based on: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/2.1.2.ibl_irradiance/2.1.2.irradiance_convolution.fs
#version 410 core

in vec3 v_st;
uniform samplerCube u_texture_a;
uniform float u_roughness_factor; // TODO(Anton) hook up
out vec4 o_frag_colour;

const float PI = 3.14159265359;

// Van Der Corpus
float radical_inverse_vdc( uint bits ) {
	bits = ( bits << 16 ) | ( bits >> 16u );
	bits = ( ( bits & 0x55555555u) << 1u ) | ( ( bits & 0xAAAAAAAAu ) >> 1u );
	bits = ( ( bits & 0x33333333u) << 2u ) | ( ( bits & 0xCCCCCCCCu ) >> 2u );
	bits = ( ( bits & 0x0F0F0F0Fu) << 4u ) | ( ( bits & 0xF0F0F0F0u ) >> 4u );
	bits = ( ( bits & 0x00FF00FFu) << 8u ) | ( ( bits & 0xFF00FF00u ) >> 8u );
	return float( bits ) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley( uint sample_idx, uint sample_count ) {
	return vec2( float( sample_idx ) / float( sample_count ), radical_inverse_vdc( sample_idx ) );
}  

vec3 importance_sample_ggx( vec2 xi, vec3 n, float roughness ) {
	float a = roughness * roughness;
	float phi       = 2.0 * PI * xi.x;
	float cos_theta = sqrt( ( 1.0 - xi.y ) / ( 1.0 + ( a * a - 1.0 ) * xi.y ) );
	float sin_theta = sqrt( 1.0 - cos_theta * cos_theta );
	// spherical to cartesian
	vec3 h = vec3( cos( phi ) * sin_theta, sin( phi ) * sin_theta, cos_theta );
	// tangent to world space
	vec3 up         = abs( n.z ) < 0.999 ? vec3( 0.0, 0.0, 1.0 ) : vec3( 1.0, 0.0, 0.0 );
	vec3 tangent    = normalize( cross( up, n ) );
	vec3 bitangent  = cross( n, tangent );
	vec3 sample_wor = tangent * h.x + bitangent * h.y + n * h.z;
	return normalize( sample_wor );
} 

void main () {
	vec3 n_wor              = normalize( v_st ); // NOTE: N = R = V
	const uint SAMPLE_COUNT = 1024u;
	float total_weight      = 0.0;
	vec3 prefiltered_rgb    = vec3( 0.0 );

	for ( uint i = 0u; i < SAMPLE_COUNT; i++ ) {
		vec2 xi       = hammersley( i, SAMPLE_COUNT );
		vec3 h        = importance_sample_ggx( xi, n_wor, u_roughness_factor );
		vec3 l        = normalize( 2.0 * dot( n_wor, h ) * h - n_wor );
		float n_dot_l = max( dot( n_wor, l ), 0.0 );
		if ( n_dot_l > 0.0 ) {
			prefiltered_rgb += texture( u_texture_a, l ).rgb * n_dot_l;
			total_weight    += n_dot_l;
		}
	}

	o_frag_colour.rgba = vec4( prefiltered_rgb / total_weight, 1.0 );
}
