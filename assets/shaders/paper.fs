#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

float hash(vec2 p) { p = fract(p * vec2(123.34, 456.21)); p += dot(p, p + 45.32); return fract(p.x * p.y); }
vec2 hash2(vec2 p) { return fract(sin(vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)))) * 43758.5453); }
float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), f.x), mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x), f.y);
}
float fbm(vec2 p) {
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 5; i++) { v += a * noise(p); p = p * 2.03 + 17.1; a *= 0.5; }
    return v;
}
float sdRoundBox(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

// Aged paper with stains, darkened edges and slightly ragged outline.
uniform vec2 size;
uniform float seed;
uniform vec3 base;
uniform float radius;
uniform float wear;
uniform float rough;

void main() {
    vec2 p = fragTexCoord * size;
    vec2 o = vec2(seed * 13.7, seed * 7.3);
    float d = sdRoundBox(p - size * 0.5, size * 0.5, radius);
    d += (fbm(p * 0.09 + o) - 0.5) * rough;
    float alpha = clamp(0.5 - d, 0.0, 1.0);

    vec3 col = base * (0.92 + 0.14 * fbm(p * 0.01 + o));
    col *= 0.975 + 0.05 * noise(p * 0.9 + o);
    float stain = smoothstep(0.56, 0.78, fbm(p * 0.016 + o * 2.0));
    col = mix(col, col * vec3(0.86, 0.74, 0.52), stain * 0.5 * wear);
    float ring = abs(fbm(p * 0.01 + o * 5.0) - 0.62);
    col = mix(col, col * vec3(0.8, 0.66, 0.45), (1.0 - smoothstep(0.0, 0.012, ring)) * 0.25 * wear);
    float edge = 1.0 - smoothstep(0.0, 0.08 * min(size.x, size.y), -d);
    col = mix(col, col * vec3(0.74, 0.62, 0.44), edge * 0.65 * wear);
    finalColor = vec4(col, alpha) * fragColor;
}
