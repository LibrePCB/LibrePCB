#ifdef GL_ES
#extension GL_OES_standard_derivatives : enable
precision mediump int;
precision mediump float;
#endif

uniform vec3 light_dir;  // Direction toward light, in view space.

varying vec4 v_color;
varying vec3 v_view_pos;

void main() {
    // Flat shading: derive face normal from screen-space position derivatives.
    vec3 dx = dFdx(v_view_pos);
    vec3 dy = dFdy(v_view_pos);
    vec3 normal = normalize(cross(dx, dy));
    // Ensure the normal faces the camera (camera looks along -Z in view space).
    if (normal.z < 0.0) normal = -normal;

    float ndotl = max(0.0, dot(normal, normalize(light_dir)));
    float ambient = 0.35;
    float diffuse = (1.0 - ambient) * ndotl;
    float intensity = ambient + diffuse;

    gl_FragColor = vec4(v_color.rgb * intensity, v_color.a);
}
