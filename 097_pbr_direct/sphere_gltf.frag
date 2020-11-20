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

	vec3 l_a = light_rgb;
	vec3 k_a = vec3( 0.03 );
	vec3 i_a = l_a * k_a;

	vec3 l_d = light_rgb;
	vec3 k_d = vec3( 0.7 );
	float dot_d = clamp( dot( n, dir_to_light ), 0.0, 1.0 );
	vec3 i_d = l_d * k_d * dot_d;

	// TODO(Anton) add specular

	return i_a + i_d;
}

void srgb_to_linear( inout vec3 rgb ) {
	rgb = pow( rgb, vec3( 2.2 ) );
}

void linear_to_srgb( inout vec3 rgb ) {
	rgb = pow( rgb, vec3( 1.0 / 2.2 ) );
}


/******************************************************************************
GLTF's code.
From Appendix A: https://github.com/KhronosGroup/glTF-Sample-Viewer

vec3 F = F_specular + F_diffuse;

From https://academy.substance3d.com/courses/the-pbr-guide-part-2
* base colour RGB     ( interpret as sRGB )
* roughness greyscale ( interpret as linear )
* metallic greyscale  ( interpret as linear )

* The dielectric F0 values are not authored by hand as the shader handles them.
* When the shader sees black in the metal map, it treats the corresponding area in the base color map as dielectric and uses a 4% (0.04) reflectance value.
* Dielectrics reflect a smaller amount of light than metals. The value for common dielectrics would be around 2-5%. In terms of sRGB, the values should be between sRGB 40-75, which overlap the 0.02-0.05 (linear) range.
* Common gemstones fall within the 0.05-0.17 (linear) range.
* Common liquids fall within the 0.02-0.04 (linear) range.
Dielectric F0
The F0 for common dielectric materials is typically set to 0.04 (linear) 4% reflective. In the metal/roughness workflow, this value is hardcoded in the shader.
Some metal/roughness implementations, such as those found in the Substance toolset and Unreal Engine 4, have a specular control,
which allows the artist to change the constant F0 value for dielectrics. In Substance, this output is labeled as ‘‘specularLevel’’
and is supplied by a texture sampler in the metal/roughness PBR shader. It represents the range of 0.0-0.08, as shown in Figure 20.
This range is remapped in the shader to 0.0-1.0 where 0.5 represents 4% reflective.

The base color map is an RGB map that can contain 2 types of data: diffuse reflected color for dielectrics and reflectance values for metals, as shown in Figure 22.
The color that represents dielectrics represents reflected wavelengths, as discussed in Part 1.
* The reflectance values are present if an area is denoted as metal in the metallic map (white values).
* Values that indicate the reflectance values for metals should be obtained from real-world measured values.
* Metal areas that fall in this range will need to have a reflectance range of 70-100% reflective.
  - gold   sRGB(255,226,155)
  - silver sRGB(252,250,245)
	- Al     sRGB(245,246,246)
	- Iron   sRGB(196,199,199)
	- Copper sRGB(250,208,192)
* If the metallic map has gray values lower than 235 sRGB you need to lower the “raw” metal reflectance value in the base color.


UNREAL
https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf

f(l,v) = top / bottom
top    = D(h) * F(l,h) * G(l,v,h)
bottom = 4 * ( n dot l ) * ( n dot v )

D(h)     = specular distribution.
           either GGX from Trowbridge-Reitz (fairly cheap with longer tail that looks more natural) or Blinn-Phong.
F(l,h)   = Fresnel.
           approximate the power.
G(l,v,h) = Geometric shadowing.
           Schlick or Smith. Minor differences. Schlick cheaper. Uses Diseny's roughness remapping.

Image-based lighting
        - to use only one sample per env map
				- split equation into 2 pre-calculated parts;
				  * pre-filtered environment map
					* environment BRDF stored in lookup texture LUT

* Specular parameter is confusing -> not really needed -> replaced with Cavity (AO)

current model
* base colour
* metallic
* roughness
* cavity

* lighting now uses inverse square falloff
******************************************************************************/

/*-----------------------------------------------------------------------------
 Specular Term (F_specular)
-----------------------------------------------------------------------------*/

/*
Surface Reflection Ratio (F)
Fresnel Schlick

Please note, that the below shader code includes the optimization for "turning off" the Fresnel edge brightening (see "Real-Time Rendering" Fourth Edition on page 325).

Fresnel creates the edge highlight ring / halo from the index of refraction. https://learnopengl.com/img/pbr/fresnel.png
"F0  represents the base reflectivity of the surface, which we calculate using something called the indices of refraction or IOR."
"Fresnel-Schlick approximation is only really defined for dielectric or non-metal surfaces.
For conductor surfaces (metals), calculating the base reflectivity with indices of refraction doesn't properly hold and we need to use a different Fresnel equation for conductors altogether. 
As this is inconvenient, we further approximate by pre-computing the surface's response at normal incidence (F0) at a 0 degree angle as if looking directly onto a surface. 
We interpolate this value based on the view angle, as per the Fresnel-Schlick approximation, such that we can use the same equation for both metals and non-metals.
"
- nice table of IOR sRGB and linear values on that site also
*/
vec3 fresnel_schlick( vec3 f0, vec3 f90, float VdotH ) {
	return f0 + ( f90 - f0 ) * pow( clamp( 1.0 - VdotH, 0.0, 1.0 ), 5.0 );
}

/* Microfacet metallic-roughness BRDF */
vec3 metallicBRDF( vec3 f0, vec3 f90, float alphaRoughness, float VdotH, float NdotL, float NdotV, float NdotH ) {
	vec3 F = fresnel_schlick( f0, f90, VdotH );
	float Vis = V_GGX( NdotL, NdotV, alphaRoughness ); // Vis = G / (4 * NdotL * NdotV)
	float D = D_GGX( NdotH, alphaRoughness );

	return F * Vis * D; // vec3 F_specular = D * Vis * F;
}

/*
Geometric Occlusion / Visibility (V)
Smith Joint GGX (Smith calls Schlick as a subfunction as explained here https://learnopengl.com/PBR/Theory)
*/
float V_GGX( float NdotL, float NdotV, float alphaRoughness ) {
	float alphaRoughnessSq = alphaRoughness * alphaRoughness;

	float GGXV = NdotL * sqrt( NdotV * NdotV * ( 1.0 - alphaRoughnessSq ) + alphaRoughnessSq ); // this is schlick (1 - k) + k
	float GGXL = NdotV * sqrt( NdotL * NdotL * ( 1.0 - alphaRoughnessSq ) + alphaRoughnessSq ); // this is schlick (1 - k) + k

	float GGX = GGXV + GGXL;
	if ( GGX > 0.0 ) {
		return 0.5 / GGX;
	}
	return 0.0;
}


/*
Normal Distribution (D)
Trowbridge-Reitz GGX
Unreal calls this the Specular distribution term D(h) https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
learnopengl.com example image of output range (for roughness values i guess): https://learnopengl.com/img/pbr/ndf.png
*/
float D_GGX( float NdotH, float alphaRoughness ) {
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float f = ( NdotH * NdotH ) * ( alphaRoughnessSq - 1.0 ) + 1.0;
    return alphaRoughnessSq / ( M_PI * f * f );
}

/*-----------------------------------------------------------------------------
Diffuse Term (F_diffuse)
-----------------------------------------------------------------------------*/

/*
Lambertian
Alternative here is Burley.
*/
vec3 lambertian( vec3 f0, vec3 f90, vec3 diffuseColor, float VdotH ) {
	return ( 1.0 - fresnel_schlick( f0, f90, VdotH ) ) *  (diffuseColor / M_PI ); // vec3 F_diffuse = (1.0 - F) * diffuse;
}

/******************************************************************************

******************************************************************************/

// PBR code based on https://github.com/michal-z/SimpleDirectPBR/blob/master/Source/Shaders/SimpleForward.hlsl
// alternative functions are also listed there
#define GPI 3.14159265359
vec3 fresnel_schlick( float h_dot_v, vec3 f0 ) {
	return f0 + ( vec3( 1.0 ) - f0 ) * pow( 1.0 - h_dot_v, 5.0 );
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

		vec3 f = fresnel_schlick( clamp( dot( h, viewer_to_surface_wor ), 0.0, 1.0 ), f0 );
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
