#version 410 core

uniform ivec2 win_dims, fb_dims;
out vec4 frag_colour;

void main() {

    vec2 rg = gl_FragCoord.xy / vec2( win_dims );
    // vec2 rg = gl_FragCoord.xy / fb_dims;

    frag_colour = vec4( rg, 0.0, 1.0 );
};