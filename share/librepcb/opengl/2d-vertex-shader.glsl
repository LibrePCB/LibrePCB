#version 330 core

attribute vec2 a_position;
attribute vec3 a_color;

out vData {
    vec3 color;
} vertex;

void main() {
    vertex.color = a_color;
    gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);
}
