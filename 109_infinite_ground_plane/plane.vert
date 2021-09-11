#version 410

in vec2 a_vp;

uniform mat4 u_P, u_V, u_M;

out vec4 v_world_pos;
out vec4 v_world_clip;

void main() {
  // just a typical world-view-projection draw
  v_world_pos  = u_M * vec4( a_vp, 0.0, 1.0 );
  v_world_clip = u_P * u_V * v_world_pos;
  gl_Position  = v_world_clip;
}
