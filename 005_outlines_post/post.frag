#version 420

in vec2 st;
// TODO - sample BOTH textures and edge-detect on them both in one pass
// TODO also the normal map
uniform sampler2D post_tex;
out vec4 fc;

float wstf = 1.0 / 800.0;
float hstf = 1.0 / 800.0;

float linear (float d) {
	float f = 10.0;
	float n = 0.1;
	float z = (2.0 * n) / (f + n - d * (f - n));
	return z;
}

void main () {
	//vec4 texel = texture (post_tex, st);
	//fc = vec4 (texel.rgb, 1.0);

	// ANTON 2017 feb 16th -- my matrices were wrong before.
	// just referenced someone else's glsl -- i think i was transposed
	// (rookie mistake) but also a bit out of order with components. 4am code?

	// in columns
	float hmat[9] = float[9] (
		 1.0,  2.0,  1.0,
		 0.0,  0.0,  0.0,
		-1.0, -2.0, -1.0);
	float vmat[9] = float[9] (
		1.0, 0.0, -1.0,
		2.0, 0.0, -2.0,
		1.0, 0.0, -1.0);
		
	vec2 hst[9] = vec2[9] (
		vec2 (st.s - hstf, st.t + wstf),
		vec2 (st.s - hstf, st.t),
		vec2 (st.s - hstf, st.t - wstf),
		vec2 (st.s, st.t + wstf),
		vec2 (st.s, st.t),
		vec2 (st.s, st.t - wstf),
		vec2 (st.s + hstf, st.t + wstf),
		vec2 (st.s + hstf, st.t),
		vec2 (st.s + hstf, st.t - wstf)
	);
	float g_y = 0.0;
	float g_x = 0.0;
	for (int i = 0; i < 9; i++) {
		g_y += linear (texture (post_tex, hst[i]).r) * hmat[i];
		g_x += linear (texture (post_tex, hst[i]).r) * vmat[i];
	}
	// was black with white lines so 1.0 - ...
	float g = 1.0 - sqrt (g_x * g_x + g_y * g_y);

	// thresholding
	if (g > 0.6) {
		g = 1.0;
	} else {
		g = 0.0;
	}
	//g = floor(2.0 * g) * 0.5;

	//
	// great snippet i found to convert to linear depth values
	// did not check maths, but looked good
	

  fc = vec4 (g, g, g, 1.0);


  // TODO combine edge detection AND the original colour here

}
