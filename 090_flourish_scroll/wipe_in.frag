// wipe-in
// GLSL

#version 410 core

in vec2 v_st;

uniform sampler2D u_tex;
uniform float u_time;

out vec4 o_frag_colour;

void main() {
  if ( u_time < v_st.s ) { discard; }
  vec4 texel = texture( u_tex, v_st );
  o_frag_colour = texel;
  o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );

}
