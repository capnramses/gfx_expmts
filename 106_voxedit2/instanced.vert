#version 410 core

in vec3 a_vp;
in vec3 a_vn;
in vec2 a_vt;
in vec3 a_instanced_pos; // world positions
in int a_instanced_b; // index of which type it is
uniform mat4 u_P, u_V, u_M;
out vec2 v_st;

void main() {
  v_st = a_vt / 16.0;
  v_st.s += 1.0 / 16.0 * float( mod( a_instanced_b, 16 ) );
  v_st.t += 1.0 / 16.0 * float( a_instanced_b / 16 );
  // * 0.5 so the cube that sits of 0 goes from -0.5 to +0.5 an doesnt overlap the cube at 1
  gl_Position = u_P * u_V *  u_M * vec4( a_vp * 0.5 + a_instanced_pos, 1.0  );
}
