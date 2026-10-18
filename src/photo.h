#pragma once
#include "content.h"
#include "raylib.h"
#include "render.h"
#include <vector>

// One plastic accordion sleeve: photo on the front, note on the back.
struct SleeveArt {
    RenderTexture2D front{};
    RenderTexture2D back{};
};

struct SleevePose {
    float drop = 0.0f;     // 0 folded up behind the previous sleeve, 1 hanging down
    float flip = 0.0f;     // 0 photo side, 1 note side
    float wobble = 0.0f;   // extra hinge angle (radians)
};

struct SleeveGeom {
    SurfaceFn f;
    Projector pj;
    Quad quad;
    bool front;
    float shade;
};

SleeveArt BakeSleeve(const Item& item, const Font& font, float aging, int seed, Theme theme);
void UnloadSleeve(SleeveArt& art);
RenderTexture2D BakeGloss();

// Chain the sleeves from `anchor`; stops at the first fully folded one.
std::vector<SleeveGeom> LayoutSleeves(const std::vector<SleevePose>& poses, Vector2 anchor);
void DrawSleeveShadow(const SleeveGeom& g, float alpha);
// `light` is a premultiplied tint; gloss.id == 0 skips the plastic sheen.
void DrawSleeve(const SleeveGeom& g, const SleeveArt& art, Texture2D gloss, Color light);
