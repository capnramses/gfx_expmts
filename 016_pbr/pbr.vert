#version 400 core

layout (location = 0) in vec3 vp;
layout (location = 1) in vec2 vt;
layout (location = 2) in vec3 vn;

uniform mat4 P, V, M;

out vec3 n;

void main () {
	gl_Position = P * V * M * vec4 (vp, 1.0);
	n = vn;
}
