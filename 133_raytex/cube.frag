#version 410 core

in vec3 v_vp;

uniform sampler3D tex;

out vec4 frag_colour;

void main() {
  vec4 texel = texture( tex, v_vp );
  frag_colour = vec4( texel.rgb, 1.0 );
}