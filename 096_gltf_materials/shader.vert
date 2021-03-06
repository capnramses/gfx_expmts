 #version 410 core

in vec3 a_vp;
in vec2 a_vt;
in vec3 a_vc;
in vec3 a_vn;

uniform mat4 u_P, u_V, u_M;

out vec3 v_p_wor;
out vec2 v_st;
out vec3 v_cube_st;
out vec3 v_rgb;
out vec3 v_n;

void main () {
  v_st = a_vt;
	v_cube_st = (u_M * vec4(a_vp,1.0)).xyz;
	v_cube_st.x = -v_cube_st.x;
  v_rgb = a_vc;
	v_n = ( u_M * vec4( a_vn, 0.0 ) ).xyz;
	v_p_wor = vec3( u_M * vec4( a_vp, 1.0 ) );
  gl_Position = u_P * u_V * vec4( v_p_wor, 1.0 );
}
