#version 430

in vec3 vp;
uniform mat4 P, V;
out vec2 st;

void main () {
	st = (vp.xy + 1.0) * 0.5;
	gl_Position = P * V * vec4 (vp, 1.0);
}
