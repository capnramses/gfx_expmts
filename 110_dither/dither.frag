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
	float antons_dither[16] = float[16](
		  0.0, 128.0,  32.0, 160.0,
		192.0,  64.0, 224.0,  96.0,
		 48.0, 176.0,  16.0, 144.0,
		240.0, 112.0, 208.0,  80.0
	);
	// note cast to int here truncates the 0.5 etc
	uint index = ( uint( pos_screen.x * dither_pixel_scale ) % 4 ) * 4 + uint( pos_screen.y * dither_pixel_scale ) % 4;
	return antons_dither[index] / 255.0;
}

void main() {

	vec4 rgba = vec4( 1.0, 1.0, 1.0, 1.0 );
	float vertical_factor = ( gl_FragCoord.y - 0.5 ) / u_screen_dims.y; // the offset of 0.5 here is because origin pixel centre is 0.5,0.5
	// dither any mesh on the LHS of the screen
	if ( ( gl_FragCoord.x - 0.5 ) / u_screen_dims.x < 0.5 ) {
		float d = dither( gl_FragCoord, 0.5 ); // change from 1.0 to 0.5 for fine/chunky
		rgba -= d * ( 1.0 - vertical_factor );
	}

	rgba.a *= vertical_factor;
	o_frag_colour = rgba;
}
