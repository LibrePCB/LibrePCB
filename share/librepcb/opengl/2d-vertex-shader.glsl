attribute vec4 a_position;
attribute vec2 a_center;

uniform mat4 mvp_matrix;

varying vec2 pos;

void main() {
    pos = a_position.xy;
    gl_Position = mvp_matrix * a_position;
}
