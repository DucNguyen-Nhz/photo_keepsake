#pragma once
#include "raylib.h"
#include <vector>

// Damped spring driving a value toward `target`. Underdamped settings overshoot,
// which gives the flutter when a sleeve drops or the cover swings.
struct Spring {
    float x = 0.0f, v = 0.0f, target = 0.0f;
    float k = 120.0f, d = 12.0f;

    Spring() = default;
    Spring(float stiffness, float damping) : k(stiffness), d(damping) {}
    void Update(float dt);
    bool Settled(float eps = 0.002f) const;
};

struct Particle {
    Vector2 pos, vel;
    float life, maxLife, size;
};

class Dust {
public:
    void Burst(Vector2 from, Vector2 to, int count);
    void Update(float dt);
    void Draw() const;
    bool Empty() const { return parts_.empty(); }
private:
    std::vector<Particle> parts_;
};

float EaseOutCubic(float t);
float Frand(float lo, float hi);
