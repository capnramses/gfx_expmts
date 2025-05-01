#version 450

layout(location = 0) in vec3 v_colour;

// Location is the framebuffer index to use. We only have 1.
layout(location = 0) out vec4 o_colour;

void main() {
    o_colour = vec4( v_colour, 1.0 );
}