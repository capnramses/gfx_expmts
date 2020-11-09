 #version 410 core

in vec2 v_st;
in vec3 v_rgb;

uniform float u_alpha;

out vec4 o_frag_colour;

void main () {
  o_frag_colour.rgb = vec3( v_st, 0.0 );
  o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );
}
