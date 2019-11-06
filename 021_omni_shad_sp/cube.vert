#version 430 core

layout (location = 0) in vec3 vp;
layout (location = 1) in vec2 vt;
layout (location = 2) in vec3 vn;
out vec2 st;
out vec3 fn;

uniform mat4 P, V, M;

void main () {
	gl_Position = P * V * M * vec4 (vp, 1.0);
	st = vt;
	fn = vn;
}