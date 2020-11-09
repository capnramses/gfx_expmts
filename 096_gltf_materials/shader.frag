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
