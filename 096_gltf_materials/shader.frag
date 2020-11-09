 #version 410 core

in vec2 v_st;
in vec3 v_rgb;

uniform float u_alpha;

uniform sampler2D u_albedo_texture;
uniform vec4 u_base_colour_rgba;
uniform float u_roughness_factor;

out vec4 o_frag_colour;

void main () {
  vec4 albedo_texel = texture( u_albedo_texture, v_st );
  o_frag_colour.rgba = albedo_texel * u_base_colour_rgba;
  o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );


  //o_frag_colour.rgb = vec3( v_st, 0.0 );
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
