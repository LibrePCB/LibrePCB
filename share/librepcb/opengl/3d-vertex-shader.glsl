#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

uniform mat4 mv_matrix;
uniform mat4 mvp_matrix;

attribute vec4 a_position;
attribute vec4 a_color;

varying vec4 v_color;
varying vec3 v_view_pos;

void main() {
    v_color = a_color;
    v_view_pos = (mv_matrix * a_position).xyz;
    gl_Position = mvp_matrix * a_position;
}
