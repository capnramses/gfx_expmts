#version 400
in vec3 vp;
uniform mat4 P, V;
out vec3 texcoords;

void main () {
  texcoords = vp;
	mat4 modV = V;
	modV[3][0] = 0.0;
	modV[3][1] = 0.0;
	modV[3][2] = 0.0;
  gl_Position = P * modV * vec4 (vp, 1.0);
}
