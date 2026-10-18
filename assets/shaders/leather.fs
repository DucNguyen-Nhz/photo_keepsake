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

// Pebbled, worn leather. `size` is in pixels.
uniform vec2 size;
uniform float seed;
uniform vec3 base;
uniform float radius;
uniform float wear;

float voronoi(vec2 x) {
    vec2 n = floor(x), f = fract(x);
    float md = 8.0;
    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++) {
            vec2 g = vec2(i, j);
            vec2 r = g + hash2(n + g) - f;
            md = min(md, dot(r, r));
        }
    return sqrt(md);
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 p = uv * size;
    vec2 o = vec2(seed * 13.7, seed * 7.3);
    float d = sdRoundBox(p - size * 0.5, size * 0.5, radius);
    d += (fbm(p * 0.05 + o) - 0.5) * 1.5;
    float alpha = clamp(0.5 - d, 0.0, 1.0);

    vec3 col = base * (0.80 + 0.35 * fbm(p * 0.008 + o));
    float v = voronoi(p / 5.5 + o);
    col *= 1.0 - 0.16 * smoothstep(0.35, 0.95, v);               // pebble crevices
    col *= 0.96 + 0.08 * noise(p * 0.7 + o);                      // fine grain
    float ridge = 1.0 - abs(2.0 * fbm(p * 0.005 + o * 1.7) - 1.0);
    col *= 1.0 - 0.14 * smoothstep(0.86, 1.0, ridge) * wear;      // creases
    float scuff = smoothstep(0.58, 0.8, fbm(p * 0.012 + o * 3.1));
    col = mix(col, col * 1.45 + vec3(0.05, 0.03, 0.0), scuff * 0.32 * wear);
    float inside = -d;
    col *= 1.0 - 0.38 * (1.0 - smoothstep(0.0, 34.0, inside));   // burnished, darker border
    col = mix(col, col * 1.6, (1.0 - smoothstep(1.5, 6.0, inside)) * 0.45 * wear); // rubbed edge
    col *= 0.92 + 0.16 * (1.0 - uv.y * 0.6 - uv.x * 0.4);         // soft sheen
    finalColor = vec4(col, alpha) * fragColor;
}
