#version 320 es

// REQUIRED
precision highp float;

in vec2 v_st;

uniform sampler2D u_tex;

out vec4 o_frag_colour;

void main () {
	vec4 texel = texture( u_tex, v_st );
	texel.rgb = pow( texel.rgb, vec3( 2.2 ) );
	o_frag_colour = texel;
	o_frag_colour.rgb = pow( o_frag_colour.rgb, vec3( 1.0 / 2.2 ) );
}
