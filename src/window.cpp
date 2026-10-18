#include "window.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace {
bool gPassthrough = false;
}

void OpenWidgetWindow(int width, int height, const char* title) {
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST |
                   FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(width, height, title);
    SetExitKey(KEY_NULL);
}

Vector2 CursorLocal() {
    double x = 0, y = 0;
    if (GLFWwindow* w = glfwGetCurrentContext()) glfwGetCursorPos(w, &x, &y);
    return {(float)x, (float)y};
}

void SetPassthrough(bool on) {
    if (on == gPassthrough) return;
    gPassthrough = on;
    if (on) SetWindowState(FLAG_WINDOW_MOUSE_PASSTHROUGH);
    else ClearWindowState(FLAG_WINDOW_MOUSE_PASSTHROUGH);
}

void SetTopmost(bool on) {
    if (on) SetWindowState(FLAG_WINDOW_TOPMOST);
    else ClearWindowState(FLAG_WINDOW_TOPMOST);
}

bool IsTopmost() { return IsWindowState(FLAG_WINDOW_TOPMOST); }
