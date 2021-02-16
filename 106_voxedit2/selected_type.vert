#version 410 core

in vec3 a_vp;
in vec2 a_vt;

uniform int u_selected_type;

uniform mat4 u_P, u_V, u_M;
out vec2 v_st;

void main() {
  v_st = a_vt / 16.0;
  v_st.s += 1.0 / 16.0 * float( mod( u_selected_type, 16 ) );
  v_st.t += 1.0 / 16.0 * float( u_selected_type / 16 );
  // * 0.5 so the cube that sits of 0 goes from -0.5 to +0.5 an doesnt overlap the cube at 1
  gl_Position = u_P * u_V *  u_M * vec4( a_vp, 1.0  );
}
