#version 410 core
in vec2 vp;
void main () {
	gl_Position = vec4 (vp * 0.5, 0.0, 1.0);
}
