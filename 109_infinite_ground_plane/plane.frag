#version 410

in vec4 v_world_pos;
in vec4 v_world_clip;

uniform sampler2D u_texture_a;

out vec4 o_frag_colour;

void main() {
  vec2 st = vec2( v_world_pos.x, v_world_pos.z );
  o_frag_colour = texture( u_texture_a, st );
	o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );
}
