#version 410 core

in vec3 a_pos;

uniform mat4 u_P, u_V, u_M;

void main() {
  gl_Position = u_P * u_V * u_M * vec4( a_pos, 1.0 );
}
