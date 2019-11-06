#version 420

in vec3 n;
in vec2 st;

uniform float ol_mode;

out vec4 fc;

void main () {
	fc = vec4 (0.87, 0.1, 0.2, 1.0);

	if (ol_mode > 0.5) {
		fc = vec4 (0.0, 0.0, 0.0, 1.0);
	}
}
