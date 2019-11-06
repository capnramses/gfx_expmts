#version 430 core

in vec2 vp;
out vec2 st;

void main () {
	st = vec2 (vp.x * 0.5 + 0.5, vp.y * 0.5 + 0.5);
	gl_Position = vec4 (vp, 0.0, 1.0);
	gl_Position.x *= (256.0 / 1024.0);
	gl_Position.y *= (256.0 / 768.0);
	gl_Position.x += (1.0 - 256.0 / 1024.0);
	gl_Position.y += (1.0 - 256.0 / 768.0);
}