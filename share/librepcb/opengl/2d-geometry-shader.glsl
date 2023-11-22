#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 5) out;

uniform mat4 mvp_matrix;

in vData {
    vec3 color;
} vertices[];

out vec3 fColor;

void add_vertex(vec4 position) {
    gl_Position = mvp_matrix * position;
    EmitVertex();
}

void build_house(vec4 position) {
    fColor = vertices[0].color;
    add_vertex(position + vec4(-0.2, -0.2, 0.0, 0.0));
    add_vertex(position + vec4( 0.2, -0.2, 0.0, 0.0));
    add_vertex(position + vec4(-0.2,  0.2, 0.0, 0.0));
    add_vertex(position + vec4( 0.2,  0.2, 0.0, 0.0));
    add_vertex(position + vec4( 0.0,  0.4, 0.0, 0.0));
    EndPrimitive();
}

void main() {
    build_house(gl_in[0].gl_Position);
}
