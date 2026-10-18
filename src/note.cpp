#include "note.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
struct Rng {
    unsigned s;
    float Next() {   // [0,1)
        s = s * 1664525u + 1013904223u;
        return (s >> 8) / 16777216.0f;
    }
    float Range(float lo, float hi) { return lo + (hi - lo) * Next(); }
};

std::vector<std::string> Wrap(const std::string& text, const Font& font, float size, float width) {
    std::vector<std::string> lines;
    size_t start = 0;
    while (start <= text.size()) {
        size_t nl = text.find('\n', start);
        std::string para = text.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
        std::string line;
        size_t i = 0;
        while (i <= para.size()) {
            size_t sp = para.find(' ', i);
            std::string word = para.substr(i, sp == std::string::npos ? std::string::npos : sp - i);
            std::string trial = line.empty() ? word : line + " " + word;
            if (!line.empty() && MeasureTextEx(font, trial.c_str(), size, 0).x > width) {
                lines.push_back(line);
                line = word;
            } else {
                line = trial;
            }
            if (sp == std::string::npos) break;
            i = sp + 1;
        }
        lines.push_back(line);
        if (nl == std::string::npos) break;
        start = nl + 1;
    }
    return lines;
}
}  // namespace

void DrawNote(Rectangle area, const std::string& text, const Font& font, int seed) {
    if (text.empty()) return;
    float size = 19.0f;
    std::vector<std::string> lines;
    for (; size >= 9.0f; size -= 1.0f) {
        lines = Wrap(text, font, size, area.width - 6);
        if (lines.size() * size * 1.08f <= area.height) break;
    }
    float lineH = size * 1.08f;
    int maxLines = std::max(1, (int)(area.height / lineH));
    if ((int)lines.size() > maxLines) lines.resize(maxLines);

    Rng rng{(unsigned)seed * 2654435761u + 7u};
    float slope = rng.Range(-1.2f, 0.6f);
    for (size_t i = 0; i < lines.size(); i++) {
        Vector2 pos = {area.x + rng.Range(0.0f, 4.0f), area.y + i * lineH + rng.Range(-0.8f, 0.8f)};
        float rot = slope + rng.Range(-0.9f, 0.9f);
        unsigned char a = (unsigned char)rng.Range(185, 225);
        // faint bleed, then the stroke
        DrawTextPro(font, lines[i].c_str(), {pos.x + 0.35f, pos.y + 0.3f}, {0, 0}, rot, size, 0,
                    Color{40, 46, 96, (unsigned char)(a / 4)});
        DrawTextPro(font, lines[i].c_str(), pos, {0, 0}, rot, size, 0, Color{34, 40, 88, a});
    }
}
