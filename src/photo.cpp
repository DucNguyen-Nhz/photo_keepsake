#include "photo.h"
#include "gothic.h"
#include "layout.h"
#include "note.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float P = L::kPad, W = L::kSleeveW, TAB = L::kSleeveTab, H = L::kSleeveH;
constexpr float kTexW = W + 2 * P, kTexH = TAB + H + 2 * P;

float Roundness(float radius, Rectangle r) {
    return std::fmin(1.0f, 2.0f * radius / std::fmin(r.width, r.height));
}

Rectangle SleeveBody() { return {P, P, W, TAB + H}; }
Rectangle CardRect() { return {P + 9, P + TAB + 9, W - 18, H - 18}; }

void DrawPlastic() {
    Rectangle body = SleeveBody();
    DrawRectangleRounded(body, Roundness(6, body), 8, Color{212, 222, 228, 60});
    DrawRectangleRoundedLinesEx(body, Roundness(6, body), 8, 1.0f, Color{255, 255, 255, 130});
    DrawLineEx({body.x + 2, body.y + TAB}, {body.x + body.width - 2, body.y + TAB}, 1.0f,
               Color{255, 255, 255, 100});
    Rectangle seal = {body.x + 3, body.y + TAB + 3, body.width - 6, body.height - TAB - 6};
    DrawRectangleRoundedLinesEx(seal, Roundness(4, seal), 8, 0.8f, Color{255, 255, 255, 50});
}

void PlasticOverlay() {
    Rectangle body = SleeveBody();
    DrawRectangleRounded(body, Roundness(6, body), 8, Color{235, 242, 248, 16});
}

void FillCard(Rectangle r, float seed, Vector3 base, float wear) {
    FillPaper(r, seed, base, 2.0f, wear, 1.2f);
}

// The print was folded once to fit the wallet.
void FoldCrease(Rectangle card, float aging) {
    float x = card.x + card.width * 0.5f;
    DrawLineEx({x, card.y + 1}, {x, card.y + card.height - 1}, 0.9f,
               Color{255, 250, 235, (unsigned char)(90 * aging)});
    DrawLineEx({x + 0.9f, card.y + 1}, {x + 0.9f, card.y + card.height - 1}, 0.8f,
               Color{60, 40, 20, (unsigned char)(45 * aging)});
}

}  // namespace

SleeveArt BakeSleeve(const Item& item, const Font& font, float aging, int seed, Theme theme) {
    if (theme == Theme::Dracula) return BakeFrame(item, font, aging, seed);
    SleeveArt art;
    float fs = 1.7f + seed * 3.13f;
    Rectangle card = CardRect();

    art.front = BakeBegin(kTexW, kTexH);
    DrawPlastic();
    FillCard(card, fs, {0.94f, 0.91f, 0.83f}, 0.8f * aging);
    Rectangle photo = {card.x + 7, card.y + 7, card.width - 14, card.height - 14};
    Texture2D img = {};
    if (!item.imagePath.empty()) img = LoadTexture(item.imagePath.c_str());
    if (img.id != 0) {
        GenTextureMipmaps(&img);
        SetTextureFilter(img, TEXTURE_FILTER_TRILINEAR);
        DrawAgedPhoto(img, photo, fs, aging, 0.0f);
        DrawRectangleLinesEx(photo, 0.5f, Color{60, 40, 20, 70});
        UnloadTexture(img);
    }
    FoldCrease(card, aging);
    PlasticOverlay();
    BakeEnd(art.front);

    art.back = BakeBegin(kTexW, kTexH);
    DrawPlastic();
    FillCard(card, fs + 0.5f, {0.89f, 0.84f, 0.72f}, aging);
    Font stamp = GetFontDefault();
    DrawTextEx(stamp, "VELOX  PHOTO  POST  CARD", {card.x + 30, card.y + card.height - 11}, 10, 2,
               Color{120, 90, 60, 30});
    FoldCrease(card, aging);
    DrawNote({card.x + 12, card.y + 18, card.width - 24, card.height - 30}, item.note, font, seed);
    PlasticOverlay();
    BakeEnd(art.back);
    return art;
}

void UnloadSleeve(SleeveArt& art) {
    UnloadRenderTexture(art.front);
    UnloadRenderTexture(art.back);
}

RenderTexture2D BakeGloss() {
    RenderTexture2D rt = BakeBegin(kTexW, kTexH);
    Shader sh = GetShader(ShaderId::Gloss);
    SetUniform(sh, "radius", 6.0f * L::kBakeScale);
    FillShaderRect(sh, SleeveBody());
    BakeEnd(rt);
    return rt;
}

std::vector<SleeveGeom> LayoutSleeves(const std::vector<SleevePose>& poses, Vector2 anchor) {
    std::vector<SleeveGeom> out;
    float theta = 0.0f;
    Vec3 hinge = {anchor.x, anchor.y, 0.0f};
    for (size_t i = 0; i < poses.size(); i++) {
        const SleevePose& sp = poses[i];
        if (sp.drop < 0.004f) break;
        theta += (1.0f - sp.drop) * PI + sp.wobble;
        Vec3 dir = {0.0f, std::cos(theta), -std::sin(theta)};
        Vec3 nrm = {0.0f, std::sin(theta), std::cos(theta)};
        float phi = sp.flip * PI;
        float cphi = std::cos(phi), sphi = std::sin(phi);
        Vec3 h = hinge;
        SleeveGeom g;
        g.f = [=](float s, float t) {
            float u = (s - 0.5f) * kTexW;
            float v = -P + t * kTexH;
            // flip about the sleeve's vertical centre line, then hang from the hinge
            return h + dir * v + Vec3{u * cphi, 0.0f, 0.0f} + nrm * (u * sphi);
        };
        g.pj = Projector{{anchor.x, anchor.y + i * L::kSleeveStep + L::kSleeveStep * 0.5f},
                         L::kPerspective};
        g.quad = ProjectSurface(g.pj, g.f);
        g.front = QuadArea(g.quad) > 0.0f;
        g.shade = SurfaceShade(g.f);
        out.push_back(g);
        hinge = hinge + dir * L::kSleeveStep;
    }
    return out;
}

void DrawSleeveShadow(const SleeveGeom& g, float alpha) {
    DrawSoftShadow(g.quad, {5, 10}, 14.0f, alpha);
}

void DrawSleeve(const SleeveGeom& g, const SleeveArt& art, Texture2D gloss, Color light) {
    Texture2D tex = g.front ? art.front.texture : art.back.texture;
    DrawSurface(g.pj, tex, g.f, 10, 10, light, !g.front);
    if (gloss.id == 0) return;
    float ga = std::clamp(0.45f + (1.0f - g.shade) * 2.2f, 0.45f, 0.95f);
    DrawSurface(g.pj, gloss, g.f, 10, 10, ShadeTint(1.0f, ga), !g.front);
}
