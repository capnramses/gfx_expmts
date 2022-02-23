//https://www.khronos.org/files/opengles3-quick-reference-card.pdf
#version 320 es
// #pragma optimize({on, off}) - enable or disable shader optimization (default on)
// #pragma debug({on, off}) - enable or disable compiling shaders with debug information (default off)

/* 
mediump Range and precision is between that provided by lowp
and highp.
lowp Range and precision can be less than mediump, but
*/

in vec3 a_vp;
in vec3 a_vt;

uniform mat4 u_P, u_V, u_M;

out vec2 v_st;

void main () {
	v_st = a_vp.xy * 0.5 + 0.5;
	v_st.t = 1.0 - v_st.t;
	gl_Position = u_P * u_V * u_M * vec4( a_vp, 1.0 );
}
