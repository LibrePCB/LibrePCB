#version 330 core

in vec3 fColor;

void main() {
    gl_FragColor = vec4(fColor, 0.5);
}
