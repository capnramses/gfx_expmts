#version 430

in vec3 vp;
out vec2 st;

void main () {
	st = vp.xy / 2.0 + 0.5;
	gl_Position = vec4 (vp, 1.0);
}
