#pragma once
#include "raylib.h"
#include <functional>
#include <vector>

struct Vec3 {
    float x, y, z;
};
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }

struct Quad {
    Vector2 p[4];   // projected corners: (s,t) = (0,0) (1,0) (1,1) (0,1)
};

// Simple pinhole projection; z points toward the viewer.
struct Projector {
    Vector2 center;
    float dist;
    Vector2 operator()(Vec3 v) const;
};

using SurfaceFn = std::function<Vec3(float s, float t)>;

// The window framebuffer is kept premultiplied so the compositor blends it correctly.
void BeginStraightBlend();   // for ordinary (straight-alpha) draws: shapes, text, image textures
void BeginPremulBlend();     // for baked render textures, which are premultiplied

void InitRender();
void ShutdownRender();

enum class ShaderId { Leather, Paper, Photo, Gloss };
Shader GetShader(ShaderId id);
void SetUniform(Shader sh, const char* name, float v);
void SetUniform(Shader sh, const char* name, Vector2 v);
void SetUniform(Shader sh, const char* name, Vector3 v);
void SetUniform(Shader sh, const char* name, Vector4 v);

// Offscreen art. Drawing between Begin/End uses 1x coordinates; pixels are kBakeScale.
RenderTexture2D BakeBegin(float w, float h);
void BakeEnd(RenderTexture2D& rt);
// Fill `r` with a procedural shader; `size` uniform is set to the pixel size.
void FillShaderRect(Shader sh, Rectangle r);

Quad ProjectSurface(const Projector& pj, const SurfaceFn& f);
// Draw a premultiplied texture mapped onto a 3D surface, subdivided for perspective.
void DrawSurface(const Projector& pj, Texture2D tex, const SurfaceFn& f, int nu, int nv,
                 Color tint, bool mirrorS);
float QuadArea(const Quad& q);   // > 0 when the front face points at the viewer
bool PointInQuad(Vector2 pt, const Quad& q);
// Lambert-ish brightness of a surface (1.0 when facing the viewer flat).
float SurfaceShade(const SurfaceFn& f);
Color ShadeTint(float b, float alpha = 1.0f);   // premultiplied tint

void DrawSoftShadow(const Quad& q, Vector2 offset, float spread, float alpha);
void DrawStitchedRoundRect(Rectangle r, float radius, float dash, float gap, float thick,
                           Color thread);
void DrawStitchedLine(Vector2 a, Vector2 b, float dash, float gap, float thick, Color thread);
