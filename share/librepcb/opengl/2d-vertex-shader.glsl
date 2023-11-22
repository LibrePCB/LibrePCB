#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 mvp_matrix;

out VS_OUT {
    vec3 color;
} vs_out;

void main() {
    gl_Position = mvp_matrix * vec4(aPos.x, aPos.y, 0.0, 1.0);
    vs_out.color = aColor;
}
