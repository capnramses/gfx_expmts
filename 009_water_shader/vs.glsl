#version 400
in vec3 vp;
uniform mat4 P, V, M;
uniform float t;
out vec3 pos;

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

float wave (int i, float x, float y) {
	float frequency = 2.0 * pi / wavelength[i];
	float phase = speed[i] * frequency;
	float theta = dot (direction[i], vec2 (x, y));
	return amplitude[i] * sin (theta * frequency + t * phase);
}

float waveHeight (float x, float y) {
	float height = 0.0;
	for (int i = 0; i < numWaves; ++i)
		height += wave(i, x, y);
	return height;
}

void main () {
	pos = vec4 (M * vec4 (vp, 1.0)).xyz;
//	pos.y += waveHeight (pos.x, pos.z);
	gl_Position = P * V * vec4 (pos, 1.0);
}
