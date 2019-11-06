#version 120

varying vec2 st;
uniform sampler2D my_tex;

void main () {
	gl_FragColor = texture2D (my_tex, st);
}
