#version 410

in vec3 vp;
uniform mat4 P, V;
out float d;

void main () {
	gl_Position = P * V * vec4 (vp.x, vp.y + 3.0, vp.z, 1.0);
	d = vp.z;
}
