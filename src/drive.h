#pragma once
// Photos from a public Google Drive folder ("Anyone with the link"), mirrored into a
// local cache. Uses the `curl` command-line tool; no Google account or API key needed.
#include <string>

// Folder id from a share link (".../folders/<id>?usp=sharing") or a bare id; "" if invalid.
std::string DriveFolderId(const std::string& linkOrId);
std::string DriveCacheDir(const std::string& folderId);

struct SyncResult {
    bool ok = false;        // listing fetched
    bool changed = false;   // files added, updated or removed
    std::string error;
};

// Downloads new/changed photos, .txt notes and wallet.ini into `cacheDir` and removes
// files deleted from the folder. Blocking; safe to run on a worker thread.
SyncResult SyncDriveFolder(const std::string& folderId, const std::string& cacheDir);
