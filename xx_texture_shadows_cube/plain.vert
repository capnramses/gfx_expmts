#version 410

layout(location = 0) in vec3 vertex_position;
uniform mat4 M, V, P;
uniform vec3 light_pos_wor;
// point in the light's space
out vec3 dist_of_light;

void main() {
	vec3 wp = vec3 (M * vec4 (vertex_position, 1.0));
	gl_Position = P * V * vec4 (wp, 1.0);
	dist_of_light = (wp - light_pos_wor);
}
