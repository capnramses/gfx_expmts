#version 420

varying vec2 st;
uniform sampler2D dm;

void main () {
	vec3 rgb = texture (dm, vec2 (1.0 - st.t, 1.0 - st.s)).rgb;
	gl_FragColor = vec4 (rgb, 1.0);
}
