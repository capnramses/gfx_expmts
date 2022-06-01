// Dither experiment
// Anton Gerdelan
// GLSL
// 26 Oct 2021

#version 410 core

// Note - you can do interesting offsets with gl_FragCoord (but it says you shouldn't unless absolutely necessary)
// and it has origin in bottom-left and that the pixel centre is not 0 but 0.5,0.5 to eg 1024.5,1024.5
// https://www.khronos.org/opengl/wiki/Built-in_Variable_(GLSL)#Fragment_shader_inputs

out vec4 o_frag_colour;

uniform float u_time; // time as a 0..1 value
uniform vec2 u_screen_dims;

// dither_pixel_scale - make smaller than 1.0 for blockier dither pattern
// to get a dither colour pattern do rgba = rgba - dither
float dither( vec4 pos_screen, float dither_pixel_scale ) {
	// for values from Unity's dither node: https://docs.unity3d.com/Packages/com.unity.shadergraph@6.9/manual/Dither-Node.html
	// values here are from my own 4x4 dither image (made in GIMP) -- see bayer_matrix_4x4.ppm for the actual image
	float antons_dither[16] = float[16]( // changed to the 4x4 from wikipedia https://en.wikipedia.org/wiki/Ordered_dithering
		  0.0,  8.0,  2.0, 10.0,
		 12.0,  4.0, 14.0,  6.0,
		  3.0, 11.0,  1.0,  9.0,
		 15.0,  7.0, 13.0,  5.0
	);
	// note cast to int here truncates the 0.5 etc
	uint index = ( uint( pos_screen.x * dither_pixel_scale ) % 4 ) * 4 + uint( pos_screen.y * dither_pixel_scale ) % 4;
	return antons_dither[index] * 0.0625; // this is == / 16.
}

void main() {

	vec4 rgba = vec4( 1.0, 1.0, 1.0, 1.0 );
	float vertical_factor = ( gl_FragCoord.y - 0.5 ) / u_screen_dims.y; // the offset of 0.5 here is because origin pixel centre is 0.5,0.5
	// dither any mesh on the LHS of the screen
	if ( ( gl_FragCoord.x - 0.5 ) / u_screen_dims.x < 0.5 ) {

		// normally we just want time or distance to get a non-alpha-blending fade out, but here
		// i use both time AND vertical position to demo the full effect range

		float d = dither( gl_FragCoord, 0.5 ); // change from 1.0 to 0.5 for fine/chunky
		if (  vertical_factor <= d ) { discard; }
		if (  u_time <= d ) { discard; }
		//rgba.rgb = vec3( d );
	} else {

		// the right-hand side of the screen uses a typical alpha blend fade, for comparison
		rgba.a = min( u_time, vertical_factor );
	}

	o_frag_colour = rgba;
}
