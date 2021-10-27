// Dither experiment
// Anton Gerdelan
// GLSL
// 26 Oct 2021

#version 410 core

// Note - you can do interesting offsets with gl_FragCoord
// https://www.khronos.org/opengl/wiki/Built-in_Variable_(GLSL)#Fragment_shader_inputs

in vec4 v_pos_clip;

out vec4 o_frag_colour;

void main() {
	o_frag_colour = vec4( 1.0, 0.0, 0.0, 1.0 );
}
