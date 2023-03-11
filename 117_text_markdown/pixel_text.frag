#version 410

// Pixel art fragment shader.
// by Anton Gerdelan, Feb 2023
// Subpixel clamp anti-aliasing and pre-multiplied alpha.

in vec2 v_st;
in float v_palidx;
flat in int v_codepoint;
flat in int v_spacing;

uniform vec2 u_fb_dims;
uniform sampler2DArray u_texture; // 5 layers.
uniform vec4 u_tint;

out vec4 o_frag_colour;

// 1.0 thresh is 1 texel wide AA. 1.5 and 2.0 can look nice.
// Based on Ben Golus' https://www.shadertoy.com/view/ltBfRD
vec2 pixelart_aa( vec2 st, vec2 tex_dims, float thresh ) {
	vec2 texels    = st * tex_dims;
	vec2 nn_texel   = floor( texels + 0.5 );
	vec2 interpf   = clamp( (texels - nn_texel) / fwidth( texels ) / thresh, -0.5, 0.5 );
  return ( nn_texel + interpf ) / tex_dims;
}

void main() {
	vec2 aa_st    = pixelart_aa( v_st, textureSize( u_texture, 0 ).xy, 1.0 ); // Assuming no mipmaps.
	vec4 texel_aa = texture( u_texture, vec3( aa_st, v_palidx ) );
	vec4 rgba     = texel_aa * u_tint;

	//rgba.rgb *= rgba.a; // Premultiply alpha. Image must be pre-multiplied first.
	//rgba.rgb = pow( rgba.rgb, vec3( 1.0 / 2.2 ) ); // Gamma correction must be the last op.
	o_frag_colour = rgba;
	//if ( o_frag_colour.a < 1.0 ) { o_frag_colour.a = 0.15; } // For debugging boxes don't overlap. Comment out for real view.


	//if ( o_frag_colour.a < 0.05 ) { discard; } 
}
