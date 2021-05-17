#version 410 core

in vec3 v_n;
in vec2 v_st;

out vec4 o_frag_colour;

void main() {
  vec3 n = normalize( v_n );

  o_frag_colour = vec4( v_n, 1.0 );
}
