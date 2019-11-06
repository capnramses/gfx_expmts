#version 120

varying vec3 texcoords;
varying vec3 n_wor;
varying vec3 p_wor;

uniform samplerCube cube_texture;
uniform vec3 light_pos_wor; // NOTE: i dont actually update this in code - assume 0,0,0
uniform vec3 cam_pos_wor;

float shad (float l, vec3 tc) {
	float texel = textureCube (cube_texture, tc).r;
	// bias is used in case of self-shadowing
	float bias = 0.025;
	float unpacking_factor = 100.0;
	if (texel * unpacking_factor - bias < l) {
		return 0.5;
	}
	return 1.0;
}

vec3 light (vec3 dist, vec3 kd, vec3 ks, float shad_fac) {
	vec3 nn = normalize (n_wor);

	vec3 ndir = normalize (dist);
	float dp = max (dot (nn, ndir), 0.0);
	vec3 ls = vec3 (0.8, 0.8, 0.8);
	vec3 ld = vec3 (0.8,0.8,0.8);
	vec3 la = vec3 (0.25, 0.25, 0.25);

	// ambient
	vec3 ia = la * kd;

	// lambert
	vec3 id = ld * kd * dp;

	// blinn
	float specular_exponent = 10.0;
	vec3 surface_to_viewer_wor = normalize (cam_pos_wor - p_wor);
	vec3 h = normalize (surface_to_viewer_wor + ndir);
	float dp2 = max (dot (h, nn), 0.0);
	float sfac = pow (dp2, specular_exponent);
	vec3 is = (ls * ks * sfac);

	return (id + is) * shad_fac + ia;
}

void main () {
	vec3 base = vec3 (0.7, 0.4, 0.2);
	vec3 spec = vec3 (0.7, 0.7, 0.7);

	// note: texcoords are also world position of cube verts/frags
	vec3 dir = texcoords - light_pos_wor;
	float l = length (dir);
	float shad_f = 1.0;

	{ // shadow cube single sample version
		shad_f = shad (l, dir);
	}
/*
	{ // blurred v
		float per = 0.0125;
		float a = shad (l, texcoords + vec3 (per, 0.0, 0.0));
		float b = shad (l, texcoords + vec3 (-per, 0.0, 0.0));
		float c = shad (l, texcoords + vec3 (0.0, per, 0.0));
		float d = shad (l, texcoords + vec3 (0.0, -per, 0.0));
		float e = shad (l, texcoords + vec3 (0.0, 0.0, per));
		float f = shad (l, texcoords + vec3 (0.0, 0.0, -per));
		shad_f = (a + b + c + d + e + f) / 6.0;
	}
*/
	gl_FragColor.rgb = light (light_pos_wor - texcoords, base, spec, shad_f);
	gl_FragColor.a = 1.0;
	
}
