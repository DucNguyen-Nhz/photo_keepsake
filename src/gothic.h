#pragma once
// Romantic Dracula theme: a love-letter grimoire instead of a wallet, gilded frames
// instead of plastic sleeves, rose petals and bats instead of dust.
#include "content.h"
#include "photo.h"
#include "raylib.h"
#include <vector>

struct TomeArt {
    RenderTexture2D cover, endpaper, page;
};
TomeArt BakeTome(const Config& cfg, const Font& title, const Font& script);
SleeveArt BakeFrame(const Item& item, const Font& note, float aging, int seed);

// Wick tip of the candlestick on the open page, in window coordinates.
Vector2 PageWick(int side);

float CandleFlicker(float time, float seed);   // roughly -1..1
void DrawCandleFlame(Vector2 wick, float scale, float time, float seed, float strength);
void DrawBatShape(Vector2 c, float span, float flap, Color col);
void DrawRose(Vector2 c, float r, Color petal);
// Little spider dangling on a thread from `anchor`.
void DrawSpider(Vector2 anchor, float time, float alpha);

class Petals {
public:
    void Burst(Vector2 from, Vector2 to, int count);
    void Update(float dt);
    void Draw() const;
private:
    struct Petal {
        Vector2 pos, vel;
        float angle, spin, flip, flipRate, size, life, maxLife;
    };
    std::vector<Petal> petals_;
};

class Bats {
public:
    void Burst(Vector2 from, int count);
    void Update(float dt);
    void Draw() const;
private:
    struct Bat {
        Vector2 pos, vel;
        float phase, rate, span, life, maxLife;
    };
    std::vector<Bat> bats_;
};
