#pragma once
#include "raylib.h"

// Transparent, undecorated, always-on-top widget window.
void OpenWidgetWindow(int width, int height, const char* title);
// Cursor relative to the window, valid even while mouse passthrough is on.
Vector2 CursorLocal();
// Let clicks fall through to whatever is underneath (only toggles on change).
void SetPassthrough(bool on);
void SetTopmost(bool on);
bool IsTopmost();
