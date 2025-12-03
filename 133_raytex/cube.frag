#version 410 core

in vec3 v_vp;
in vec3 v_pos_wor;

uniform sampler3D tex;
uniform vec3 u_cam_pos_wor;

out vec4 frag_colour;

void main() {
  vec3 ray_d = normalize( v_pos_wor - u_cam_pos_wor );

  vec4 texel = texture( tex, v_vp );
  frag_colour = vec4( texel.rgb, 1.0 );

  frag_colour.rgb = ray_d;
}
