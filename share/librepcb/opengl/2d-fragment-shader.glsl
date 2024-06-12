//uniform vec2 u_resolution;

uniform vec3 u_line1;
uniform vec3 u_line2;

varying vec2 pos;

float calcDistanceToLineSegment(vec2 p, vec2 v, vec2 w) {
  float l2 = dot(v-w, v-w);
  if (l2 == 0.0) return distance(p, v);
  float t = max(0.0, min(1.0, dot(p - v, w - v) / l2));
  vec2 projection = v + t * (w - v);
  return distance(p, projection);
}

void main() {
    float r = calcDistanceToLineSegment(pos, u_line1.xy, u_line2.xy);

    if ((r > u_line1.z) || (r < u_line2.z)) {
        discard;
    }

    gl_FragColor = vec4(1, 0, 0, 0.5);
}
