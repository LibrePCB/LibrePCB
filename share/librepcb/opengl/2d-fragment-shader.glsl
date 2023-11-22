#version 330 core

// smooth in vec2 gf_position;
in vec4 gf_color;

void main() {
    // if (length(gf_position) < 0.2){
    //     discard;
    // }

    gl_FragColor = gf_color;
}
