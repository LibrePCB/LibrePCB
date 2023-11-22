#version 330 core

attribute float a_type;
attribute vec2 a_position;
attribute vec4 a_params;
attribute vec4 a_color;

out float vg_type;
out vec4 vg_params;
out vec4 vg_color;

void main() {
    vg_type = a_type;
    gl_Position = vec4(a_position, 0.0, 1.0);
    vg_params = a_params;
    vg_color = a_color;
}
