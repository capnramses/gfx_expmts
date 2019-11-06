#version 410

uniform vec3 pos;
uniform mat4 P, V;

void main () {
	gl_Position = P * V * vec4 (pos, 1.0);
	gl_PointSize = 20.0; // size in pixels
}
