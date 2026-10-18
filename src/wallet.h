#pragma once
#include "content.h"
#include "raylib.h"
#include "render.h"

struct WalletArt {
    RenderTexture2D outer{};   // outside of the cover
    RenderTexture2D inner{};   // inside of the cover (card pockets)
    RenderTexture2D panel{};   // right inside panel with the photo pocket
};

struct WalletPose {
    float cover = 0.0f;   // 0 closed, 1 fully open (spring may overshoot)
    float time = 0.0f;
    int side = 1;         // +1: panel right of the spine, cover opens left; -1: mirrored
};

struct WalletHits {
    Quad cover;
    Quad panel;
};

WalletArt BakeWallet(const Config& cfg, const Font& font);
void UnloadWallet(WalletArt& art);

void DrawWalletShadow(const WalletPose& pose);
WalletHits DrawWallet(const WalletArt& art, const WalletPose& pose);
// Left edge of the inside panel (the part that stays put when the cover opens).
float PanelX0(int side);
// Where the first photo sleeve hangs from (hidden inside the pocket).
Vector2 SleeveAnchor(int side);
