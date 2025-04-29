#version 410 core

uniform ivec2 win_dims, fb_dims;
uniform sampler2D u_col_tex, u_pal_tex;
out vec4 frag_colour;

void main() {

    vec2 st = gl_FragCoord.xy / vec2( win_dims );
    st.t = 1.0 - st.t;
    // vec2 rg = gl_FragCoord.xy / fb_dims;

    float idx = texture(u_col_tex, st ).r;
    vec3 rgb = texture(u_pal_tex, vec2( idx, 0.0 ) ).rgb;

    frag_colour = vec4( rgb, 1.0 );
};