#version 120

varying vec3 n;
varying vec2 st;

uniform float ol_mode;

void main () {
	gl_FragColor = vec4 (0.87, 0.1, 0.2, 1.0);

	if (ol_mode > 0.5) {
		gl_FragColor = vec4 (0.0, 0.0, 0.0, 1.0);
	}
}
