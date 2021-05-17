#version 410 core

in vec3 v_n;
in vec2 v_st;

uniform sampler2D u_texture_a; // simplex
uniform float u_time;

out vec4 o_frag_colour;

void main() {

  float t = sin( u_time * 0.25 ) * 0.5 + 0.5;

  vec2 st_scroll = vec2( v_st.s + sin( u_time * 0.02 ), v_st.t + cos( u_time * 0.01 ) );
  vec4 texel = texture( u_texture_a, st_scroll );
  vec4 texel_circuit = texture( u_texture_a, v_st * 3.0 );

  float f = texel.r;

  vec3 n = normalize( v_n );

  vec3 base_rgb = vec3(0.5,0.5,0.5);
  
  float a = 1.0;
  vec3 rgb = base_rgb;
  if ( t > f ) {
    rgb = texel_circuit.r * vec3(0.25,0.25,1.0);
    a = 0.5;
  }

  o_frag_colour = vec4( rgb,1.0 );
}
