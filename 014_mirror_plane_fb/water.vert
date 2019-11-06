#version 410

in vec2 vp;
uniform mat4 P, V, M;

void main () {
	gl_Position = P * V * M * vec4 (vp, 0.0, 1.0);
}
