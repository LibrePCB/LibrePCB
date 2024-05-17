#version 330 core

uniform float u_z;
uniform vec4 u_color;

attribute float a_type;
attribute vec2 a_position;
attribute vec4 a_params;

out float vg_type;
out vec4 vg_params;
out vec4 vg_color;

void main() {
    vg_type = a_type;
    gl_Position = vec4(a_position, u_z, 1.0);
    vg_params = a_params;
    vg_color = u_color;
}
