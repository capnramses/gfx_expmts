#version 410

// texture coordinates from vertex shaders
in vec2 st;

// texture sampler
uniform sampler2D tex;
uniform float t;

// output fragment colour RGBA
out vec4 frag_colour;

float w = 800.0;
float h = 800.0;

void main () {
	// invert colour of right-hand side
	vec4 colour = texture (tex, st);

	vec2 offs = vec2 (0.0, 0.0);
	offs.s = sin (4.0 * t) * sin (st.s * 100.0) * 5.0;
	offs.t = cos (4.0 * t) * sin (st.t * 100.0) * 5.0;
	offs.s *= 1.0 / 800.0;
	offs.t *= 1.0 / 800.0;

	vec4 ref_col = texture (tex, vec2 (st.s + offs.s, 1.0 - (st.t + offs.t))); // TODO complexer

	float ref_fac = 0.25; // TODO fresnel this
	vec3 rgb = mix (colour.rgb, ref_col.rgb, ref_fac * (1.0 - colour.a));

	frag_colour = vec4 (rgb, 1.0);
}
