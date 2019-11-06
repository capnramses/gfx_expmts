#version 430

in vec2 st;
//uniform sampler2D depth_tex;
uniform samplerCube depth_tex;
out vec4 frag_colour;

void main () {
	//float d = texture (depth_tex, st).r;
	vec2 sst = st * 2.0 - 1.0;
	sst.t = -sst.t;
	float d = texture (depth_tex, vec3 (sst, -1.0)).r;
	
	frag_colour = vec4 (d, d, d, 1.0);
}
