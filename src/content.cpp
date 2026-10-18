#include "content.h"
#include "drive.h"
#include "raylib.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace {
std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string Env(const char* name) {
    const char* v = std::getenv(name);
    return v ? v : "";
}

std::vector<std::pair<std::string, std::string>> ReadIni(const std::string& path) {
    std::vector<std::pair<std::string, std::string>> kv;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        kv.emplace_back(Trim(line.substr(0, eq)), Trim(line.substr(eq + 1)));
    }
    return kv;
}

std::string ReadText(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return Trim(ss.str());
}

std::vector<Item> ScanItems(const std::string& dir) {
    std::vector<Item> items;
    if (!DirectoryExists(dir.c_str())) return items;
    FilePathList files = LoadDirectoryFiles(dir.c_str());
    std::vector<std::string> images;
    for (unsigned i = 0; i < files.count; i++) {
        if (IsFileExtension(files.paths[i], ".png;.jpg;.jpeg;.bmp;.PNG;.JPG;.JPEG"))
            images.push_back(files.paths[i]);
    }
    UnloadDirectoryFiles(files);
    std::sort(images.begin(), images.end());
    for (const auto& img : images) {
        Item it;
        it.imagePath = img;
        std::string txt = std::string(GetDirectoryPath(img.c_str())) + "/" +
                          GetFileNameWithoutExt(img.c_str()) + ".txt";
        if (FileExists(txt.c_str())) it.note = ReadText(txt);
        items.push_back(it);
    }
    return items;
}
}  // namespace

std::string AssetDir() {
    std::string app = GetApplicationDirectory();
    for (const std::string& dir : {app + "assets/", app + "../Resources/assets/"})
        if (DirectoryExists(dir.c_str())) return dir;
    return app + "assets/";
}

std::string ConfigDir() {
#if defined(_WIN32)
    std::string base = Env("APPDATA");
    if (base.empty()) base = ".";
    return base + "/wallet_image";
#elif defined(__APPLE__)
    return Env("HOME") + "/Library/Application Support/wallet_image";
#else
    std::string base = Env("XDG_CONFIG_HOME");
    if (base.empty()) base = Env("HOME") + "/.config";
    return base + "/wallet_image";
#endif
}

// Applies the keys present in `path`; relative font paths resolve against `base`.
void ApplyIni(Config& cfg, const std::string& path, const std::string& base) {
    for (auto& [k, v] : ReadIni(path)) {
        if (k == "theme") cfg.theme = v == "wallet" ? Theme::Wallet : Theme::Dracula;
        else if (k == "aging") cfg.aging = std::clamp((float)std::atof(v.c_str()), 0.0f, 1.0f);
        else if (k == "initials") cfg.initials = v;
        else if (k == "font") cfg.fontPath = v.empty() || v[0] == '/' ? v : base + "/" + v;
        else if (k == "drive") cfg.drive = v;
        else if (k == "x") cfg.x = std::atoi(v.c_str());
        else if (k == "y") cfg.y = std::atoi(v.c_str());
    }
}

Content LoadContent(int argc, char** argv) {
    Content c;
    std::string userDir = ConfigDir() + "/items";
    c.dir = userDir;
    bool explicitDir = false;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--open" && i + 1 < argc) c.startOpen = std::atoi(argv[++i]);
        else if (a == "--flip" && i + 1 < argc) c.startFlip = std::atoi(argv[++i]);
        else {
            c.dir = a;
            explicitDir = true;
        }
    }

    // Local wallet.ini always has the last word, so apply it after any other source.
    std::string localDir = c.dir, localIni = c.dir + "/wallet.ini";
    ApplyIni(c.cfg, localIni, localDir);

    if (!c.cfg.drive.empty()) {
        c.driveId = DriveFolderId(c.cfg.drive);
        if (c.driveId.empty()) TraceLog(LOG_WARNING, "drive= is not a Google Drive folder link");
    }
    if (!c.driveId.empty()) {
        c.dir = DriveCacheDir(c.driveId);
        ApplyIni(c.cfg, c.dir + "/wallet.ini", c.dir);
        ApplyIni(c.cfg, localIni, localDir);
    }

    c.items = ScanItems(c.dir);
    if (c.items.empty() && !explicitDir && c.driveId.empty()) {
        TraceLog(LOG_WARNING, "No photos in %s, showing samples", c.dir.c_str());
        c.dir = AssetDir() + "samples";
        c.items = ScanItems(c.dir);
        ApplyIni(c.cfg, c.dir + "/wallet.ini", c.dir);
        ApplyIni(c.cfg, localIni, localDir);
    }
    if (c.items.empty()) {
        Item hint;
        hint.note = c.driveId.empty()
                        ? "Put photos (.jpg/.png) and notes (.txt with the same name) in:\n" + userDir
                        : "Fetching photos from Google Drive...\nIf nothing appears, check the folder is shared "
                          "as \"Anyone with the link\" and has .jpg/.png files.";
        c.items.push_back(hint);
    }
    return c;
}

bool LoadSavedState(SavedState& st) {
    bool hx = false, hy = false;
    for (auto& [k, v] : ReadIni(ConfigDir() + "/state.ini")) {
        if (k == "x") { st.x = std::atoi(v.c_str()); hx = true; }
        if (k == "y") { st.y = std::atoi(v.c_str()); hy = true; }
        if (k == "dock") st.dock = v == "left" ? -1 : v == "right" ? 1 : 0;
    }
    return hx && hy;
}

void SaveState(const SavedState& st) {
    std::string dir = ConfigDir();
    if (!DirectoryExists(dir.c_str())) MakeDirectory(dir.c_str());
    std::ofstream out(dir + "/state.ini");
    out << "x=" << st.x << "\ny=" << st.y << "\ndock="
        << (st.dock < 0 ? "left" : st.dock > 0 ? "right" : "none") << "\n";
}
