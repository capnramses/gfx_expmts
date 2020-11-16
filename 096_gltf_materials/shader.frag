/*
- can precompute and load a look-up table for PBR as a ktx texture
- load a prefiltered (blurry) environment map
- can use normal map if tangents loaded in
- convert textures to linear space before use
*/

#version 410 core

in vec3 v_p_wor;
in vec2 v_st;
in vec3 v_rgb;
in vec3 v_cube_st;
in vec3 v_n;

uniform float u_alpha;

uniform sampler2D u_texture_albedo;
uniform sampler2D u_texture_metal_roughness;
uniform sampler2D u_texture_emissive;
uniform sampler2D u_texture_ambient_occlusion;
uniform sampler2D u_texture_normal;
uniform samplerCube u_texture_environment;
uniform vec4 u_base_colour_rgba;
uniform vec3 u_cam_pos_wor; 
uniform float u_roughness_factor;

out vec4 o_frag_colour;

const float PI = 3.14159265358979323846;

// http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
vec3 diffuse_burley( float l_dot_h, float alpha_roughness, vec3 diffuse_colour, float n_dot_l, float n_dot_v ) {
	float f90 = 2.0 * l_dot_h * l_dot_h * alpha_roughness - 0.5;
	vec3 op = ( diffuse_colour / PI ) * ( 1.0 + f90 * pow( ( 1.0 - n_dot_l ), 5.0 ) ) *	( 1.0 + f90 * pow( ( 1.0 - n_dot_v ), 5.0 ) );
	return clamp( op, vec3( 0.0 ), vec3( 1.0 ) );
}

// returns 'F' term for equation
vec3 specular_fresnel( vec3 reflectance0, vec3 reflectance90, float v_dot_h ) {
	vec3 op = reflectance0 + ( reflectance90 - reflectance0 ) * pow( clamp( 1.0 - v_dot_h, 0.0, 1.0 ), 5.0);
	return clamp( op, vec3( 0.0 ), vec3( 1.0 ) );
}

// specular atten 'G' - rougher reflect less
float geometric_occl( float alpha_roughness, float n_dot_l, float n_dot_v ) {
  float rough_sq = alpha_roughness * alpha_roughness;
  float attenuation_l = 2.0 * n_dot_l / ( n_dot_l + sqrt( rough_sq + ( 1.0 - rough_sq ) * ( n_dot_l * n_dot_l ) ) );
  float attenuation_v = 2.0 * n_dot_v / ( n_dot_v + sqrt( rough_sq + ( 1.0 - rough_sq ) * ( n_dot_v * n_dot_v ) ) );
  return clamp( attenuation_l * attenuation_v, 0.0, 1.0 );
}

// distrib of microfacet normals 'D'. "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz.
float microfacet_distrib( float n_dot_h, float alpha_roughness ) {
  float rough_sq = alpha_roughness * alpha_roughness;
  float f = ( n_dot_h * rough_sq - n_dot_h ) * n_dot_h + 1.0;
  return clamp( rough_sq / ( PI * f * f ), 0.0, 1.0 );
}

void main () {
  vec4 albedo_texel = texture( u_texture_albedo, v_st );
  vec4 metal_roughness_texel = texture( u_texture_metal_roughness, v_st );
  vec4 emissive_texel = texture( u_texture_emissive, v_st );
  vec4 ambient_occlusion_texel = texture( u_texture_ambient_occlusion, v_st );
  vec4 normal_texel = texture( u_texture_normal, v_st );
	// NOTE(anton) can use normal map here
	vec3 n_wor = normalize( v_n );

	/*
	glTF 2.0
	- roughness is stored in the green, metallic is stored in the blue.
	- red channel for optional occlusion map data:

	*/
	vec3 rgb = vec3( 1.0 );
	{
		// TODO(Anton) -- u_roughness_factor
		float perceptual_roughness = clamp( metal_roughness_texel.g * 1.0, 0.04, 1.0 );
		float alpha_roughness = perceptual_roughness * perceptual_roughness; // this is material's roughness ( a convention )
		float metallic  = clamp( metal_roughness_texel.b * 1.0, 0.0, 1.0);
		vec4 base_colour = albedo_texel * u_base_colour_rgba;
		vec3 f0 = vec3( 0.04 );
		vec3 diffuse_rgb = base_colour.rgb * ( vec3( 1.0 ) - f0 );
		diffuse_rgb *= 1.0 - metallic;
		vec3 specular_rgb = mix( f0, base_colour.rgb, metallic );
		float reflectance = max( max( specular_rgb.r, specular_rgb.g), specular_rgb.b );
		// there's a bit here about grazing angles and Fresnel
		// NOTE -- different var to specenvr90 and specenvr0
		float reflectance90 = clamp( reflectance * 25.0, 0.0, 1.0 );  
		vec3 specular_env_r0 = specular_rgb.rgb;
		vec3 specular_env_r90 = vec3( 1.0 ) * reflectance90;
		vec3 v_wor = normalize( u_cam_pos_wor - v_p_wor );
  	vec3 reflection_wor = -normalize( reflect( v_wor, n_wor ) );

		//////////////////////////////////////////////////////////

		// NB with HDR cube maps can calc fancy lighting here - i just use colour directly
  	vec4 cube_texel = texture( u_texture_environment, reflection_wor );

		vec3 pbr_light_contrib = vec3( 0.0 );
		////////////////
			vec3 lightColor = cube_texel.rgb;
			// NOTE(Anton) i think this is direction /to/ the light ??
			vec3 light_dir_wor = n_wor;// normalize( vec3( 0.0, 1.0, 1.0 ) );

			float n_dot_v = clamp( abs( dot( n_wor, v_wor ) ), 0.001, 1.0 );
			vec3 h_wor = ( light_dir_wor + v_wor );
			float n_dot_h = clamp( dot( n_wor, h_wor ), 0.0, 1.0 );
			float v_dot_h = clamp( dot( v_wor, h_wor ), 0.0, 1.0 );
			float n_dot_l = clamp( dot( n_wor, light_dir_wor ), 0.001, 1.0 );
			float l_dot_h = clamp( dot( light_dir_wor, h_wor ), 0.0, 1.0 );

			vec3  F = specular_fresnel( specular_env_r0, specular_env_r90, v_dot_h );
			float G = geometric_occl( alpha_roughness, n_dot_l, n_dot_v );   
			float D = microfacet_distrib( n_dot_h, alpha_roughness ); 
			
			vec3 diffuse_contrib = ( 1.0 - F ) * diffuse_burley( l_dot_h, alpha_roughness, base_colour.rgb, n_dot_l, n_dot_v );
			vec3 spec_contrib = clamp(  F * G * D / ( 4.0 * n_dot_l * n_dot_v ), vec3( 0.0 ), vec3( 1.0 ) );   

			pbr_light_contrib = clamp( n_dot_l * lightColor * (diffuse_contrib + spec_contrib), vec3( 0.0 ), vec3( 1.0 ) );
		//////////////


		vec3 color = pbr_light_contrib;
		color *= ( ambient_occlusion_texel.r < 0.01 ? 1.0 : ambient_occlusion_texel.r );
		color += emissive_texel.rgb ;

		{
			vec3 I =  n_dot_l * vec3( 0.75 ) * ( diffuse_contrib + spec_contrib );
			// NOTE(Anton) had to modify cube texel's spec/diff comp manually because don't have HDR texture BRDF info in mine
			color = I + (n_dot_l * cube_texel.rgb * 0.4 * spec_contrib + 0.1 * diffuse_contrib *cube_texel.rgb ) * ambient_occlusion_texel.r + emissive_texel.rgb;
		}
	

		rgb = clamp( vec3(color ), vec3( 0.0 ), vec3( 1.0 ) );
	}

	o_frag_colour = vec4( rgb, 1.0 );
}


/* PBR BRDF from Khronos glTF example implementation: https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#appendix-b-brdf-implementation
The metalness values are sampled from the B channel. The roughness values are sampled from the G channel.
interesting HLSL reference: https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/pbr.hlsl
{
  const dielectricSpecular = 0.04
  const black = 0

  c_diff = lerp(baseColor.rgb * (1 - dielectricSpecular), black, metallic)
  f0 = lerp(0.04, baseColor.rgb, metallic)
  α = roughness^2

  F = f0 + (1 - f0) * (1 - abs(VdotH))^5

  f_diffuse = (1 - F) * (1 / π) * c_diff
  f_specular = F * D(α) * G(α) / (4 * abs(VdotN) * abs(LdotN))

  material = f_diffuse + f_specular
}
*/
