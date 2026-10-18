#include "gothic.h"
#include "anim.h"
#include "layout.h"
#include "note.h"
#include "render.h"
#include "wallet.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float P = L::kPad, TW = L::kWalletW, TH = L::kWalletH;
constexpr float W = L::kSleeveW, TAB = L::kSleeveTab, H = L::kSleeveH;
constexpr float kTexW = W + 2 * P, kTexH = TAB + H + 2 * P;
constexpr float kBorder = 15.0f;

const Color kGold = {184, 146, 72, 235};
const Color kGoldDim = {150, 116, 56, 200};
const Color kInk = {96, 12, 20, 230};

struct Rng {
    unsigned s;
    float Next() {
        s = s * 1664525u + 1013904223u;
        return (s >> 8) / 16777216.0f;
    }
    float Range(float lo, float hi) { return lo + (hi - lo) * Next(); }
};

Vector3 Leather() { return {0.20f, 0.042f, 0.052f}; }

void CenterText(const Font& f, const char* t, float cx, float y, float size, Color c) {
    Vector2 m = MeasureTextEx(f, t, size, 1.0f);
    DrawTextEx(f, t, {cx - m.x * 0.5f + 0.6f, y + 0.8f}, size, 1.0f, Color{10, 2, 4, 160});
    DrawTextEx(f, t, {cx - m.x * 0.5f, y}, size, 1.0f, c);
}

// Clip drawing inside a bake to `r` (1x units).
void BeginBakeClip(Rectangle r) {
    float s = L::kBakeScale;
    BeginScissorMode((int)(r.x * s), (int)(r.y * s), (int)(r.width * s), (int)(r.height * s));
}

// --- candle pieces --------------------------------------------------------
// A stub candle standing on (cx, baseY); returns the wick tip.
Vector2 DrawCandleStub(float cx, float baseY, float w, float h, unsigned seed) {
    Rng rng{seed};
    DrawEllipse((int)cx, (int)(baseY + 1), w * 1.1f, 2.6f, Color{90, 66, 30, 255});        // dish
    DrawEllipse((int)cx, (int)baseY, w * 1.05f, 2.2f, Color{176, 138, 64, 255});
    Rectangle body = {cx - w * 0.5f, baseY - h, w, h};
    DrawRectangleGradientH((int)body.x, (int)body.y, (int)std::ceil(body.width), (int)body.height,
                           Color{196, 184, 156, 255}, Color{236, 228, 206, 255});
    DrawEllipse((int)cx, (int)body.y, w * 0.5f, 1.6f, Color{246, 240, 222, 255});       // melted top
    for (int i = 0; i < 3; i++) {                                                         // drips
        float x = body.x + rng.Range(0.5f, w - 1.5f), len = rng.Range(3.0f, h * 0.6f);
        DrawRectangleRounded({x, body.y, 1.6f, len}, 1.0f, 4, Color{244, 238, 220, 255});
        DrawCircleV({x + 0.8f, body.y + len}, 1.1f, Color{244, 238, 220, 255});
    }
    DrawLineEx({cx, body.y + 0.5f}, {cx + 0.4f, body.y - 3.5f}, 0.9f, Color{30, 20, 15, 255});   // wick
    return {cx + 0.4f, body.y - 3.5f};
}

// --- tome -----------------------------------------------------------------
void CornerGuards(Rectangle r) {
    float s = 26.0f;
    struct C { Vector2 p; float dx, dy; } cs[4] = {
        {{r.x, r.y}, 1, 1}, {{r.x + r.width, r.y}, -1, 1},
        {{r.x + r.width, r.y + r.height}, -1, -1}, {{r.x, r.y + r.height}, 1, -1}};
    for (auto& c : cs) {
        Vector2 a = c.p, b = {c.p.x + c.dx * s, c.p.y}, d = {c.p.x, c.p.y + c.dy * s};
        DrawTriangle(a, b, d, Color{96, 70, 30, 255});
        DrawTriangle(a, d, b, Color{96, 70, 30, 255});   // either winding
        Vector2 a2 = {a.x + c.dx * 2, a.y + c.dy * 2};
        Vector2 b2 = {b.x - c.dx * 5, b.y + c.dy * 2}, d2 = {d.x + c.dx * 2, d.y - c.dy * 5};
        DrawTriangle(a2, b2, d2, Color{186, 148, 74, 255});
        DrawTriangle(a2, d2, b2, Color{186, 148, 74, 255});
        DrawCircleV({a.x + c.dx * 8.5f + 0.4f, a.y + c.dy * 8.5f + 0.6f}, 2.6f, Color{40, 25, 10, 200});
        DrawCircleV({a.x + c.dx * 8.5f, a.y + c.dy * 8.5f}, 2.4f, Color{232, 204, 140, 255});
    }
}

RenderTexture2D BakeCover(const Config& cfg, const Font& title) {
    RenderTexture2D rt = BakeBegin(TW + 2 * P, TH + 2 * P);
    Rectangle r = {P, P, TW, TH};
    FillLeather(r, 21.3f, Leather(), 6.0f, 0.55f + 0.45f * cfg.aging);
    DrawRectangleLinesEx({r.x + 9, r.y + 9, r.width - 18, r.height - 18}, 1.3f, kGold);
    DrawRectangleLinesEx({r.x + 13, r.y + 13, r.width - 26, r.height - 26}, 0.7f, kGoldDim);
    CornerGuards(r);

    Vector2 c = {r.x + TW * 0.5f, r.y + TH * 0.38f};
    DrawBatShape({c.x + 1.0f, c.y + 1.2f}, 86.0f, 0.25f, Color{8, 2, 4, 200});
    DrawBatShape(c, 86.0f, 0.25f, kGold);
    DrawBatShape({c.x, c.y + 1.0f}, 80.0f, 0.25f, Color{120, 26, 30, 255});
    const char* name = cfg.initials.empty() ? "Dracula" : cfg.initials.c_str();
    CenterText(title, name, c.x, r.y + TH * 0.62f, 30.0f, kGold);

    // clasp on the free edge
    Rectangle clasp = {r.x + TW - 17, r.y + TH * 0.5f - 19, 15, 38};
    DrawRectangleRounded({clasp.x + 0.8f, clasp.y + 1.2f, clasp.width, clasp.height}, 0.3f, 4, Color{10, 4, 2, 180});
    FillGilt(clasp, 4.0f, 3.3f);
    DrawRectangleRec({clasp.x + 4, clasp.y + 4, clasp.width - 8, clasp.height - 8}, Color{120, 92, 42, 255});
    DrawCircleV({clasp.x + clasp.width * 0.5f, clasp.y + clasp.height * 0.45f}, 2.0f, Color{12, 6, 4, 255});
    DrawTriangle({clasp.x + clasp.width * 0.5f, clasp.y + clasp.height * 0.45f},
                 {clasp.x + clasp.width * 0.5f - 1.6f, clasp.y + clasp.height * 0.62f},
                 {clasp.x + clasp.width * 0.5f + 1.6f, clasp.y + clasp.height * 0.62f}, Color{12, 6, 4, 255});
    BakeEnd(rt);
    return rt;
}

void DrawCastle(Rectangle e) {
    Color ink = {9, 5, 11, 255};
    Color lit = {255, 186, 92, 230};
    float gx = e.x + e.width * 0.30f, gy = e.y + e.height * 0.56f;   // cliff top
    // cliff
    DrawTriangle({e.x - 10, e.y + e.height}, {e.x + e.width * 0.7f, e.y + e.height}, {gx, gy}, ink);
    DrawTriangle({e.x - 10, e.y + e.height}, {gx, gy}, {e.x + e.width * 0.7f, e.y + e.height}, ink);
    DrawTriangle({e.x - 10, e.y + e.height}, {gx - 30, gy + 6}, {gx + 12, e.y + e.height}, ink);
    DrawTriangle({e.x - 10, e.y + e.height}, {gx + 12, e.y + e.height}, {gx - 30, gy + 6}, ink);
    DrawRectangleRec({e.x, e.y + e.height - 10, e.width, 10}, Color{12, 6, 12, 255});
    // keep, towers, roofs
    auto tower = [&](float x, float w, float h, float roof) {
        DrawRectangleRec({x, gy - h, w, h + 8}, ink);
        Vector2 a = {x - 2.5f, gy - h}, b = {x + w + 2.5f, gy - h}, t = {x + w * 0.5f, gy - h - roof};
        DrawTriangle(a, b, t, ink);
        DrawTriangle(a, t, b, ink);
        DrawLineEx(t, {t.x, t.y - 6}, 0.8f, ink);
    };
    DrawRectangleRec({gx - 22, gy - 34, 44, 42}, ink);
    for (int i = 0; i < 6; i++) DrawRectangleRec({gx - 22 + i * 8.0f, gy - 38, 4.5f, 5}, ink);
    tower(gx - 34, 13, 58, 22);
    tower(gx + 20, 11, 48, 18);
    tower(gx - 4, 9, 70, 26);
    DrawRectangleRec({gx + 30, gy - 22, 22, 28}, ink);
    tower(gx + 46, 8, 34, 14);
    // lit windows
    Rectangle wins[] = {{gx - 30, gy - 44, 2.2f, 3.6f}, {gx - 2, gy - 58, 2.2f, 3.6f},
                        {gx + 23, gy - 36, 2.2f, 3.6f}, {gx - 12, gy - 22, 2.2f, 3.2f},
                        {gx + 8, gy - 18, 2.2f, 3.2f}, {gx + 36, gy - 12, 2.0f, 3.0f}};
    for (auto& w : wins) {
        DrawCircleGradient((int)(w.x + 1), (int)(w.y + 2), 5.0f, Color{255, 160, 70, 60}, Color{255, 160, 70, 0});
        DrawRectangleRec(w, lit);
    }
}

RenderTexture2D BakeEndpaper(const Config& cfg) {
    RenderTexture2D rt = BakeBegin(TW + 2 * P, TH + 2 * P);
    Rectangle r = {P, P, TW, TH};
    FillLeather(r, 23.9f, Leather(), 6.0f, 0.55f + 0.45f * cfg.aging);
    Rectangle e = {r.x + 8, r.y + 8, r.width - 16, r.height - 16};
    BeginBakeClip(e);
    DrawRectangleGradientV((int)e.x, (int)e.y, (int)e.width, (int)e.height, Color{14, 10, 30, 255},
                           Color{74, 18, 34, 255});
    Rng rng{91u};
    for (int i = 0; i < 46; i++)
        DrawCircleV({rng.Range(e.x, e.x + e.width), rng.Range(e.y, e.y + e.height * 0.6f)},
                    rng.Range(0.3f, 0.8f), Color{235, 225, 255, (unsigned char)rng.Range(60, 200)});
    Vector2 moon = {e.x + e.width * 0.72f, e.y + e.height * 0.30f};
    DrawCircleGradient((int)moon.x, (int)moon.y, 62.0f, Color{240, 220, 190, 70}, Color{240, 220, 190, 0});
    DrawCircleV(moon, 21.0f, Color{238, 228, 204, 255});
    DrawCircleV({moon.x - 6, moon.y - 4}, 4.0f, Color{214, 202, 178, 255});
    DrawCircleV({moon.x + 7, moon.y + 5}, 5.5f, Color{220, 208, 184, 255});
    DrawCircleV({moon.x + 2, moon.y - 10}, 2.5f, Color{216, 204, 180, 255});
    DrawEllipse((int)(moon.x - 18), (int)(moon.y + 17), 34, 3.5f, Color{40, 20, 44, 110});
    DrawEllipse((int)(moon.x + 30), (int)(moon.y + 11), 26, 3, Color{40, 20, 44, 90});
    DrawCastle(e);
    DrawBatShape({moon.x - 30, moon.y - 8}, 12.0f, 0.6f, Color{10, 5, 12, 255});
    DrawBatShape({moon.x + 4, moon.y + 2}, 9.0f, -0.4f, Color{10, 5, 12, 255});
    DrawBatShape({moon.x + 30, moon.y - 18}, 7.0f, 0.2f, Color{10, 5, 12, 255});
    EndScissorMode();
    DrawRectangleLinesEx(e, 1.0f, kGoldDim);
    BakeEnd(rt);
    return rt;
}

constexpr float kPocketFrac = 0.46f;

Vector2 PageCandleBase() {   // in panel texture coordinates (1x, including padding)
    return {P + TW - 34, P + TH * kPocketFrac - 6};
}

RenderTexture2D BakePage(const Config& cfg, const Font& title) {
    RenderTexture2D rt = BakeBegin(TW + 2 * P, TH + 2 * P);
    Rectangle r = {P, P, TW, TH};
    float wear = 0.55f + 0.45f * cfg.aging;
    FillLeather(r, 25.1f, Leather(), 6.0f, wear);
    Rectangle page = {r.x + 8, r.y + 8, r.width - 16, r.height - 16};
    FillPaper(page, 4.2f, {0.84f, 0.76f, 0.58f}, 1.0f, std::min(1.0f, wear * 1.1f), 1.0f);
    DrawTextEx(title, "Ex Libris", {page.x + 12, page.y + 6}, 20.0f, 1.0f, Color{110, 20, 26, 210});
    for (int i = 0; i < 3; i++) {
        float y = page.y + 34 + i * 9.0f;
        DrawLineEx({page.x + 12, y}, {page.x + page.width - 60, y}, 0.6f, Color{90, 60, 40, 45});
    }
    Vector2 cb = PageCandleBase();
    DrawCandleStub(cb.x, cb.y, 10.0f, 30.0f, 7u);

    float pocketY = r.y + TH * kPocketFrac;
    for (int i = 0; i < 2; i++)   // tops of the hanging frames, tucked in the pocket
        FillGilt({r.x + (TW - W) * 0.5f + i * 2.0f, pocketY - 10 + i * 4.0f, W - i * 4.0f, 18}, 5.0f, 1.0f + i);
    Rectangle pocket = {r.x + 10, pocketY, TW - 20, TH * (1 - kPocketFrac) - 10};
    DrawRectangleRec({pocket.x, pocket.y - 1.5f, pocket.width, 2}, Color{10, 2, 4, 120});
    FillPaper(pocket, 6.6f, {0.34f, 0.05f, 0.07f}, 3.0f, 0.6f, 0.4f);
    DrawLineEx({pocket.x + 2, pocket.y + 1}, {pocket.x + pocket.width - 2, pocket.y + 1}, 1.6f, kGold);
    float in = 4.0f;
    Color thread = {196, 160, 90, 220};
    DrawStitchedLine({pocket.x + in, pocket.y + 5}, {pocket.x + in, pocket.y + pocket.height - in}, 3.6f, 2.4f, 1.0f, thread);
    DrawStitchedLine({pocket.x + in, pocket.y + pocket.height - in},
                     {pocket.x + pocket.width - in, pocket.y + pocket.height - in}, 3.6f, 2.4f, 1.0f, thread);
    DrawStitchedLine({pocket.x + pocket.width - in, pocket.y + pocket.height - in},
                     {pocket.x + pocket.width - in, pocket.y + 5}, 3.6f, 2.4f, 1.0f, thread);
    BakeEnd(rt);
    return rt;
}

// --- frames ---------------------------------------------------------------
Rectangle FrameRect() { return {P, P + TAB, W, H}; }
constexpr float kCandleX = P + W - 40, kCandleBase = P + TAB + H - 8;

void Chains() {
    for (float fx : {0.2f, 0.8f}) {
        float x = P + W * fx;
        DrawEllipse((int)x, (int)(P + 2), 2.6f, 4.8f, Color{58, 58, 64, 255});
        DrawEllipse((int)x, (int)(P + 2), 1.2f, 3.2f, Color{20, 20, 24, 255});
        DrawRectangleRec({x - 1.0f, P + 5, 2.0f, TAB}, Color{120, 120, 128, 255});
        DrawRectangleRec({x - 1.0f, P + 5, 0.8f, TAB}, Color{170, 170, 178, 255});
        DrawCircleV({x, P + TAB + 2.5f}, 2.2f, Color{150, 118, 58, 255});   // eye on the frame
    }
}

RenderTexture2D BakeFrameFront(const Item& item, float aging, float seed) {
    RenderTexture2D rt = BakeBegin(kTexW, kTexH);
    Chains();
    Rectangle f = FrameRect();
    DrawRectangleRec({f.x + 1.5f, f.y + 2.0f, f.width, f.height}, Color{5, 2, 3, 120});
    FillGilt(f, kBorder, seed);
    Rectangle mat = {f.x + kBorder, f.y + kBorder, f.width - 2 * kBorder, f.height - 2 * kBorder};
    FillPaper(mat, seed + 2.0f, {0.16f, 0.03f, 0.05f}, 0.0f, 0.3f, 0.0f);
    Rectangle photo = {mat.x + 5, mat.y + 5, mat.width - 10, mat.height - 10};
    Texture2D img = {};
    if (!item.imagePath.empty()) img = LoadTexture(item.imagePath.c_str());
    if (img.id != 0) {
        GenTextureMipmaps(&img);
        SetTextureFilter(img, TEXTURE_FILTER_TRILINEAR);
        DrawAgedPhoto(img, photo, seed, aging, 1.0f);
        UnloadTexture(img);
    } else {
        DrawRectangleRec(photo, Color{20, 14, 18, 255});
    }
    DrawRectangleLinesEx(photo, 1.2f, Color{0, 0, 0, 140});
    for (Vector2 c : {Vector2{f.x + 7, f.y + 7}, Vector2{f.x + f.width - 7, f.y + 7},
                      Vector2{f.x + f.width - 7, f.y + f.height - 7}, Vector2{f.x + 7, f.y + f.height - 7}}) {
        DrawCircleV({c.x + 0.5f, c.y + 0.7f}, 4.2f, Color{20, 12, 4, 160});   // rosettes
        DrawCircleV(c, 4.0f, Color{196, 158, 80, 255});
        DrawCircleV(c, 1.8f, Color{110, 26, 30, 255});
    }
    DrawCandleStub(kCandleX, kCandleBase, 9.0f, 24.0f, (unsigned)(seed * 1000));
    BakeEnd(rt);
    return rt;
}

RenderTexture2D BakeFrameBack(const Item& item, const Font& note, float aging, float seed, int iseed) {
    RenderTexture2D rt = BakeBegin(kTexW, kTexH);
    Chains();
    Rectangle f = FrameRect();
    FillPaper(f, seed + 5.0f, {0.24f, 0.15f, 0.09f}, 1.0f, 1.0f, 0.4f);   // old wood
    for (int i = 1; i < 5; i++) {
        float x = f.x + f.width * i / 5.0f;
        DrawLineEx({x, f.y + 2}, {x, f.y + f.height - 2}, 0.7f, Color{20, 10, 5, 120});
    }
    Rectangle board = {f.x + 12, f.y + 12, f.width - 24, f.height - 24};
    FillPaper(board, seed + 6.0f, {0.33f, 0.22f, 0.14f}, 0.0f, 0.8f, 0.2f);
    for (int i = 0; i < 4; i++) {   // nails holding the board
        float t = (i + 0.5f) / 4.0f;
        DrawCircleV({board.x + board.width * t, board.y + 2}, 1.1f, Color{60, 60, 64, 255});
        DrawCircleV({board.x + board.width * t, board.y + board.height - 2}, 1.1f, Color{60, 60, 64, 255});
    }
    Rectangle parch = {board.x + 8, board.y + 7, board.width - 16, board.height - 14};
    FillPaper(parch, seed + 7.0f, {0.86f, 0.78f, 0.59f}, 1.0f, aging, 1.4f);
    DrawCircleV({parch.x + 6, parch.y + 5}, 2.2f, Color{186, 150, 76, 255});
    DrawCircleV({parch.x + parch.width - 6, parch.y + 5}, 2.2f, Color{186, 150, 76, 255});
    DrawNote({parch.x + 10, parch.y + 10, parch.width - 34, parch.height - 16}, item.note, note, iseed, kInk);
    Vector2 seal = {parch.x + parch.width - 16, parch.y + parch.height - 15};
    DrawCircleV({seal.x + 0.6f, seal.y + 0.9f}, 10.5f, Color{40, 4, 6, 160});
    DrawCircleV(seal, 10.0f, Color{132, 14, 22, 255});
    DrawCircleV(seal, 7.0f, Color{112, 10, 18, 255});
    DrawBatShape(seal, 11.0f, 0.2f, Color{80, 6, 12, 255});
    BakeEnd(rt);
    return rt;
}
}  // namespace

TomeArt BakeTome(const Config& cfg, const Font& title) {
    return {BakeCover(cfg, title), BakeEndpaper(cfg), BakePage(cfg, title)};
}

SleeveArt BakeFrame(const Item& item, const Font& note, float aging, int seed) {
    float fs = 1.7f + seed * 3.13f;
    return {BakeFrameFront(item, aging, fs), BakeFrameBack(item, note, aging, fs, seed)};
}

Vector2 FrameWickST() {
    float tipY = kCandleBase - 24.0f - 3.5f;
    return {(kCandleX + 0.4f) / kTexW, tipY / kTexH};
}

Vector2 PageWick(int side) {
    Vector2 cb = PageCandleBase();
    return {PanelX0(side) - P + cb.x + 0.4f, L::kTop - P + cb.y - 30.0f - 3.5f};
}

float CandleFlicker(float time, float seed) {
    return std::sin(time * 13.0f + seed) * 0.5f + std::sin(time * 7.3f + seed * 2.1f) * 0.35f +
           std::sin(time * 23.1f + seed * 3.7f) * 0.15f;
}

void DrawCandleFlame(Vector2 wick, float scale, float time, float seed, float strength) {
    if (strength <= 0.01f) return;
    float n = CandleFlicker(time, seed);
    float h = (10.0f + 1.8f * n) * scale * strength;
    float w = 2.9f * scale * std::sqrt(strength);
    float sway = std::sin(time * 4.3f + seed) * 0.9f * scale;

    BeginAdditiveBlend();
    float glowR = 46.0f * scale * (0.92f + 0.08f * n);
    DrawCircleGradient((int)wick.x, (int)(wick.y - h * 0.4f), glowR,
                       Color{255, 150, 60, (unsigned char)(80 * strength)}, Color{255, 110, 40, 0});
    BeginStraightBlend();

    auto teardrop = [&](float sw, float sh, Color c) {
        Vector2 base = {wick.x, wick.y - sh * 0.22f};
        DrawEllipse((int)base.x, (int)base.y, sw, sh * 0.3f, c);
        Vector2 tip = {wick.x + sway, wick.y - sh};
        DrawTriangle({base.x - sw, base.y}, {base.x + sw, base.y}, tip, c);
        DrawTriangle({base.x - sw, base.y}, tip, {base.x + sw, base.y}, c);
    };
    teardrop(w, h, Color{255, 132, 36, (unsigned char)(200 * strength)});
    teardrop(w * 0.6f, h * 0.7f, Color{255, 214, 120, (unsigned char)(235 * strength)});
    teardrop(w * 0.3f, h * 0.4f, Color{255, 250, 225, (unsigned char)(250 * strength)});
    DrawEllipse((int)wick.x, (int)(wick.y - 0.5f), w * 0.55f, 1.4f, Color{110, 140, 255, (unsigned char)(110 * strength)});
}

void DrawBatShape(Vector2 c, float span, float flap, Color col) {
    float s = span;
    float ty = -flap * 0.22f;
    // right-wing outline (x >= 0); mirrored for the left wing
    const Vector2 wing[] = {{0.03f, 0.0f},   {0.06f, -0.04f}, {0.20f, -0.10f + ty * 0.4f},
                            {0.36f, -0.10f + ty * 0.8f}, {0.50f, ty - 0.02f},
                            {0.42f, ty * 0.7f + 0.10f}, {0.35f, ty * 0.5f + 0.05f},
                            {0.27f, ty * 0.4f + 0.12f}, {0.19f, ty * 0.25f + 0.06f},
                            {0.10f, 0.10f},  {0.04f, 0.07f}};
    const int n = sizeof(wing) / sizeof(wing[0]);
    for (int side : {1, -1}) {
        Vector2 pts[n + 1];
        pts[0] = {c.x + side * 0.02f * s, c.y + 0.03f * s};
        for (int i = 0; i < n; i++) pts[i + 1] = {c.x + side * wing[i].x * s, c.y + wing[i].y * s};
        DrawTriangleFan(pts, n + 1, col);
    }
    DrawEllipse((int)c.x, (int)(c.y + 0.03f * s), 0.05f * s, 0.10f * s, col);   // body
    DrawCircleV({c.x, c.y - 0.06f * s}, 0.045f * s, col);                         // head
    DrawTriangle({c.x - 0.04f * s, c.y - 0.08f * s}, {c.x - 0.015f * s, c.y - 0.08f * s},
                 {c.x - 0.035f * s, c.y - 0.14f * s}, col);
    DrawTriangle({c.x + 0.04f * s, c.y - 0.08f * s}, {c.x + 0.035f * s, c.y - 0.14f * s},
                 {c.x + 0.015f * s, c.y - 0.08f * s}, col);
}

void Bats::Burst(Vector2 from, int count) {
    for (int i = 0; i < count; i++) {
        Bat b;
        b.pos = {from.x + Frand(-20, 20), from.y + Frand(-30, 30)};
        b.vel = {Frand(-130, 130), Frand(-110, -40)};
        b.phase = Frand(0, 6.28f);
        b.rate = Frand(13, 19);
        b.span = Frand(14, 26);
        b.maxLife = b.life = Frand(1.6f, 2.6f);
        bats_.push_back(b);
    }
}

void Bats::Update(float dt) {
    for (auto& b : bats_) {
        b.life -= dt;
        b.phase += b.rate * dt;
        b.vel.y -= 25.0f * dt;
        b.vel.x += std::sin(b.phase * 0.35f) * 60.0f * dt;   // erratic flutter
        b.pos.x += b.vel.x * dt;
        b.pos.y += b.vel.y * dt + std::sin(b.phase) * 0.6f;
    }
    bats_.erase(std::remove_if(bats_.begin(), bats_.end(), [](const Bat& b) { return b.life <= 0; }),
                bats_.end());
}

void Bats::Draw() const {
    for (const auto& b : bats_) {
        float a = std::min(1.0f, b.life / 0.6f) * std::min(1.0f, (b.maxLife - b.life) / 0.15f);
        DrawBatShape(b.pos, b.span, std::sin(b.phase), Color{14, 8, 16, (unsigned char)(235 * a)});
    }
}
