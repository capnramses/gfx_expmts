#version 410

// Pixel art fragment shader.
// by Anton Gerdelan, Feb 2023
// Subpixel clamp anti-aliasing and
// pre-multiplied alpha.

in vec2 v_st;

uniform vec2 u_fb_dims;
uniform sampler2D u_texture; // Ideally a bi-linear filter with no mipmaps.

out vec4 o_frag_colour;

// Only works if the image/texture itself was pre-multiplied first.
vec4 premul_alpha( vec4 rgba ) {
	return vec4( rgba.rgb *= rgba.a, rgba.a );
}

vec2 calc_nneigh( vec2 st, vec2 dims ) {
	st *= dims;
	return ( floor( st ) + 0.5 ) / dims;
}

// 1.0 thresh is 1 texel wide AA. 1.5 and 2.0 can look nice.
// Based on Ben Golus' https://www.shadertoy.com/view/ltBfRD
vec2 pixelart_aa( vec2 st, vec2 tex_dims, float thresh ) {
	vec2 texels    = st * tex_dims;         // e.g. 0.55 u -> 34.4 texels across.
	// Nearest neighbour, which we snap to if texel fully inside a pixel.
	vec2 nn_texel   = floor( texels + 0.5 ); // Not sure about 0.5 centre offset in GL.
	vec2 remaining  = texels - nn_texel;
	// Sum of derivs; gives us ST for next texel along.
	vec2 next_st    = fwidth( texels ); 
	// Otherwise work out new uv so bi-linear blends nicely in the in-between regions.
	vec2 interpf   = clamp( remaining / next_st / thresh, -0.5, 0.5 );
	// Returns either exact snap or an intermediate value to linear blend.
  return ( nn_texel + interpf ) / tex_dims; // Implicit cast. 
}

void main() {
	vec4 rgba;

	ivec2 tex_dims = textureSize( u_texture, 0 );   // Assuming no mipmaps.
	vec2 aa_st     = pixelart_aa( v_st, tex_dims, 1.5 );
	vec4 texel_aa  = texture( u_texture, aa_st );

	// Snaps to nearest neighbour, even if using bilinear texture filtering.
	vec2 nn_st     = calc_nneigh( v_st, tex_dims );
	vec4 texel_nn  = texture( u_texture, nn_st );

	vec4 texel_regular = texture( u_texture, v_st );

	vec2 screen_fac = gl_FragCoord.xy / u_fb_dims;
	if ( screen_fac.x < 0.33 ) {
		rgba.rgb = texel_aa.bgr;
		rgba.a = texel_aa.a;
	} else 
	if ( screen_fac.x < 0.66 ) {
		rgba.rgb = texel_nn.grb;
		rgba.a = texel_nn.a;
	} else {
		rgba.rgb = texel_regular.rgb;
		rgba.a = texel_regular.a;
	}

	// This should get rid of outlines forming around alpha hard step edges, but only if the texture had premul baked in.
	rgba = premul_alpha( rgba );
	// For pixel art with NN a hard edge is okay instead, but with AA it reintroduces artifacts/breaks AA on edges. 
	//if ( rgba.a < 0.1 ) { discard; }

	rgba.rgb = pow( rgba.rgb, vec3( 1.0 / 2.2 ) ); // Gamma correction must be the last op.
	o_frag_colour = rgba;
}
