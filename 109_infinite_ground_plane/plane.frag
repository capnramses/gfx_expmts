#version 410

in vec4 v_world_pos;
in vec4 v_world_clip;

uniform sampler2D u_texture_a;

out vec4 o_frag_colour;

void main() {
  // world coordinates of mesh are used as texture coordinates to give the illusion of an infinite surface
  vec2 st = vec2( v_world_pos.x, v_world_pos.z );
  // line anti-aliasing is handled by a texture with mimapping that also contains 2 grid resolutions
  o_frag_colour = texture( u_texture_a, st );
  // gamma correction
	o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );
}
