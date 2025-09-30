#version 450

/*
vec2 positions[3] = vec2[](
    vec2( 0.0, -0.5 ),
    vec2( 0.5, 0.5 ),
    vec2( -0.5, 0.5 )
);

vec3 colours[3] = vec3[](
    vec3( 1.0, 0.0, 0.0 ),
    vec3( 0.0, 1.0, 0.0 ),
    vec3( 0.0, 0.0, 1.0 )
);
*/

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec3 a_colour;

layout(location = 0) out vec3 v_colour;

void main() {
    //gl_Position = vec4( positions[gl_VertexIndex], 0.0, 1.0 );
    // v_colour = colours[gl_VertexIndex];

    gl_Position = vec4( a_position, 0.0, 1.0 );
    v_colour = a_colour;  
}