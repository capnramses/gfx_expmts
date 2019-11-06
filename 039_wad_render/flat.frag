// WAD Rend - Copyright 2017 Anton Gerdelan <antonofnote@gmail.com>
// C99
#version 120

varying vec3 n_eye;
varying vec3 p_wor;
uniform sampler2D tex;

void main() {
	vec3 nn = normalize(n_eye);
	float fac = max(0.0, dot(nn, vec3(0.0,0.0,1.0)));
	gl_FragColor = vec4 (1.0, 1.0, 1.0, 1.0);
	vec3 light = vec3(fac * 0.3 + 0.6);
	//gl_FragColor.r = 1.0;

	vec2 st = vec2( p_wor.x, -p_wor.z );
	vec3 texel = texture2D( tex, st / 64.0 ).rgb;

	gl_FragColor.rgb = texel * light;
}
