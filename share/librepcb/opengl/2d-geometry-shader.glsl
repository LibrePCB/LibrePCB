#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 5) out;

in float vg_type[1];
in vec4 vg_params[1];
in vec4 vg_color[1];

smooth out vec2 gf_circle_position;
out float gf_circle_radius;
out vec4 gf_color;

uniform mat4 mvp_matrix;

void emit_vertex(vec4 position) {
    gl_Position = mvp_matrix * position;
    EmitVertex();
}

void draw_triangle(vec4 p0, vec4 p1, vec4 p2) {
    emit_vertex(p0);
    emit_vertex(p1);
    emit_vertex(p2);
    EndPrimitive();
}

void draw_circle(vec4 position, float diameter) {
    float radius = diameter / 2.0;
    gf_circle_position = (mvp_matrix * position).xy;
    gf_circle_radius = radius;
    emit_vertex(position + vec4(-radius, -radius, 0.0, 0.0));
    emit_vertex(position + vec4(-radius, radius, 0.0, 0.0));
    emit_vertex(position + vec4(radius, -radius, 0.0, 0.0));
    emit_vertex(position + vec4(radius, radius, 0.0, 0.0));
    EndPrimitive();
}

void draw_line(vec4 p0, vec4 p1, float width) {
    vec2 dir = normalize(p1.xy - p0.xy);
    vec2 normal = vec2(dir.y, -dir.x);
    vec4 offset1 = vec4(normal * (width * p0.w), 0, 0);
    vec4 offset2 = vec4(normal * (width * p1.w), 0, 0);
    emit_vertex(p0 + offset1);
    emit_vertex(p0 - offset1);
    emit_vertex(p1 + offset2);
    emit_vertex(p1 - offset2);
    EndPrimitive();
}

void main() {
    gf_color = vg_color[0];
    if (vg_type[0] == 1.0) {
        draw_triangle(gl_in[0].gl_Position,
                       vec4(vg_params[0].xy, 0.0, 1.0),
                       vec4(vg_params[0].zw, 0.0, 1.0));
    } else if (vg_type[0] == 3.0) {
        draw_circle(gl_in[0].gl_Position,
                    vg_params[0].x);
    } else if (vg_type[0] == 2.0) {
        draw_line(gl_in[0].gl_Position,
                   vec4(vg_params[0].xy, 0.0, 1.0),
                   vg_params[0].z);
    }
}
