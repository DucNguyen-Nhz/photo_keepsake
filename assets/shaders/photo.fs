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

// Old photographic print: sepia, faded contrast, grain, vignette, foxing and a fold crease.
uniform sampler2D texture0;
uniform vec4 srcRect;     // normalized crop region of texture0
uniform vec2 size;        // output size in pixels
uniform float seed;
uniform float aging;

void main() {
    vec2 uv = (fragTexCoord - srcRect.xy) / srcRect.zw;
    vec2 o = vec2(seed * 13.7, seed * 7.3);
    vec3 c = texture(texture0, fragTexCoord).rgb;

    float l = dot(c, vec3(0.299, 0.587, 0.114));
    vec3 sep = vec3(l) * vec3(1.08, 0.95, 0.76) + vec3(0.03, 0.015, -0.01);
    sep = mix(vec3(0.42, 0.37, 0.29), sep, 0.86);                  // lifted blacks
    c = mix(c, sep, clamp(0.2 + 0.75 * aging, 0.0, 1.0));
    c = mix(c, vec3(0.93, 0.86, 0.70), 0.14 * aging);               // yellowed

    float v = 1.0 - smoothstep(0.3, 0.95, length((uv - 0.5) * vec2(1.0, 1.15)));
    c *= mix(1.0, 0.6 + 0.4 * v, aging);

    vec2 px = uv * size;
    c += (hash(floor(px) + o) - 0.5) * 0.10 * aging;                // grain
    float fox = smoothstep(0.74, 0.82, fbm(uv * 7.0 + o));
    c = mix(c, vec3(0.62, 0.47, 0.27), fox * 0.35 * aging);

    // fold crease down the middle: bright ridge + soft shadow on one side
    float cx = uv.x - 0.5 - 0.006 * sin(uv.y * 9.0 + seed);
    c = mix(c, vec3(0.96, 0.93, 0.86), exp(-cx * cx * 60000.0) * 0.55 * aging);
    c *= 1.0 - 0.14 * aging * (1.0 - smoothstep(0.0, 0.02, cx)) * step(0.0, cx);
    // a few fine scratches
    float sx = fract(uv.x * 3.0 + fbm(uv * 3.0 + o) * 0.9);
    c = mix(c, vec3(0.95, 0.92, 0.85), (1.0 - smoothstep(0.0, 0.0025, abs(sx - 0.5))) *
                                        step(0.7, noise(uv * vec2(2.0, 14.0) + o)) * 0.35 * aging);

    finalColor = vec4(clamp(c, 0.0, 1.0), 1.0) * fragColor;
}
