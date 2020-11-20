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


NOTE: I have no idea what F90 is from glTF ref so gonna use a model without it.
*/

#ifdef PBR

#define M_PI 3.14159265359

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

/* Microfacet metallic-roughness BRDF */
vec3 metallicBRDF( vec3 f0, vec3 f90, float alphaRoughness, float VdotH, float NdotL, float NdotV, float NdotH ) {
	vec3 F = fresnel_schlick( f0, f90, VdotH );
	float Vis = V_GGX( NdotL, NdotV, alphaRoughness ); // Vis = G / (4 * NdotL * NdotV)
	float D = D_GGX( NdotH, alphaRoughness );

	return F * Vis * D; // vec3 F_specular = D * Vis * F;
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

#endif
/******************************************************************************
Notes from https://learnopengl.com/PBR/Theory

The reflectance equation

  L_o(p,w) = integral for omega( f_r( p, w_i, w_o ) * L_i( p, w_i ) * n dot w_i * dw_i )

	NB 'irradiance' is the sum of all radiance (all radiance within a half sphere omega pointing in direction n around point p.
	The integral sums all radiance inside the hemisphere. It must be done as averaging a sum of small steps (dw_i) - the Riemann sum.
	Increase number of steps to increase accuracy.

  L -   radiance - magnitude of light from a single direction
	      L = (d^2 flux) / ( dA * dw * cos(theta) ) 
			  total observed energy in area A over solid angle w of a light of radiant intensity phi. at angle thea from surface normal.
			  use infinitely small A and w for light at a point on surface (per fragment).
			  w becomes a direction vector, and A into point p.
  w -   solid angle - area of a shape projected onto a unit sphere 
	I -   radiant intensity - amount of radiant flux (light energy) per solid angle. .: strength of light over a projected area.
	      I = delta flux / delta w
	f_r - bidirectional reflective distribution function. almost everything uses Cook-Torrance.
				for a mirror surface function returns 0.0 for all angles except directly back to viewer - that is 1.0.
	      p     - point on surface
				w_i   - incoming light vector
				w_o   - outgoing light vector
				n     - surface normal
				alpha - microsurface's roughness at p

				Cook Torrance: f_r = k_d * f_lambert + k_s * f_cook_torrance
				  k_d - ratio of incoming light energy refracted. (diffuse on left-hand side)
					f_lambert = c / pi
					c - albedo (surface colour)
					k_s - ratio of incoming light energy reflected.
          f_cook_torrance = (D*F*G) / ( 4 * ( w_o dot n ) * ( w_i dot n ) )
					D - normal distribution function - appoximation of surface microfacet alignment with halfway vector (based on roughness). (Epic uses Trowbridge-Reitz GGX)
					G - geometry function - self-shadowing by microfacets. very rough surfaces overshadow other microfacets. ( Epic uses Fresnel-Schlick)
					F - fresnel equation - ratio of reflection at different surface angles. (Epic uses Smith's Schlick-GGX aka Schlick-Beckmann)

					D = NDF_GGXTR(n,h,alpha) = alpha^2 / ( pi * ( ( n dot h )^2 * ( alpha^2 - 1 ) + 1 )^2 )
					G = G(n, v, l, k) = G_sub(n, v, k) * G_sub(n, l, k)
					    we we will use G_schlickGGX for G_sub:
							G_schlickGGX(n, v, k) = ( n dot v ) / ( ( n dot v )( 1 - k ) + k )
					    k - a remapping of alpha depending on whether we use direct or Image-Based Lighting (IBL)
							k_direct = (alpha + 1)^2 / 8
							k_IBL    = alpha^2 / 2
					F = Fresnel_schlick( h, v, F0 ) = F0 + (1-F0)(1-(h dot v))^5
					    F0 is the base reflectivity of the surface - calculated using index of refraction IOR - only really valid for non-metal surfaces.
							We interpolate a result from F0 (view angle) so that it works for metal and non metal surfaces.
							Most dielectrics have F0 0.2-0.17 as a scalar, but metals are tinted RGB vec3(0.56, 0.57, 0.58) (iron) and higher.
							We use metal factor to differentiate.

							vec3 F0 = vec3(0.04);
							F0      = mix(F0, surfaceColor.rgb, metalness); // tint with surface colour if metallic

.:
  L_o( p, w_o ) = integral_omega ( k_d(c/pi) + DFG / 4(w_o dot n)(w_i dot n) ) * L_i(p,w_i)n dot w_i * dw_i 


******************************************************************************/
// NOTE(Anton) glTF and others use an f90 variable here to balance f0. note sure.
vec3 fresnel_schlick( float cos_theta, vec3 f0 ) {
	return f0 + ( 1.0 - f0 ) * pow( 1.0 - cos_theta, 5.0 );
}  

void main() {
	vec3 n_wor = normalize( v_n_wor );
	

#ifdef PBR
	{
		vec3 albedo = vec3( 1.0, 0.0, 0.0 );
		float metal = clamp( u_metallic_factor, 0.01, 1.0 );

		vec3 v_to_p_dir_wor   = normalize( u_cam_pos_wor - v_p_wor );     // V
		vec3 L_o = vec3( 0.0 ); // total reflected light
		{ // for each light l in scene (this is equivalent to solving integral over omega for direct light sources
			vec3 light_colour   = vec3( 23.0, 21.0, 20.0 );               // NOTE(Anton) higher than 1.0 values! radiant flux translated to rgb triplet.
			vec3 p_to_l_dir_wor = normalize( u_light_pos_wor - v_p_wor ); // L. == equiv to w_i incoming vector
			vec3 h_wor          = normalize( v_to_p_dir_wor + p_to_l_dir_wor );
			float distance      = length( u_light_pos_wor - v_p_wor );
			float attenuation   = 1.0 / ( distance * distance );
			vec3 radiance       = light_colour * attenuation; // NB more correct atten with inverse-square available when gamma correcting
			// Cook-Torrance specular term
			vec3 f0 = mix( vec3( 0.04 ), albedo, metal );
			vec3 f  = fresnel_schlick( clamp( dot( h_wor, v_to_p_dir_wor ), 0.0, 1.0 ), f0 );
		}
		
		
		float cos_theta     = clamp( dot( n_wor, p_to_l_dir_wor ), 0.0, 1.0 );
		// NOTE: a directional light source has a constant w_i, with no attenuation factor. area lights etc have different properties.
		float attenuation   = calc_atten( v_p_wor, u_light_pos_wor );
}
	float k_a = 0.9; // AO
	float roughness = clamp( u_roughness_factor, 0.01, 1.0 ); // @lh0xfb: There is a singularity where roughness = 0
	float alpha_roughness = roughness * roughness;
	float k = alpha_roughness + 1.0;
	k = ( k * k ) / 8.0;
	vec3 f0 = mix( vec3( 0.04 ), albedo, metal );
	vec3 l_o = vec3( 0.0 );

	{ // for each light in n lights...
		vec3 dist3d_to_light_wor = u_light_pos_wor - v_p_wor; // LightVector 
		vec3 dir_to_light_wor = normalize( dist3d_to_light_wor ); // L
		vec3 h_wor = normalize( dir_to_light_wor + viewer_to_surface_wor );

		float kS = calculateSpecularComponent(...); // reflection/specular fraction
		float kD = 1.0 - kS;                        // refraction/diffuse  fraction
	}
	vec3 ambient = vec3( 0.03 ) * albedo * k_a;
	vec3 rgb = ambient + l_o;
#else
	vec3 rgb = blinn_phong( n_wor, u_light_pos_wor, vec3( 0.8 ) );
#endif

	o_frag_colour = vec4( rgb, 1.0 );
	linear_to_srgb( o_frag_colour.rgb );
}
