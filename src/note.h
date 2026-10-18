#pragma once
#include "raylib.h"
#include <string>

// Handwritten note: wraps and shrinks `text` to fit `area`, with a slightly unsteady hand.
void DrawNote(Rectangle area, const std::string& text, const Font& font, int seed,
              Color ink = Color{34, 40, 88, 215});
