#include "gothic.h"
#include "anim.h"
#include "layout.h"
#include "note.h"
#include "content.h"
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

Vector3 Leather() { return {0.29f, 0.045f, 0.085f}; }   // burgundy

void CenterText(const Font& f, const char* t, float cx, float y, float size, Color c) {
    Vector2 m = MeasureTextEx(f, t, size, 1.0f);
    DrawTextEx(f, t, {cx - m.x * 0.5f + 0.6f, y + 0.8f}, size, 1.0f, Color{10, 2, 4, 160});
    DrawTextEx(f, t, {cx - m.x * 0.5f, y}, size, 1.0f, c);
}

// Corner cobweb: spokes fanning from `corner` into the quadrant (dx, dy), joined by sagging rings.
void DrawCobweb(Vector2 corner, float size, float dx, float dy, unsigned seed, unsigned char alpha) {
    Rng rng{seed * 2654435761u + 17u};
    const int spokes = 7;
    Vector2 dir[spokes];
    for (int i = 0; i < spokes; i++) {
        float t = (float)i / (spokes - 1) * PI * 0.5f;
        if (i > 0 && i < spokes - 1) t += rng.Range(-0.08f, 0.08f);
        dir[i] = {dx * std::cos(t), dy * std::sin(t)};
        float len = size * rng.Range(0.8f, 1.0f);
        DrawLineEx(corner, {corner.x + dir[i].x * len, corner.y + dir[i].y * len}, 0.5f,
                   Color{228, 226, 232, alpha});
    }
    for (int k = 1; k <= 5; k++) {
        float rad = size * k / 5.6f;
        for (int i = 0; i + 1 < spokes; i++) {
            float r0 = rad * rng.Range(0.94f, 1.04f), r1 = rad * rng.Range(0.94f, 1.04f);
            Vector2 p = {corner.x + dir[i].x * r0, corner.y + dir[i].y * r0};
            Vector2 q = {corner.x + dir[i + 1].x * r1, corner.y + dir[i + 1].y * r1};
            Vector2 m = {(p.x + q.x) * 0.5f, (p.y + q.y) * 0.5f};
            m = {m.x + (corner.x - m.x) * 0.14f, m.y + (corner.y - m.y) * 0.14f};   // sag
            Color c = {228, 226, 232, (unsigned char)(alpha * 0.8f)};
            DrawLineEx(p, m, 0.4f, c);
            DrawLineEx(m, q, 0.4f, c);
        }
    }
    // a broken strand drifting loose
    int i = 2 + (int)rng.Range(0, 3);
    Vector2 a = {corner.x + dir[i].x * size * 0.6f, corner.y + dir[i].y * size * 0.6f};
    Vector2 b = {a.x + dx * size * 0.15f, a.y + dy * size * 0.45f};
    DrawLineEx(a, b, 0.4f, Color{228, 226, 232, (unsigned char)(alpha * 0.6f)});
}

// Fine dust specks over `r`, and an optional dusty film along the top edge.
void DrawDust(Rectangle r, unsigned seed, int count, bool topFilm) {
    Rng rng{seed * 747796405u + 3u};
    for (int i = 0; i < count; i++)
        DrawCircleV({rng.Range(r.x, r.x + r.width), rng.Range(r.y, r.y + r.height)}, rng.Range(0.25f, 0.75f),
                    Color{214, 208, 196, (unsigned char)rng.Range(25, 85)});
    if (topFilm)
        DrawRectangleGradientV((int)r.x, (int)r.y, (int)r.width, 6, Color{206, 200, 188, 70},
                               Color{206, 200, 188, 0});
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

    Vector2 c = {r.x + TW * 0.5f, r.y + TH * 0.36f};
    DrawBatShape({c.x + 1.0f, c.y + 1.2f}, 96.0f, 0.15f, Color{8, 2, 4, 200});
    DrawBatShape(c, 96.0f, 0.15f, kGoldDim);
    DrawBatShape({c.x, c.y + 0.8f}, 90.0f, 0.15f, Color{70, 10, 22, 255});
    DrawCircleV({c.x + 0.8f, c.y + 2.2f}, 12.0f, Color{8, 2, 4, 150});
    DrawRose({c.x, c.y + 1.0f}, 11.0f, Color{170, 18, 40, 255});
    const char* name = cfg.initials.empty() ? "Dracula" : cfg.initials.c_str();
    CenterText(title, name, c.x, r.y + TH * 0.58f, 30.0f, kGold);
    CenterText(title, "amor aeternus", c.x, r.y + TH * 0.78f, 13.0f, kGoldDim);

    // clasp on the free edge
    Rectangle clasp = {r.x + TW - 17, r.y + TH * 0.5f - 19, 15, 38};
    DrawRectangleRounded({clasp.x + 0.8f, clasp.y + 1.2f, clasp.width, clasp.height}, 0.3f, 4, Color{10, 4, 2, 180});
    FillGilt(clasp, 4.0f, 3.3f);
    DrawRectangleRec({clasp.x + 4, clasp.y + 4, clasp.width - 8, clasp.height - 8}, Color{120, 92, 42, 255});
    DrawCircleV({clasp.x + clasp.width * 0.5f, clasp.y + clasp.height * 0.45f}, 2.0f, Color{12, 6, 4, 255});
    DrawTriangle({clasp.x + clasp.width * 0.5f, clasp.y + clasp.height * 0.45f},
                 {clasp.x + clasp.width * 0.5f - 1.6f, clasp.y + clasp.height * 0.62f},
                 {clasp.x + clasp.width * 0.5f + 1.6f, clasp.y + clasp.height * 0.62f}, Color{12, 6, 4, 255});
    DrawDust(r, 13u, 170, true);
    DrawCobweb({r.x + 3, r.y + 3}, 46.0f, 1, 1, 11u, 110);
    DrawCobweb({r.x + TW - 3, r.y + TH - 3}, 30.0f, -1, -1, 12u, 90);
    BakeEnd(rt);
    return rt;
}

void Damask(Rectangle e) {
    DrawRectangleGradientV((int)e.x, (int)e.y, (int)e.width, (int)e.height, Color{58, 8, 20, 255},
                           Color{34, 4, 12, 255});
    for (float y = e.y + 6; y < e.y + e.height; y += 22)
        for (float x = e.x + 6 + std::fmod(y, 44.0f) * 0.5f; x < e.x + e.width; x += 22) {
            Color c = {92, 18, 36, 70};
            DrawTriangle({x, y - 5}, {x - 3, y}, {x + 3, y}, c);   // little fleur
            DrawTriangle({x, y - 5}, {x + 3, y}, {x - 3, y}, c);
            DrawTriangle({x - 3, y}, {x, y + 5}, {x + 3, y}, c);
            DrawTriangle({x - 3, y}, {x + 3, y}, {x, y + 5}, c);
            DrawCircleV({x - 4, y - 3}, 1.0f, c);
            DrawCircleV({x + 4, y - 3}, 1.0f, c);
        }
}

// Rose stem with leaves from `a` to `b`, rose head at `b`.
void RoseSprig(Vector2 a, Vector2 b, float r, Color petal) {
    Color stem = {44, 70, 34, 255}, leaf = {56, 92, 42, 255};
    DrawLineEx(a, b, 1.4f, stem);
    Vector2 m = {a.x + (b.x - a.x) * 0.45f, a.y + (b.y - a.y) * 0.45f};
    for (float s : {1.0f, -1.0f}) {
        Vector2 tip = {m.x + s * r * 1.2f, m.y - r * 0.5f};
        DrawTriangle(m, tip, {m.x + s * r * 0.5f, m.y + r * 0.35f}, leaf);
        DrawTriangle(m, {m.x + s * r * 0.5f, m.y + r * 0.35f}, tip, leaf);
        DrawTriangle(m, {m.x + s * r * 0.4f, m.y - r * 0.6f}, tip, leaf);
        DrawTriangle(m, tip, {m.x + s * r * 0.4f, m.y - r * 0.6f}, leaf);
        m = {a.x + (b.x - a.x) * 0.7f, a.y + (b.y - a.y) * 0.7f};
    }
    DrawRose(b, r, petal);
}

RenderTexture2D BakeEndpaper(const Config& cfg) {
    RenderTexture2D rt = BakeBegin(TW + 2 * P, TH + 2 * P);
    Rectangle r = {P, P, TW, TH};
    FillLeather(r, 23.9f, Leather(), 6.0f, 0.55f + 0.45f * cfg.aging);
    Rectangle e = {r.x + 8, r.y + 8, r.width - 16, r.height - 16};
    BeginBakeClip(e);
    Damask(e);
    // candle glow behind the painting
    DrawCircleGradient((int)(e.x + e.width * 0.5f), (int)(e.y + e.height * 0.5f), 110.0f,
                       Color{255, 140, 80, 40}, Color{255, 140, 80, 0});
    float oh = e.height - 6, ow = oh * 0.76f;
    Rectangle oval = {e.x + (e.width - ow) * 0.5f, e.y + 3, ow, oh};
    DrawEllipse((int)(oval.x + ow * 0.5f + 1.5f), (int)(oval.y + oh * 0.5f + 2.5f), ow * 0.5f, oh * 0.5f,
                Color{10, 0, 4, 150});
    std::string path = AssetDir() + "gothic/the_kiss_hayez.jpg";
    Texture2D img = FileExists(path.c_str()) ? LoadTexture(path.c_str()) : Texture2D{};
    if (img.id != 0) {
        GenTextureMipmaps(&img);
        SetTextureFilter(img, TEXTURE_FILTER_TRILINEAR);
        float b = 8.0f;
        DrawAgedPhoto(img, {oval.x + b - 1, oval.y + b - 1, ow - 2 * b + 2, oh - 2 * b + 2}, 31.0f, cfg.aging, 2.0f);
        UnloadTexture(img);
    }
    FillGilt(oval, 8.0f, 12.0f, true);
    RoseSprig({oval.x - 26, e.y + e.height + 4}, {oval.x - 4, oval.y + oh - 14}, 7.0f, Color{160, 16, 36, 255});
    RoseSprig({oval.x + ow + 26, e.y + e.height + 4}, {oval.x + ow + 4, oval.y + oh - 14}, 7.0f,
              Color{160, 16, 36, 255});
    DrawDust(e, 23u, 120, false);
    DrawCobweb({e.x + e.width, e.y}, 52.0f, -1, 1, 21u, 125);
    DrawCobweb({e.x, e.y}, 30.0f, 1, 1, 22u, 100);
    EndScissorMode();
    DrawRectangleLinesEx(e, 1.0f, kGoldDim);
    BakeEnd(rt);
    return rt;
}

constexpr float kPocketFrac = 0.46f;

Vector2 PageCandleBase() {   // in panel texture coordinates (1x, including padding)
    return {P + TW - 34, P + TH * kPocketFrac - 6};
}

RenderTexture2D BakePage(const Config& cfg, const Font& title, const Font& script) {
    RenderTexture2D rt = BakeBegin(TW + 2 * P, TH + 2 * P);
    Rectangle r = {P, P, TW, TH};
    float wear = 0.55f + 0.45f * cfg.aging;
    FillLeather(r, 25.1f, Leather(), 6.0f, wear);
    Rectangle page = {r.x + 8, r.y + 8, r.width - 16, r.height - 16};
    FillPaper(page, 4.2f, {0.84f, 0.76f, 0.58f}, 1.0f, std::min(1.0f, wear * 1.1f), 1.0f);
    DrawTextEx(title, "My Mina,", {page.x + 12, page.y + 5}, 17.0f, 1.0f, Color{110, 16, 30, 220});
    DrawTextPro(script, "Across oceans of time,", {page.x + 22, page.y + 26}, {0, 0}, -1.5f, 14.0f, 0,
                Color{100, 14, 28, 220});
    DrawTextPro(script, "I would find you.", {page.x + 34, page.y + 41}, {0, 0}, -1.5f, 14.0f, 0,
                Color{100, 14, 28, 220});
    RoseSprig({page.x + page.width - 92, page.y + 66}, {page.x + page.width - 62, page.y + 50}, 5.5f,
              Color{120, 20, 34, 220});   // pressed, slightly faded
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
    DrawDust(page, 31u, 110, true);
    DrawCobweb({pocket.x + 1, pocket.y + pocket.height - 1}, 32.0f, 1, -1, 32u, 110);
    BakeEnd(rt);
    return rt;
}

// --- frames ---------------------------------------------------------------
Rectangle FrameRect() { return {P, P + TAB, W, H}; }

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
    unsigned us = (unsigned)(seed * 1000);
    Rng rng{us};
    DrawDust(f, us + 1, 150, true);
    bool left = rng.Next() < 0.5f;
    DrawCobweb({left ? f.x + 2 : f.x + f.width - 2, f.y + 2}, rng.Range(32.0f, 46.0f), left ? 1.0f : -1.0f, 1,
               us + 2, 125);
    if (rng.Next() < 0.5f) DrawCobweb({f.x + 2, f.y + f.height - 2}, rng.Range(20.0f, 28.0f), 1, -1, us + 3, 100);
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
    DrawRose(seal, 4.6f, Color{96, 8, 16, 255});
    DrawDust(f, (unsigned)(seed * 1000) + 9, 120, true);
    DrawCobweb({f.x + f.width - 2, f.y + 2}, 34.0f, -1, 1, (unsigned)(seed * 1000) + 8, 110);
    BakeEnd(rt);
    return rt;
}
}  // namespace

TomeArt BakeTome(const Config& cfg, const Font& title, const Font& script) {
    return {BakeCover(cfg, title), BakeEndpaper(cfg), BakePage(cfg, title, script)};
}

SleeveArt BakeFrame(const Item& item, const Font& note, float aging, int seed) {
    float fs = 1.7f + seed * 3.13f;
    return {BakeFrameFront(item, aging, fs), BakeFrameBack(item, note, aging, fs, seed)};
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

void DrawRose(Vector2 c, float r, Color petal) {
    auto shade = [&](float k) {
        return Color{(unsigned char)(petal.r * k), (unsigned char)(petal.g * k), (unsigned char)(petal.b * k), petal.a};
    };
    for (int i = 0; i < 5; i++) {   // outer petals
        float a = i * 72.0f * DEG2RAD - PI / 2;
        Vector2 p = {c.x + std::cos(a) * r * 0.5f, c.y + std::sin(a) * r * 0.5f};
        DrawCircleV(p, r * 0.55f, shade(0.62f));
        DrawCircleV({p.x - std::cos(a) * r * 0.08f, p.y - std::sin(a) * r * 0.08f}, r * 0.45f, shade(0.85f));
    }
    for (int i = 0; i < 4; i++) {   // inner petals
        float a = (i * 90.0f + 30.0f) * DEG2RAD;
        DrawCircleV({c.x + std::cos(a) * r * 0.25f, c.y + std::sin(a) * r * 0.25f}, r * 0.36f, shade(1.0f));
    }
    DrawRing(c, r * 0.12f, r * 0.2f, 20, 300, 12, shade(0.55f));   // swirl
    DrawRing(c, r * 0.28f, r * 0.34f, 200, 470, 12, shade(0.6f));
    DrawCircleV(c, r * 0.12f, shade(0.5f));
}

void Petals::Burst(Vector2 from, Vector2 to, int count) {
    for (int i = 0; i < count; i++) {
        float t = Frand(0, 1);
        Petal p;
        p.pos = {from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t};
        p.vel = {Frand(-30, 30), Frand(-25, 15)};
        p.angle = Frand(0, 360);
        p.spin = Frand(-90, 90);
        p.flip = Frand(0, 6.28f);
        p.flipRate = Frand(2, 5);
        p.size = Frand(5.0f, 8.0f);
        p.maxLife = p.life = Frand(3.5f, 6.0f);
        petals_.push_back(p);
    }
}

void Petals::Update(float dt) {
    for (auto& p : petals_) {
        p.life -= dt;
        p.vel.y += (22.0f - p.vel.y) * 0.9f * dt;                 // drift down at terminal speed
        p.vel.x += std::sin(p.flip * 0.7f) * 18.0f * dt;          // sway
        p.vel.x *= 1.0f - 0.4f * dt;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.angle += p.spin * dt;
        p.flip += p.flipRate * dt;
    }
    petals_.erase(std::remove_if(petals_.begin(), petals_.end(), [](const Petal& p) { return p.life <= 0; }),
                  petals_.end());
}

void Petals::Draw() const {
    for (const auto& p : petals_) {
        float a = std::min(1.0f, p.life / 1.0f) * std::min(1.0f, (p.maxLife - p.life) / 0.3f);
        float w = p.size * (0.35f + 0.65f * std::fabs(std::cos(p.flip)));   // tumbling
        float k = 0.75f + 0.25f * std::cos(p.flip);
        Color c = {(unsigned char)(170 * k), (unsigned char)(16 * k), (unsigned char)(38 * k), (unsigned char)(230 * a)};
        float ca = std::cos(p.angle * DEG2RAD), sa = std::sin(p.angle * DEG2RAD);
        auto pt = [&](float x, float y) { return Vector2{p.pos.x + x * ca - y * sa, p.pos.y + x * sa + y * ca}; };
        // teardrop petal from a fan of points
        Vector2 pts[9];
        pts[0] = pt(0, 0);
        for (int i = 0; i < 8; i++) {
            float t = i / 7.0f * PI;
            float rr = p.size * (0.6f + 0.4f * std::sin(t));
            pts[i + 1] = pt(std::cos(t) * w, -std::sin(t) * rr - p.size * 0.2f);
        }
        DrawTriangleFan(pts, 9, c);
        DrawTriangle(pt(-w * 0.6f, -p.size * 0.15f), pt(0, p.size * 0.5f), pt(w * 0.6f, -p.size * 0.15f), c);
        DrawTriangle(pt(-w * 0.6f, -p.size * 0.15f), pt(w * 0.6f, -p.size * 0.15f), pt(0, p.size * 0.5f), c);
    }
}

void DrawSpider(Vector2 anchor, float time, float alpha) {
    if (alpha <= 0.01f) return;
    float drop = 28.0f + 60.0f * (0.5f + 0.5f * std::sin(time * 0.33f));   // lowers and climbs back
    Vector2 body = {anchor.x + std::sin(time * 1.3f) * 2.5f, anchor.y + drop};
    DrawLineEx(anchor, {body.x, body.y - 2.0f}, 0.6f, Color{225, 225, 232, (unsigned char)(150 * alpha)});
    Color ink = {18, 12, 16, (unsigned char)(240 * alpha)};
    for (int side : {-1, 1}) {
        for (int i = 0; i < 4; i++) {
            float a = (-50.0f + i * 32.0f) * DEG2RAD + std::sin(time * 5.0f + i + side) * 0.08f;
            Vector2 knee = {body.x + side * std::cos(a) * 3.8f, body.y + std::sin(a) * 3.2f - 2.0f};
            Vector2 foot = {knee.x + side * 2.6f, knee.y + 3.2f};
            DrawLineEx(body, knee, 0.6f, ink);
            DrawLineEx(knee, foot, 0.5f, ink);
        }
    }
    DrawCircleV({body.x, body.y + 1.6f}, 2.6f, ink);   // abdomen
    DrawCircleV({body.x, body.y - 1.4f}, 1.6f, ink);   // head
    DrawCircleV({body.x - 0.8f, body.y + 1.0f}, 0.6f, Color{120, 20, 30, (unsigned char)(200 * alpha)});
}
