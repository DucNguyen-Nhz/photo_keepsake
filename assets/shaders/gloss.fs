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

// Plastic sleeve gloss: two soft diagonal highlight bands, masked to the sleeve shape.
uniform vec2 size;
uniform float radius;

void main() {
    vec2 uv = fragTexCoord;
    vec2 p = uv * size;
    float d = sdRoundBox(p - size * 0.5, size * 0.5, radius);
    float mask = clamp(0.5 - d, 0.0, 1.0);
    float k = uv.x * 0.8 + uv.y * 0.55;
    float b1 = (k - 0.38) / 0.09, b2 = (k - 0.58) / 0.025;   // pow() is undefined for negative bases
    float band = exp(-b1 * b1) * 0.42 + exp(-b2 * b2) * 0.30;
    float rim = (1.0 - smoothstep(0.0, 3.0, -d)) * 0.25;
    float a = clamp(band + rim + 0.03, 0.0, 1.0) * mask;
    finalColor = vec4(1.0, 1.0, 1.0, a) * fragColor;
}
