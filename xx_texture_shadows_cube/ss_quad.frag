#version 410

in vec2 st;
uniform samplerCube depth_tex;
out vec4 frag_colour;

void main () {
	float d = textureCube (depth_tex, normalize (vec3 (
		2.0 * st.s - 1.0,
		-1.0,
		-2.0 * st.t + 1.0
	))).r;
	// linearise
	float far = 50.0;
	float near = 1.0;
	float range = far - near;
	// normalise
	float dn = 2.0 * d - 1.0;
	dn = 2.0 * near * far / (far + near - dn * range);
	// range 0 to 1 again
	dn = (dn - near) / range;
	frag_colour = vec4 (dn, dn, dn, 1.0);
}
