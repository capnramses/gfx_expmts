#version 410 core

#define PBR
//#define TONE_MAPPING // kills edge highlights and some metallic tint but smooths banded light exposure

in vec3 v_p_wor;
in vec2 v_st;
in vec3 v_n_wor;
in vec3 v_tan;
in mat3 TBN;

uniform samplerCube u_texture_irradiance_map;
uniform samplerCube u_texture_prefilter_map;
uniform sampler2D   u_texture_brdf_lut;

uniform sampler2D u_texture_albedo;
uniform sampler2D u_texture_metal_roughness;
uniform sampler2D u_texture_emissive;
uniform sampler2D u_texture_ambient_occlusion;
uniform sampler2D u_texture_normal;

uniform float u_roughness_factor;
uniform float u_metallic_factor;

uniform vec3 u_cam_pos_wor;
uniform vec3 u_light_pos_wor;
uniform vec4 u_base_colour_rgba;

out vec4 o_frag_colour;

vec3 blinn_phong( vec3 n, vec3 light_pos, vec3 light_rgb ) {
	vec3 dir_to_light = normalize( u_light_pos_wor - v_p_wor );
	vec3 dir_to_viewer = normalize( u_cam_pos_wor - v_p_wor );

	vec3 l_a = light_rgb;
	vec3 k_a = u_base_colour_rgba.rgb * 0.05;
	vec3 i_a = l_a * k_a;

	vec3 l_s = light_rgb;
	vec3 k_s = u_base_colour_rgba.rgb * ( 1.0 - u_roughness_factor );
	float specular_exponent = 50.0; // specular 'power'
	vec3 half_way = normalize( dir_to_viewer + dir_to_light );
	float dot_s = clamp( dot( half_way, n ), 0.0, 1.0 );
	float specular_factor = pow( dot_s, specular_exponent );
	vec3 i_s = l_s * k_s * specular_factor * u_metallic_factor;

	vec3 l_d = light_rgb;
	vec3 k_d = ( vec3( 1.0 ) - k_s ) * u_base_colour_rgba.rgb; // NOTE(Anton) aping energy conservation here
	float dot_d = clamp( dot( n, dir_to_light ), 0.0, 1.0 );
	vec3 i_d = l_d * k_d * dot_d;

	return i_a + ( i_d + i_s ) * 0.95;
}

#ifdef PBR

#define M_PI 3.14159265359

/* F - fresnel equation.
Ratio of reflection at different surface angles.
Epic uses Fresnel-Schlick. Not to be confused woth Schlick-Beckman, below.
NOTE(Anton) glTF and others use an f90 variable here to balance f0. note sure.
*/
vec3 f_fresnel_schlick( float cos_theta, vec3 f0 ) {
	return f0 + ( 1.0 - f0 ) * pow( 1.0 - cos_theta, 5.0 );
}

/* used in ambient IBL */
vec3 fresnel_schlick_roughness( float cos_theta, vec3 f0, float roughness ) {
	return f0 + ( max( vec3( 1.0 - roughness ), f0 ) - f0 ) * pow( 1.0 - cos_theta, 5.0 );
}

/* D - normal distribution function.
Appoximation of surface microfacet alignment with halfway vector (based on roughness).
Epic uses Trowbridge-Reitz GGX.
*/
float d_trowbridge_reitz_ggx( float n_dot_h, float roughness_factor_k ) {
	float alpha_sq = roughness_factor_k * roughness_factor_k;
	float part = n_dot_h * n_dot_h * ( alpha_sq - 1.0 ) + 1.0;
	return ( roughness_factor_k * roughness_factor_k ) / ( M_PI * part * part );
}

float g_schlick_ggx( float n_dot_something, float roughness ) {
	float r = roughness + 1.0;
	float k = ( roughness * roughness ) / 8.0;
	return n_dot_something / ( n_dot_something * ( 1.0 - k ) + k );
}

/* G - geometry function.
Self-shadowing by microfacets. Very rough surfaces overshadow other microfacets.
Epic uses Smith's Schlick-GGX aka Schlick-Beckmann.
*/
float g_smith( vec3 n, vec3 v, vec3 l, float roughness ) {
	float n_dot_v = clamp( dot( n, v ), 0.0, 1.0 );
	float n_dot_l = clamp( dot( n, l ), 0.0, 1.0 );
	float part_a  = g_schlick_ggx( n_dot_v, roughness );
	float part_b  = g_schlick_ggx( n_dot_l, roughness );
	return part_a * part_b;
}
#endif

void main() {
	vec3 n_wor = normalize( v_n_wor );
	
#ifdef PBR
	vec3 rgb = vec3( 0.0 );
	////
		// NB plug in RGB version here (linear) as it gets corrected to sRGB later. a texture would store the sRGB (usually)
		// Copper	RGB (0.95, 0.64, 0.54) sRGB (0.98, 0.82, 0.76)
		// NOTE colour for gold RGB value vec3( 1.0, 0.71, 0.29)

		/*
		The base color map is an RGB map that can contain 2 types of data: diffuse reflected color for dielectrics and reflectance values for metals, as shown in Figure 22.
		The color that represents dielectrics represents reflected wavelengths, as discussed in Part 1.
		* The reflectance values are present if an area is denoted as metal in the metallic map (white values).
		* Values that indicate the reflectance values for metals should be obtained from real-world measured values.
		* Metal areas that fall in this range will need to have a reflectance range of 70-100% reflective.

		* If the metallic map has gray values lower than 235 sRGB you need to lower the “raw” metal reflectance value in the base color.
		*/
		vec3 gold   = pow( vec3( 255.0, 226.0, 155.0 ) / 255.0, vec3( 2.2 ) );
		vec3 silver = pow( vec3( 252.0, 250.0, 245.0 ) / 255.0, vec3( 2.2 ) );
		vec3 Al     = pow( vec3( 245.0, 246.0, 246.0 ) / 255.0, vec3( 2.2 ) );
		vec3 Iron   = pow( vec3( 196.0, 199.0, 199.0 ) / 255.0, vec3( 2.2 ) );
		vec3 Copper = pow( vec3( 250.0, 208.0, 192.0 ) / 255.0, vec3( 2.2 ) );
		vec3 red    = vec3( 1.0, 0.0, 0.0 );
		vec3 purple = vec3( 1.0, 0.0, 1.0 );

		// TODO(Anton):
		// "If the metallic map has gray values lower than 235 sRGB you need to lower the “raw” metal reflectance value in the base color."
		vec4 src_rgba = texture( u_texture_albedo, v_st ).rgba;
		vec3 albedo_texel_rgb = src_rgba.rgb;
		// weirdly i think gb are used but not B so vec2 makes no sense
		vec3 metal_roughness_texel_rgb = texture( u_texture_metal_roughness, v_st ).rgb;
		vec3 emissive_texel_rgb = texture( u_texture_emissive, v_st ).rgb;
		vec3 ambient_occlusion_texel_rgb = texture( u_texture_ambient_occlusion, v_st ).rgb;
		vec3 normal_texel_rgb = texture( u_texture_normal, v_st ).rgb;
		vec3 n_tan = normalize( normal_texel_rgb * 2.0 - 1.0 );
		n_wor = normalize( TBN * n_tan );

		vec3 albedo     = pow( albedo_texel_rgb, vec3( 2.2 ) );
		float metal     = metal_roughness_texel_rgb.b; // NOTE(Anton) not sure if linear!
		float roughness = metal_roughness_texel_rgb.g; // NOTE(Anton) not sure if linear!
		float ao        = ambient_occlusion_texel_rgb.r;
		 // TODO later - ambient factor if no texture
		//vec3 albedo = u_base_colour_rgba.rgb;//vec3( 1.0, 0.71, 0.29 );//vec3( 1.00, 0.86, 0.57 );//vec3( 1.0, 0.0, 0.0 );
	  //float metal = clamp( u_metallic_factor, 0.01, 1.0 );
		//float roughness = clamp( u_roughness_factor, 0.01, 1.0 );
			
		// IOR term but we use metal term to differentiate from a dead-on angle.
		vec3 f0       = mix( vec3( 0.04 ), albedo, metal ); // F0 is the base reflectivity of the surface - calculated using index of refraction IOR

		vec3 v_to_p_dir_wor   = normalize( u_cam_pos_wor - v_p_wor );     // V
		vec3 L_o              = vec3( 0.0 ); // total reflected light
		 ///// for each light l in scene (this is equivalent to solving integral over omega for direct light sources
		 // NOTE(Anton) higher than 1.0 values! radiant flux translated to rgb triplet.
			vec3 light_colour   = vec3( 200.0 );               
			vec3 p_to_l_dir_wor = normalize( u_light_pos_wor - v_p_wor ); // L. == equiv to w_i incoming vector
			vec3 h_wor          = normalize( v_to_p_dir_wor + p_to_l_dir_wor );

			float distance      = length( u_light_pos_wor - v_p_wor );
			float attenuation   = 1.0 / ( distance * distance );

			vec3 radiance       = light_colour * attenuation; // NB more correct atten with inverse-square available when gamma correcting
			// Cook-Torrance specular term
			float h_dot_v = clamp( dot( h_wor, v_to_p_dir_wor ), 0.0, 1.0 );
			vec3 F        = f_fresnel_schlick( h_dot_v, f0 );
			float n_dot_h = clamp( dot( n_wor, h_wor ), 0.0, 1.0 );
			float D       = d_trowbridge_reitz_ggx( n_dot_h, roughness );  // normal distribution function      
			float G       = g_smith( n_wor, v_to_p_dir_wor, p_to_l_dir_wor, roughness );      
			// calculate cook-torrance BRDF
			// TODO precalc ndotv etc
			float denominator   = 4.0 * clamp( dot( n_wor, v_to_p_dir_wor ), 0.0, 1.0 ) * clamp( dot( n_wor, p_to_l_dir_wor), 0.0, 1.0 );
			vec3 specular       = ( D * G * F ) / max( denominator, 0.001 );

			vec3 k_s = F;
			vec3 k_d = ( vec3( 1.0 ) - k_s ) * ( 1.0 - metal );

			float n_dot_l = clamp( dot( n_wor, p_to_l_dir_wor ), 0.0, 1.0 );     
    	L_o += ( k_d * albedo / M_PI + specular ) * radiance * n_dot_l;
		//////

		vec3 ambient = vec3( 0.03 ) * albedo * ao;

		vec3 prefilteredColor;
		{ // IBL
			// LUT
			vec3 f        = fresnel_schlick_roughness( max( dot( n_wor, v_to_p_dir_wor ), 0.0 ), f0, roughness );

			// diffuse IBL
			float n_dot_v = clamp( dot( n_wor, v_to_p_dir_wor ), 0.0, 1.0 );
			vec3 k_s = f;
			vec3 k_d = 1.0 - k_s;
			k_d *= ( 1.0 - metal );
			vec3 irradiance_rgb = texture( u_texture_irradiance_map, n_wor ).rgb;
			vec3 diffuse = irradiance_rgb * albedo;

			// specular IBL
			// prefilter
			vec3 reflection_wor = reflect( -v_to_p_dir_wor, n_wor );   
			const float MAX_REFLECTION_LOD = 4.0;
			prefilteredColor = textureLod( u_texture_prefilter_map, reflection_wor, roughness * MAX_REFLECTION_LOD ).rgb;  
			vec2 envBRDF  = texture( u_texture_brdf_lut, vec2( max( dot( n_wor, v_to_p_dir_wor ), 0.0), roughness ) ).rg;
			vec3 specular = prefilteredColor * (f * envBRDF.x + envBRDF.y) * albedo;   // NOTE(Anton) added *albedo here because it was making it white!//

			ambient = ( k_d * diffuse + specular ) * ao;
		}
		rgb  = clamp( ambient, vec3( 0.0 ), vec3( 1.0 ) ) + clamp( L_o, vec3( 0.0 ), vec3( 1.0 ) );

#ifdef TONE_MAPPING
	if ( gl_FragCoord.x < 1024.0 * 0.5 ) {
		//	rgb.b = 1.0;
			rgb = rgb / ( rgb + vec3( 1.0 ) ); // tone mapping
	}
#endif

	//	rgb = prefilteredColor;
	////
#else
	vec3 rgb = blinn_phong( n_wor, u_light_pos_wor, vec3( 0.8 ) );
#endif

	rgb += emissive_texel_rgb;

	o_frag_colour.rgb = vec3( rgb );
	o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );
	o_frag_colour.a = src_rgba.a;

}
