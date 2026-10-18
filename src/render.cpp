#include "render.h"
#include "content.h"
#include "layout.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
Texture2D gWhite;
Shader gShaders[5];

std::vector<Vector2> RoundRectPath(Rectangle r, float rad, int seg) {
    std::vector<Vector2> pts;
    rad = std::min(rad, std::min(r.width, r.height) * 0.5f);
    struct C { float cx, cy, a0; } corners[4] = {
        {r.x + r.width - rad, r.y + rad, -90.0f},
        {r.x + r.width - rad, r.y + r.height - rad, 0.0f},
        {r.x + rad, r.y + r.height - rad, 90.0f},
        {r.x + rad, r.y + rad, 180.0f},
    };
    for (auto& c : corners) {
        for (int i = 0; i <= seg; i++) {
            float a = (c.a0 + 90.0f * i / seg) * DEG2RAD;
            pts.push_back({c.cx + std::cos(a) * rad, c.cy + std::sin(a) * rad});
        }
    }
    pts.push_back(pts.front());
    return pts;
}

void DrawDashedPath(const std::vector<Vector2>& pts, float dash, float gap, float thick,
                    Color thread) {
    Color hole = {20, 10, 5, (unsigned char)(thread.a * 0.55f)};
    float period = dash + gap;
    float phase = dash + gap * 0.5f;   // position inside the dash/gap cycle; start mid-gap
    for (size_t i = 0; i + 1 < pts.size(); i++) {
        Vector2 a = pts[i], b = pts[i + 1];
        float len = std::hypot(b.x - a.x, b.y - a.y);
        if (len < 1e-4f) continue;
        Vector2 d = {(b.x - a.x) / len, (b.y - a.y) / len};
        float s = 0.0f;
        while (s < len) {
            bool inDash = phase < dash;
            float run = std::min((inDash ? dash : period) - phase, len - s);
            if (inDash) {
                Vector2 p0 = {a.x + d.x * s, a.y + d.y * s};
                Vector2 p1 = {a.x + d.x * (s + run), a.y + d.y * (s + run)};
                DrawLineEx({p0.x + 0.35f, p0.y + 0.5f}, {p1.x + 0.35f, p1.y + 0.5f}, thick, hole);
                DrawLineEx(p0, p1, thick, thread);
            }
            s += run;
            phase += run;
            if (phase >= period - 1e-4f) phase = 0.0f;
        }
    }
}
}  // namespace

Vector2 Projector::operator()(Vec3 v) const {
    float s = dist / std::max(dist - v.z, 50.0f);
    return {center.x + (v.x - center.x) * s, center.y + (v.y - center.y) * s};
}

void BeginStraightBlend() {
    rlDrawRenderBatchActive();
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA,
                              RL_FUNC_ADD, RL_FUNC_ADD);
    rlSetBlendMode(RL_BLEND_CUSTOM_SEPARATE);
}

void BeginPremulBlend() {
    rlDrawRenderBatchActive();
    rlSetBlendFactorsSeparate(RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA,
                              RL_FUNC_ADD, RL_FUNC_ADD);
    rlSetBlendMode(RL_BLEND_CUSTOM_SEPARATE);
}

void BeginAdditiveBlend() {
    rlDrawRenderBatchActive();
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE, RL_ONE, RL_ONE, RL_FUNC_ADD, RL_FUNC_ADD);
    rlSetBlendMode(RL_BLEND_CUSTOM_SEPARATE);
}

void InitRender() {
    Image img = GenImageColor(1, 1, WHITE);
    gWhite = LoadTextureFromImage(img);
    UnloadImage(img);

    std::string dir = AssetDir() + "shaders/";
    const char* names[5] = {"leather.fs", "paper.fs", "photo.fs", "gloss.fs", "gilt.fs"};
    for (int i = 0; i < 5; i++) {
        gShaders[i] = LoadShader(nullptr, (dir + names[i]).c_str());
    }
    rlDisableBackfaceCulling();
}

void ShutdownRender() {
    UnloadTexture(gWhite);
    for (auto& s : gShaders) UnloadShader(s);
}

Shader GetShader(ShaderId id) { return gShaders[(int)id]; }

void SetUniform(Shader sh, const char* name, float v) {
    SetShaderValue(sh, GetShaderLocation(sh, name), &v, SHADER_UNIFORM_FLOAT);
}
void SetUniform(Shader sh, const char* name, Vector2 v) {
    SetShaderValue(sh, GetShaderLocation(sh, name), &v, SHADER_UNIFORM_VEC2);
}
void SetUniform(Shader sh, const char* name, Vector3 v) {
    SetShaderValue(sh, GetShaderLocation(sh, name), &v, SHADER_UNIFORM_VEC3);
}
void SetUniform(Shader sh, const char* name, Vector4 v) {
    SetShaderValue(sh, GetShaderLocation(sh, name), &v, SHADER_UNIFORM_VEC4);
}

RenderTexture2D BakeBegin(float w, float h) {
    RenderTexture2D rt = LoadRenderTexture((int)std::ceil(w * L::kBakeScale),
                                           (int)std::ceil(h * L::kBakeScale));
    BeginTextureMode(rt);
    ClearBackground(BLANK);
    BeginStraightBlend();
    rlPushMatrix();
    rlScalef(L::kBakeScale, L::kBakeScale, 1.0f);
    return rt;
}

void BakeEnd(RenderTexture2D& rt) {
    rlDrawRenderBatchActive();
    rlPopMatrix();
    EndTextureMode();
    GenTextureMipmaps(&rt.texture);
    SetTextureFilter(rt.texture, TEXTURE_FILTER_TRILINEAR);
}

void FillShaderRect(Shader sh, Rectangle r) {
    SetUniform(sh, "size", Vector2{r.width * L::kBakeScale, r.height * L::kBakeScale});
    BeginShaderMode(sh);
    DrawTexturePro(gWhite, {0, 0, 1, 1}, r, {0, 0}, 0.0f, WHITE);
    EndShaderMode();
}

void FillLeather(Rectangle r, float seed, Vector3 base, float radius, float wear) {
    Shader sh = GetShader(ShaderId::Leather);
    SetUniform(sh, "seed", seed);
    SetUniform(sh, "base", base);
    SetUniform(sh, "radius", radius * L::kBakeScale);
    SetUniform(sh, "wear", wear);
    FillShaderRect(sh, r);
}

void FillPaper(Rectangle r, float seed, Vector3 base, float radius, float wear, float rough) {
    Shader sh = GetShader(ShaderId::Paper);
    SetUniform(sh, "seed", seed);
    SetUniform(sh, "base", base);
    SetUniform(sh, "radius", radius * L::kBakeScale);
    SetUniform(sh, "wear", wear);
    SetUniform(sh, "rough", rough * L::kBakeScale);
    FillShaderRect(sh, r);
}

void FillGilt(Rectangle r, float border, float seed) {
    Shader sh = GetShader(ShaderId::Gilt);
    SetUniform(sh, "border", border * L::kBakeScale);
    SetUniform(sh, "seed", seed);
    FillShaderRect(sh, r);
}

void DrawAgedPhoto(Texture2D img, Rectangle dst, float seed, float aging, float style) {
    float iw = (float)img.width, ih = (float)img.height;
    float want = dst.width / dst.height;
    Rectangle src = {0, 0, iw, ih};
    if (iw / ih > want) {
        src.width = ih * want;
        src.x = (iw - src.width) * 0.5f;
    } else {
        src.height = iw / want;
        src.y = (ih - src.height) * 0.3f;   // keep faces in portraits
    }
    Shader sh = GetShader(ShaderId::Photo);
    SetUniform(sh, "srcRect", Vector4{src.x / iw, src.y / ih, src.width / iw, src.height / ih});
    SetUniform(sh, "size", Vector2{dst.width * L::kBakeScale, dst.height * L::kBakeScale});
    SetUniform(sh, "seed", seed);
    SetUniform(sh, "aging", aging);
    SetUniform(sh, "style", style);
    BeginShaderMode(sh);
    DrawTexturePro(img, src, dst, {0, 0}, 0.0f, WHITE);
    EndShaderMode();
}

Quad ProjectSurface(const Projector& pj, const SurfaceFn& f) {
    return Quad{{pj(f(0, 0)), pj(f(1, 0)), pj(f(1, 1)), pj(f(0, 1))}};
}

void DrawSurface(const Projector& pj, Texture2D tex, const SurfaceFn& f, int nu, int nv,
                 Color tint, bool mirrorS) {
    std::vector<Vector2> pts((nu + 1) * (nv + 1));
    for (int j = 0; j <= nv; j++)
        for (int i = 0; i <= nu; i++)
            pts[j * (nu + 1) + i] = pj(f((float)i / nu, (float)j / nv));

    rlCheckRenderBatchLimit(nu * nv * 4);
    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
    rlNormal3f(0, 0, 1);
    auto vert = [&](int i, int j) {
        float s = (float)i / nu, t = (float)j / nv;
        rlColor4ub(tint.r, tint.g, tint.b, tint.a);
        rlTexCoord2f(mirrorS ? 1.0f - s : s, 1.0f - t);   // render textures are stored flipped
        Vector2 p = pts[j * (nu + 1) + i];
        rlVertex2f(p.x, p.y);
    };
    for (int j = 0; j < nv; j++) {
        for (int i = 0; i < nu; i++) {
            vert(i, j);
            vert(i, j + 1);
            vert(i + 1, j + 1);
            vert(i + 1, j);
        }
    }
    rlEnd();
    rlSetTexture(0);
}

float QuadArea(const Quad& q) {
    float a = 0;
    for (int i = 0; i < 4; i++) {
        const Vector2& p = q.p[i];
        const Vector2& n = q.p[(i + 1) % 4];
        a += p.x * n.y - n.x * p.y;
    }
    return a * 0.5f;
}

bool PointInQuad(Vector2 pt, const Quad& q) {
    bool pos = false, neg = false;
    for (int i = 0; i < 4; i++) {
        Vector2 a = q.p[i], b = q.p[(i + 1) % 4];
        float c = (b.x - a.x) * (pt.y - a.y) - (b.y - a.y) * (pt.x - a.x);
        if (c > 0) pos = true;
        if (c < 0) neg = true;
    }
    return !(pos && neg);
}

float SurfaceShade(const SurfaceFn& f) {
    Vec3 o = f(0, 0), a = f(1, 0) - o, b = f(0, 1) - o;
    Vec3 n = {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len < 1e-5f) return 0.6f;
    n = n * (1.0f / len);
    if (n.z < 0) n = n * -1.0f;   // shade whichever face we see
    Vec3 l = {-0.28f, -0.42f, 0.86f};
    float ll = std::sqrt(l.x * l.x + l.y * l.y + l.z * l.z);
    float d = (n.x * l.x + n.y * l.y + n.z * l.z) / ll;
    float rest = 0.62f + 0.38f * 0.86f / ll;
    return std::clamp((0.62f + 0.38f * std::max(d, 0.0f)) / rest, 0.0f, 1.08f);
}

Color ShadeTint(float b, float alpha) {
    auto c = [](float v) { return (unsigned char)std::clamp(v * 255.0f, 0.0f, 255.0f); };
    return Color{c(b * alpha), c(b * alpha), c(b * alpha), c(alpha)};
}

void DrawSoftShadow(const Quad& q, Vector2 offset, float spread, float alpha) {
    Vector2 c = {0, 0};
    for (auto& p : q.p) c = {c.x + p.x * 0.25f, c.y + p.y * 0.25f};
    const int layers = 10;
    rlCheckRenderBatchLimit(layers * 4);
    rlSetTexture(gWhite.id);
    rlBegin(RL_QUADS);
    for (int k = 0; k < layers; k++) {
        float grow = spread * (k + 1) / layers - spread * 0.35f;
        unsigned char a = (unsigned char)std::clamp(alpha / layers * 255.0f * 1.6f, 0.0f, 255.0f);
        for (int i : {0, 3, 2, 1}) {
            Vector2 p = q.p[i];
            float dx = p.x - c.x, dy = p.y - c.y;
            float len = std::max(std::hypot(dx, dy), 1e-3f);
            rlColor4ub(0, 0, 0, a);
            rlTexCoord2f(0.5f, 0.5f);
            rlVertex2f(p.x + dx / len * grow + offset.x, p.y + dy / len * grow + offset.y);
        }
    }
    rlEnd();
    rlSetTexture(0);
}

void DrawStitchedRoundRect(Rectangle r, float radius, float dash, float gap, float thick,
                           Color thread) {
    DrawDashedPath(RoundRectPath(r, radius, 8), dash, gap, thick, thread);
}

void DrawStitchedLine(Vector2 a, Vector2 b, float dash, float gap, float thick, Color thread) {
    DrawDashedPath({a, b}, dash, gap, thick, thread);
}
