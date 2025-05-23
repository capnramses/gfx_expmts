#version 410 core

uniform ivec2 win_dims, fb_dims;
out vec4 frag_colour;

float rounded_box( in vec2 p, in vec2 b, in float r ) {
    return length( max( p - b + r, 0.0 ) ) - r;
}

void main() {
    vec2 rg = gl_FragCoord.xy / vec2( win_dims );
    // vec2 rg = gl_FragCoord.xy / fb_dims;

    vec2 box_c = vec2( 0.6, 0.7 );
    vec2 box_half_dims = vec2( 0.2, 0.2 );
    float radius = 0.05;
    float sdb = rounded_box( abs( rg - box_c ), box_half_dims, radius);

    vec3 rgb = mix( vec3(rg, 0.0), vec3(0.0,0.0,1.0), 1.0-floor( 1.0-sdb ));

    frag_colour = vec4( rgb, 1.0 );
};