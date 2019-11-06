#version 430

in vec2 st;
uniform sampler2D scene_tex;
out vec4 frag_colour;

void main () {
	vec4 texel = texture (scene_tex, st);
	frag_colour = vec4 (texel.rgb, 1.0);
};
