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

// Carved, gilded picture-frame moulding along the edges of the quad. `border` and `size` in pixels.
uniform vec2 size;
uniform float border;
uniform float seed;

float g2(float x) { return exp(-x * x); }
// moulding height across the frame: outer bead, cove, raised ridge, inner bead
float prof(float t) {
    return 0.55 * g2((t - 0.10) / 0.07) + 0.25 * sin(t * 3.14159) +
           0.75 * g2((t - 0.58) / 0.12) + 0.45 * g2((t - 0.90) / 0.05);
}

void main() {
    vec2 p = fragTexCoord * size;
    float dl = p.x, dr = size.x - p.x, dt = p.y, db = size.y - p.y;
    float d = min(min(dl, dr), min(dt, db));
    float alpha = clamp(border - d + 0.5, 0.0, 1.0) * clamp(d + 0.5, 0.0, 1.0);
    float t = clamp(d / border, 0.0, 1.0);

    vec2 inward;
    float along;
    if (d == dl)      { inward = vec2(1.0, 0.0);  along = p.y; }
    else if (d == dr) { inward = vec2(-1.0, 0.0); along = p.y; }
    else if (d == dt) { inward = vec2(0.0, 1.0);  along = p.x; }
    else              { inward = vec2(0.0, -1.0); along = p.x; }

    float dh = (prof(t + 0.01) - prof(t - 0.01)) / 0.02;
    // carved ornament: beads on the inner bead, leaf notches on the ridge
    float bead = 0.5 + 0.5 * cos(along / border * 9.0);
    float leaf = 0.5 + 0.5 * cos(along / border * 4.0 + t * 6.0);
    float carve = g2((t - 0.90) / 0.05) * (bead - 0.5) * 2.5 + g2((t - 0.58) / 0.12) * (leaf - 0.5) * 1.6;

    vec3 N = normalize(vec3(-inward * (dh * 0.55 + carve * 0.35), 1.0));
    vec3 Ld = normalize(vec3(-0.45, -0.6, 0.75));
    float diff = max(dot(N, Ld), 0.0);
    float sp = max(dot(reflect(-Ld, N), vec3(0.0, 0.0, 1.0)), 0.0);
    sp = sp * sp; sp = sp * sp; sp = sp * sp; sp = sp * sp;   // ^16

    vec2 o = vec2(seed * 13.7, seed * 7.3);
    vec3 gold = vec3(0.70, 0.53, 0.22);
    vec3 col = gold * (0.18 + 1.0 * diff) + vec3(1.0, 0.86, 0.55) * sp * 0.55;
    col *= 0.6 + 0.4 * smoothstep(0.0, 0.5, prof(t));                     // grime in the grooves
    float tarnish = smoothstep(0.5, 0.8, fbm(p * 0.025 + o));
    col = mix(col, vec3(0.16, 0.12, 0.07), tarnish * 0.35);
    col *= 0.93 + 0.1 * noise(p * 0.8 + o);
    finalColor = vec4(col, alpha) * fragColor;
}
