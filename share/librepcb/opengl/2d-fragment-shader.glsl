#version 330 core

uniform mat4 mvp_matrix;
uniform vec2 u_resolution;
smooth in vec2 gf_circle_position;
in float gf_circle_radius;
in vec4 gf_color;

void main() {
    vec2 center_px = u_resolution * (gf_circle_position + vec2(1.0, 1.0)) / vec2(2.0, 2.0);
    float radius_px = (vec4(gf_circle_radius * u_resolution.x / 2.0, 0.0, 0.0, 0.0) * mvp_matrix).x;
    if ((gf_circle_radius > 0) && (distance(center_px, gl_FragCoord.xy) > radius_px)){
        discard;
    }

    gl_FragColor = gf_color;
}
