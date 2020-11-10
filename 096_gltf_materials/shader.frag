 #version 410 core

in vec2 v_st;
in vec3 v_rgb;
in vec3 v_cube_st;

uniform float u_alpha;

uniform sampler2D u_texture_a; // albedo
uniform samplerCube u_texture_b; // cube
uniform vec4 u_base_colour_rgba;
uniform float u_roughness_factor;

out vec4 o_frag_colour;

void main () {
  vec4 albedo_texel = texture( u_texture_a, v_st );
  vec4 cube_texel = texture( u_texture_b, normalize( v_cube_st ) );

  vec4 rgba = albedo_texel * u_base_colour_rgba;
	rgba.rgb = cube_texel.rgb * 0.1 + albedo_texel.rgb * 0.9;

	o_frag_colour = rgba;
  //o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );
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
