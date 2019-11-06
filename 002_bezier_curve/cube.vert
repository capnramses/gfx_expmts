#version 410

in vec3 vp;
in vec2 vt;
in vec3 vn;

uniform mat4 M, V, P;

out vec3 n;
out vec2 st;
out float z;

uniform float t;
uniform vec3 A;
uniform vec3 B;
uniform vec3 C;

vec3 quadratic_bezier () {
	vec3 D = mix (A, B, t); // D = A + t(B - A)
	vec3 E = mix (B, C, t); // E = B + t(C - B)
	vec3 P = mix (D, E, t); // P = D + t(E - D);
	
	return P;
}

void main () {
	vec3 pos = vp + quadratic_bezier ();
	gl_Position = P * V * M * vec4 (pos, 1.0);
	n = vec3 (V * M * vec4 (vn, 0.0));
	st = vt;
}
