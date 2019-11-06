#version 430 core

layout (location = 0) in vec3 vp;
layout (location = 1) in vec2 vt;
layout (location = 2) in vec3 vn;
out vec2 st;
out vec3 fn;

uniform mat4 P, V, M;

void main () {
	gl_Position = P * V * M * vec4 (vp, 1.0);
	gl_Position.xyz /= gl_Position.w;
	gl_Position.xyz *= 0.5;
	gl_Position.x -= 0.5;
	gl_Position.y += 0.5;
	gl_Position.xyz /= (1.0 / gl_Position.w);
	st = vt;
	fn = vn;
}