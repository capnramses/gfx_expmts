#version 400
in vec3 pos;
uniform mat4 V, M;
uniform float t;
uniform samplerCube cube_texture;
out vec4 frag_colour;

const float pi = 3.14159;
const int numWaves = 4;
const float amplitude[8] = float[]( 0.06, 0.04, 0.02, 0.01, 0.005, 0.125, 0.125, 0.125 );
const float wavelength[8] = float[]( 1.0, 0.7, 0.4, 0.2, 0.05, 0.5, 0.5, 0.5 );
const float speed[8] = float[]( 0.4, 0.4, 0.3, 0.2, 0.8, 0.25, 0.25, 0.25 );
const vec2 direction[8] = vec2[](
	normalize (vec2 (1.0, 0.56)),
	normalize (vec2 (1.0, 0.0)),
	normalize (vec2 (1.0, -0.2)),
	normalize (vec2 (0.4, -1.0)),
	normalize (vec2 (0.5, -1.0)),
	normalize (vec2 (-0.2, -0.7)),
	vec2 (1.0, 0.0),
	vec2 (1.0, 0.0)
);

const vec3 light_dir_wor = normalize (vec3 (-1.0, -1.0, 0.0));
const vec3 Ls = vec3 (1.0, 1.0, 1.0); // white specular colour
const vec3 La = vec3 (0.2, 0.2, 0.2); // grey ambient colour

const vec3 Ks = vec3 (1.0, 1.0, 1.0);
const vec3 Kd = vec3 (0.4, 0.6, 0.9);
const vec3 Ka = vec3 (1.0, 1.0, 1.0);
const float specular_exponent = 100.0; // specular 'power'

float dWavedx(int i, float x, float y) {
	float frequency = 2.0*pi/wavelength[i];
	float phase = speed[i] * frequency;
	float theta = dot(direction[i], vec2(x, y));
	float A = amplitude[i] * direction[i].x * frequency;
	return A * (cos(theta * frequency + t * phase));
}

float dWavedy(int i, float x, float y) {
	float frequency = 2.0*pi/wavelength[i];
	float phase = speed[i] * frequency;
	float theta = dot(direction[i], vec2(x, y));
	float A = amplitude[i] * direction[i].y * frequency;
	return A * (cos(theta * frequency + t * phase));
}

vec3 waveNormal(float x, float y) {
	float dx = 0.0;
	float dy = 0.0;
	for (int i = 0; i < numWaves; ++i) {
		dx += dWavedx(i, x, y);
		dy += dWavedy(i, x, y);
	}
	vec3 n = vec3(-dx, 1.0 , -dy);
	return normalize(n);
}

void main () {
	vec3 p_eye = (V * vec4 (pos, 1.0)).xyz;
	vec3 n_loc = waveNormal (pos.x, pos.z);
	vec3 n_eye = (V * vec4 (n_loc, 0.0)).xyz;

	vec3 Ia = La * Ka;

	vec3 light_dir_eye = (V * vec4 (light_dir_wor, 0.0)).xyz;
	float dp = max (0.0, dot (-light_dir_eye, n_eye));
	vec3 Ld_dyn;
	Ld_dyn.r = 0.1;
	Ld_dyn.g = max (0.0, -pos.y + 0.6) * 0.7 + 0.2;
	Ld_dyn.b = 0.5;
	Ld_dyn += max (0.0, (pos.y - 0.01) * 5.0);
	Ld_dyn += max (0.0, (pos.y - 0.07) * 10.0);
	Ld_dyn *= 0.5;
	vec3 Id = Ld_dyn * Kd * dp; // final diffuse intensity

	vec3 reflection_eye = reflect (light_dir_eye, n_eye);
	vec3 surface_to_viewer_eye = normalize (-p_eye);
	dp = max (0.0, dot (reflection_eye, surface_to_viewer_eye));
	float specular_factor = pow (dp, specular_exponent);
	vec3 Is = Ls * Ks * specular_factor; // final specular intensity

	vec3 I = Is + Id + Ia;

	mat4 modV = V;
	modV[3][0] = 0.0;
	modV[3][1] = 0.0;
	modV[3][2] = 0.0;

	/* reflect ray around normal from eye to surface */
	vec3 incident_eye = normalize (p_eye);
	vec3 reflected = reflect (incident_eye, n_eye);
	// convert from eye to world space
	reflected = vec3 (inverse (modV) * vec4 (reflected, 0.0));
	vec4 Rflct = texture (cube_texture, reflected);

	float ratio = 1.0 /1.333;
	vec3 refracted = refract (incident_eye, n_eye, ratio);
	refracted = vec3 (inverse (modV) * vec4 (refracted, 0.0));
	vec4 Rfrct = texture (cube_texture, refracted);

	frag_colour.rgb = Rfrct.rgb * 0.5 + Rflct.rgb * 0.25 + I.rgb * 0.25;
	frag_colour.a = 1.0;
}
