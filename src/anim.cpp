#include "anim.h"
#include <algorithm>
#include <cmath>

void Spring::Update(float dt) {
    dt = std::min(dt, 1.0f / 20.0f);
    int steps = std::max(1, (int)std::ceil(dt * 240.0f));
    float h = dt / steps;
    for (int i = 0; i < steps; i++) {
        float a = k * (target - x) - d * v;
        v += a * h;
        x += v * h;
    }
}

bool Spring::Settled(float eps) const {
    return std::fabs(target - x) < eps && std::fabs(v) < eps * 10.0f;
}

float EaseOutCubic(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    float u = 1.0f - t;
    return 1.0f - u * u * u;
}

float Frand(float lo, float hi) {
    return lo + (hi - lo) * (GetRandomValue(0, 10000) / 10000.0f);
}

void Dust::Burst(Vector2 from, Vector2 to, int count) {
    for (int i = 0; i < count; i++) {
        float t = Frand(0.0f, 1.0f);
        Particle p;
        p.pos = {from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t};
        p.vel = {Frand(-38.0f, 38.0f), Frand(-30.0f, 6.0f)};
        p.maxLife = p.life = Frand(0.9f, 2.2f);
        p.size = Frand(0.6f, 1.8f);
        parts_.push_back(p);
    }
}

void Dust::Update(float dt) {
    for (auto& p : parts_) {
        p.life -= dt;
        p.vel.x *= 1.0f - 1.6f * dt;
        p.vel.y = p.vel.y * (1.0f - 1.2f * dt) + 9.0f * dt;   // drifts, then settles slowly
        p.vel.x += std::sin((p.pos.y + p.life * 60.0f) * 0.05f) * 6.0f * dt;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
    }
    parts_.erase(std::remove_if(parts_.begin(), parts_.end(),
                                [](const Particle& p) { return p.life <= 0.0f; }),
                 parts_.end());
}

void Dust::Draw() const {
    for (const auto& p : parts_) {
        float f = p.life / p.maxLife;
        float a = std::sin(f * PI) * 0.8f;   // fade in and out
        DrawCircleV(p.pos, p.size, Color{235, 220, 185, (unsigned char)(a * 255)});
    }
}
