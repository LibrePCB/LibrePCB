#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

uniform mat4 mvp_matrix;

attribute vec4 a_position;
attribute vec4 a_color;

varying vec4 v_color;

void main() {
    v_color = a_color;
    gl_Position = mvp_matrix * a_position;
}
