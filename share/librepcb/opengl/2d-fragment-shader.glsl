//uniform vec2 u_resolution;

uniform vec4 u_circle;

varying vec2 pos;

void main() {
    float r = length(pos - u_circle.xy);

    if (r > u_circle.z || r < u_circle.a) {
        discard;
    }

    gl_FragColor = vec4(1, 0, 0, 0.5);
}
