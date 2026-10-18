#include "drive.h"
#include "content.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace {
#if defined(_WIN32)
const char* kNull = "NUL";
#else
const char* kNull = "/dev/null";
#endif

bool IsIdChar(char c) { return std::isalnum((unsigned char)c) || c == '-' || c == '_'; }

bool ValidId(const std::string& id) {
    return id.size() >= 10 && std::all_of(id.begin(), id.end(), IsIdChar);
}

// Ids and paths are validated/quoted before reaching the shell; file names from Drive never do.
bool Curl(const std::string& url, const fs::path& out) {
    std::string cmd = "curl -sSfL --connect-timeout 10 --max-time 180 -o \"" + out.string() + "\" \"" + url +
                      "\" 2>" + kNull;
    return std::system(cmd.c_str()) == 0;
}

std::string ReadAll(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string DecodeEntities(std::string s) {
    const std::pair<const char*, const char*> ents[] = {
        {"&amp;", "&"}, {"&quot;", "\""}, {"&#39;", "'"}, {"&lt;", "<"}, {"&gt;", ">"}};
    for (auto& [from, to] : ents) {
        for (size_t p = s.find(from); p != std::string::npos; p = s.find(from, p + 1))
            s.replace(p, std::string(from).size(), to);
    }
    return s;
}

// Keep the name readable (including non-ASCII) but safe as a file name on every OS.
std::string SafeName(std::string s) {
    for (char& c : s)
        if (std::string("/\\:*?\"<>|").find(c) != std::string::npos || (unsigned char)c < 32) c = '_';
    while (!s.empty() && (s.front() == '.' || s.front() == ' ')) s.erase(s.begin());
    return s;
}

std::string Lower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

bool Wanted(const std::string& name, bool& isImage) {
    std::string ext = Lower(fs::path(name).extension().string());
    isImage = ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp";
    return isImage || ext == ".txt" || Lower(name) == "wallet.ini";
}

struct Entry {
    std::string id, name;
};

// Parses Drive's simple embedded folder view: <div class="flip-entry" id="entry-ID"> ...
// <div class="flip-entry-title">NAME</div>
std::vector<Entry> ParseListing(const std::string& html) {
    std::vector<Entry> out;
    const std::string idTag = "id=\"entry-", titleTag = "flip-entry-title\">";
    size_t pos = 0;
    while ((pos = html.find(idTag, pos)) != std::string::npos) {
        pos += idTag.size();
        size_t end = html.find('"', pos);
        if (end == std::string::npos) break;
        std::string id = html.substr(pos, end - pos);
        size_t t = html.find(titleTag, end);
        size_t next = html.find(idTag, end);
        if (t == std::string::npos || (next != std::string::npos && t > next)) continue;
        t += titleTag.size();
        size_t te = html.find("</div>", t);
        if (te == std::string::npos) break;
        if (ValidId(id)) out.push_back({id, SafeName(DecodeEntities(html.substr(t, te - t)))});
        pos = te;
    }
    return out;
}

std::map<std::string, std::string> ReadManifest(const fs::path& p) {   // name -> id
    std::map<std::string, std::string> m;
    std::ifstream in(p);
    std::string line;
    while (std::getline(in, line)) {
        size_t tab = line.find('\t');
        if (tab != std::string::npos) m[line.substr(tab + 1)] = line.substr(0, tab);
    }
    return m;
}
}  // namespace

std::string DriveFolderId(const std::string& link) {
    std::string s = link;
    for (const std::string key : {"/folders/", "id="}) {
        size_t p = s.find(key);
        if (p != std::string::npos) {
            s = s.substr(p + key.size());
            break;
        }
    }
    size_t n = 0;
    while (n < s.size() && IsIdChar(s[n])) n++;
    s = s.substr(0, n);
    return ValidId(s) ? s : "";
}

std::string DriveCacheDir(const std::string& folderId) {
    return ConfigDir() + "/drive/" + folderId;
}

SyncResult SyncDriveFolder(const std::string& folderId, const std::string& cacheDir) {
    SyncResult r;
    if (!ValidId(folderId)) {
        r.error = "invalid Drive folder link";
        return r;
    }
    std::error_code ec;
    fs::path dir = fs::u8path(cacheDir);
    fs::create_directories(dir, ec);

    fs::path listing = dir / ".listing.html";
    if (!Curl("https://drive.google.com/embeddedfolderview?id=" + folderId, listing)) {
        r.error = "could not reach Google Drive (offline?)";
        return r;
    }
    std::string html = ReadAll(listing);
    fs::remove(listing, ec);
    if (html.find("flip-entries") == std::string::npos) {
        r.error = "folder is not public; share it as \"Anyone with the link\"";
        return r;
    }
    r.ok = true;

    fs::path manifestPath = dir / ".manifest";
    auto old = ReadManifest(manifestPath);
    std::map<std::string, std::string> now;
    for (const Entry& e : ParseListing(html)) {
        bool isImage = false;
        if (e.name.empty() || !Wanted(e.name, isImage) || now.count(e.name)) continue;
        now[e.name] = e.id;
        fs::path dest = dir / fs::u8path(e.name);
        auto it = old.find(e.name);
        if (isImage && it != old.end() && it->second == e.id && fs::exists(dest)) continue;   // cached

        fs::path part = dir / (e.id + ".part");
        if (!Curl("https://drive.usercontent.google.com/download?id=" + e.id + "&export=download&confirm=t", part)) {
            fs::remove(part, ec);
            if (it != old.end()) now[e.name] = it->second;   // keep the old copy
            continue;
        }
        if (!isImage && fs::exists(dest) && ReadAll(dest) == ReadAll(part)) {   // note unchanged
            fs::remove(part, ec);
            continue;
        }
        fs::rename(part, dest, ec);
        if (!ec) r.changed = true;
    }

    for (const auto& [name, id] : old) {   // deleted from the folder
        if (!now.count(name)) {
            fs::remove(dir / fs::u8path(name), ec);
            r.changed = true;
        }
    }
    std::ofstream out(manifestPath);
    for (const auto& [name, id] : now) out << id << '\t' << name << '\n';
    return r;
}
