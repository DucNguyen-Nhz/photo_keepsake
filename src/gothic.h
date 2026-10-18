#pragma once
// Dracula-castle theme: a grimoire instead of a wallet, gilded frames with candles
// instead of plastic sleeves, bats instead of dust.
#include "content.h"
#include "photo.h"
#include "raylib.h"
#include <vector>

struct TomeArt {
    RenderTexture2D cover, endpaper, page;
};
TomeArt BakeTome(const Config& cfg, const Font& title);
SleeveArt BakeFrame(const Item& item, const Font& note, float aging, int seed);

// Wick tip of the candle on a frame, as surface coordinates (s,t) of the sleeve.
Vector2 FrameWickST();
// Wick tip of the candlestick on the open page, in window coordinates.
Vector2 PageWick(int side);

float CandleFlicker(float time, float seed);   // roughly -1..1
void DrawCandleFlame(Vector2 wick, float scale, float time, float seed, float strength);
void DrawBatShape(Vector2 c, float span, float flap, Color col);

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
