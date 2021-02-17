#version 410 core

in vec2 v_st;
uniform vec4 u_tint;
uniform sampler2D u_texture_a;
out vec4 o_frag_colour;

void main() {
  vec4 texel = texture( u_texture_a, v_st );
  texel.rgb = pow( texel.rgb, vec3( 2.2 ) );
  o_frag_colour = texel * u_tint;
  o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );
}
