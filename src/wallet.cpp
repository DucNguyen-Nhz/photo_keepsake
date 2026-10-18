#include "wallet.h"
#include "gothic.h"
#include "layout.h"
#include <cmath>

float PanelX0(int side);

namespace {
constexpr float P = L::kPad, W = L::kWalletW, H = L::kWalletH;

const Color kThread = {201, 174, 122, 235};

float Roundness(float radius, Rectangle r) {
    return std::fmin(1.0f, 2.0f * radius / std::fmin(r.width, r.height));
}

void Emboss(const Font& font, const char* text, Vector2 pos, float size) {
    DrawTextEx(font, text, {pos.x - 0.6f, pos.y - 0.8f}, size, 1.0f, Color{0, 0, 0, 120});
    DrawTextEx(font, text, {pos.x + 0.5f, pos.y + 0.7f}, size, 1.0f, Color{255, 225, 180, 45});
    DrawTextEx(font, text, pos, size, 1.0f, Color{60, 34, 18, 110});
}

// Leather pocket stitched on three sides, with a rubbed top edge.
void Pocket(Rectangle r, float seed, Vector3 base, float wear) {
    DrawRectangleRounded({r.x, r.y - 1.0f, r.width, r.height + 1.0f}, Roundness(5, r), 6,
                         Color{25, 12, 6, 90});   // shadow line above the pocket lip
    FillLeather(r, seed, base, 5.0f, wear);
    DrawLineEx({r.x + 4, r.y + 1.2f}, {r.x + r.width - 4, r.y + 1.2f}, 1.0f, Color{255, 220, 170, 55});
    float in = 4.0f;
    DrawStitchedLine({r.x + in, r.y + 5}, {r.x + in, r.y + r.height - in}, 3.6f, 2.4f, 1.1f, kThread);
    DrawStitchedLine({r.x + in, r.y + r.height - in}, {r.x + r.width - in, r.y + r.height - in},
                     3.6f, 2.4f, 1.1f, kThread);
    DrawStitchedLine({r.x + r.width - in, r.y + r.height - in}, {r.x + r.width - in, r.y + 5},
                     3.6f, 2.4f, 1.1f, kThread);
}

RenderTexture2D BakeOuter(const Config& cfg, const Font& font) {
    RenderTexture2D rt = BakeBegin(W + 2 * P, H + 2 * P);
    Rectangle r = {P, P, W, H};
    FillLeather(r, 3.1f, {0.34f, 0.205f, 0.12f}, L::kWalletR, 0.6f + 0.4f * cfg.aging);
    DrawStitchedRoundRect({r.x + 7, r.y + 7, r.width - 14, r.height - 14}, 9.0f, 4.2f, 2.6f, 1.3f,
                          kThread);
    if (!cfg.initials.empty()) {
        float size = 28.0f;
        Vector2 m = MeasureTextEx(font, cfg.initials.c_str(), size, 1.0f);
        Emboss(font, cfg.initials.c_str(), {r.x + r.width - m.x - 22, r.y + r.height - m.y - 16}, size);
    }
    BakeEnd(rt);
    return rt;
}

RenderTexture2D BakeInner(const Config& cfg, const Font& font) {
    RenderTexture2D rt = BakeBegin(W + 2 * P, H + 2 * P);
    Rectangle r = {P, P, W, H};
    float wear = 0.5f + 0.5f * cfg.aging;
    FillLeather(r, 5.7f, {0.43f, 0.275f, 0.165f}, L::kWalletR, wear);
    DrawStitchedRoundRect({r.x + 7, r.y + 7, r.width - 14, r.height - 14}, 9.0f, 4.2f, 2.6f, 1.3f,
                          kThread);

    // an old ticket tucked behind the first pocket
    Rectangle ticket = {r.x + 26, r.y + 20, 118, 52};
    FillPaper(ticket, 9.2f, {0.86f, 0.80f, 0.66f}, 1.0f, cfg.aging);
    DrawRectangleLinesEx({ticket.x + 4, ticket.y + 4, ticket.width - 8, ticket.height - 8}, 0.7f,
                         Color{150, 50, 40, 110});
    DrawTextEx(font, "ADMIT ONE", {ticket.x + 10, ticket.y + 7}, 15.0f, 1.0f, Color{150, 50, 40, 150});
    DrawTextEx(font, "No 0417", {ticket.x + 70, ticket.y + 9}, 11.0f, 1.0f, Color{60, 40, 30, 120});

    Vector3 pocket = {0.40f, 0.255f, 0.152f};
    Pocket({r.x + 12, r.y + 46, r.width - 24, r.height - 58}, 7.3f, pocket, wear);
    Pocket({r.x + 12, r.y + 82, r.width - 24, r.height - 94}, 8.1f, pocket, wear);
    Pocket({r.x + 12, r.y + 118, r.width - 24, r.height - 130}, 8.9f, pocket, wear);
    BakeEnd(rt);
    return rt;
}

RenderTexture2D BakePanel(const Config& cfg) {
    RenderTexture2D rt = BakeBegin(W + 2 * P, H + 2 * P);
    Rectangle r = {P, P, W, H};
    float wear = 0.5f + 0.5f * cfg.aging;
    FillLeather(r, 11.4f, {0.41f, 0.26f, 0.155f}, L::kWalletR, wear);
    DrawStitchedRoundRect({r.x + 7, r.y + 7, r.width - 14, r.height - 14}, 9.0f, 4.2f, 2.6f, 1.3f,
                          kThread);

    // top edges of the folded photo sleeves peeking out of the pocket
    float pocketY = r.y + H * 0.46f;
    for (int i = 0; i < 3; i++) {
        Rectangle e = {r.x + (W - L::kSleeveW) * 0.5f + i * 1.5f, pocketY - 11 + i * 3.0f,
                       L::kSleeveW - i * 3.0f, 20};
        DrawRectangleRounded(e, Roundness(4, e), 4, Color{200, 205, 200, 55});
        DrawRectangleRoundedLinesEx(e, Roundness(4, e), 4, 0.8f, Color{240, 236, 220, 90});
    }
    Pocket({r.x + 10, pocketY, W - 20, H * 0.54f - 10}, 12.9f, {0.38f, 0.24f, 0.142f}, wear);
    BakeEnd(rt);
    return rt;
}

Projector WalletProjector() {
    return Projector{{L::kHingeX, L::kTop + H * 0.5f}, L::kPerspective};
}

SurfaceFn CoverSurface(float cover, int side) {
    float psi = cover * PI;
    float c = std::cos(psi) * side, s = std::sin(psi);
    return [c, s](float u01, float v01) {
        float u = -P + u01 * (W + 2 * P);
        float v = -P + v01 * (H + 2 * P);
        // lift slightly while swinging so it clears the panel
        return Vec3{L::kHingeX + u * c, L::kTop + v, u * s + 2.0f};
    };
}

Quad PanelQuad(int side) {
    float x0 = PanelX0(side) - P, y0 = L::kTop - P, x1 = PanelX0(side) + W + P, y1 = L::kTop + H + P;
    return Quad{{{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}}};
}
}  // namespace

WalletArt BakeWallet(const Config& cfg, const Font& font) {
    WalletArt art;
    if (cfg.theme == Theme::Dracula) {
        TomeArt t = BakeTome(cfg, font);
        return {t.cover, t.endpaper, t.page};
    }
    art.outer = BakeOuter(cfg, font);
    art.inner = BakeInner(cfg, font);
    art.panel = BakePanel(cfg);
    return art;
}

void UnloadWallet(WalletArt& art) {
    UnloadRenderTexture(art.outer);
    UnloadRenderTexture(art.inner);
    UnloadRenderTexture(art.panel);
}

float PanelX0(int side) { return side > 0 ? L::kHingeX : L::kHingeX - W; }

Vector2 SleeveAnchor(int side) {
    return {PanelX0(side) + W * 0.5f, L::kTop + H - L::kPocketDepth};
}

void DrawWalletShadow(const WalletPose& pose) {
    float breathe = 0.03f * std::sin(pose.time * 1.3f);
    DrawSoftShadow(PanelQuad(pose.side), {4, 9}, 18.0f, 0.34f + breathe);
    float lift = std::sin(std::fmin(std::fmax(pose.cover, 0.0f), 1.0f) * PI);
    if (pose.cover > 0.01f) {
        Quad q = ProjectSurface(WalletProjector(), CoverSurface(pose.cover, pose.side));
        DrawSoftShadow(q, {4 + 18 * lift, 9 + 22 * lift}, 18.0f + 20 * lift, (0.3f + breathe) * (1 - 0.4f * lift));
    }
}

WalletHits DrawWallet(const WalletArt& art, const WalletPose& pose) {
    WalletHits hits;
    hits.panel = PanelQuad(pose.side);

    // leather thickness under the panel
    BeginStraightBlend();
    Rectangle body = {PanelX0(pose.side) + 1, L::kTop + 3, W, H};
    DrawRectangleRounded(body, Roundness(L::kWalletR, body), 8, Color{34, 19, 10, 255});

    BeginPremulBlend();
    Texture2D pt = art.panel.texture;
    DrawTexturePro(pt, {0, 0, (float)pt.width, -(float)pt.height},
                   {PanelX0(pose.side) - P, L::kTop - P, W + 2 * P, H + 2 * P}, {0, 0}, 0.0f, WHITE);

    Projector pj = WalletProjector();
    SurfaceFn f = CoverSurface(pose.cover, pose.side);
    hits.cover = ProjectSurface(pj, f);
    bool outside = QuadArea(hits.cover) * pose.side > 0.0f;
    bool mirror = hits.cover.p[1].x < hits.cover.p[0].x;   // keep artwork readable either way
    Color tint = ShadeTint(SurfaceShade(f));
    DrawSurface(pj, outside ? art.outer.texture : art.inner.texture, f, 12, 4, tint, mirror);
    return hits;
}
