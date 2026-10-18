#pragma once
#include <string>
#include <vector>

struct Item {
    std::string imagePath;   // empty: blank card
    std::string note;        // text written on the back
};

enum class Theme { Wallet, Dracula };

struct Config {
    Theme theme = Theme::Dracula;
    float aging = 1.0f;      // 0 = fresh print, 1 = very worn
    std::string initials;    // embossed on the wallet cover
    std::string fontPath;    // handwriting font override
    int x = -1, y = -1;      // window position override
};

struct Content {
    std::string dir;
    int startOpen = 0;       // --open N: start with N sleeves hanging
    int startFlip = -1;      // --flip I: start with sleeve I showing its note
    std::vector<Item> items;
    Config cfg;
};

// Folder from the first non-flag argument, else the per-user items folder, else bundled samples.
Content LoadContent(int argc, char** argv);
std::string ConfigDir();
// Bundled assets: next to the executable, or Contents/Resources inside a macOS .app.
std::string AssetDir();
// Window placement remembered between runs.
struct SavedState {
    int x = 0, y = 0;
    int dock = 1;   // 0 floating, -1 docked to the left edge, +1 to the right edge
};
bool LoadSavedState(SavedState& st);
void SaveState(const SavedState& st);
