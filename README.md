# Wallet Keepsake

A small desktop widget: a worn leather wallet tucked against the edge of your screen. Click it and
it slides in, opens, and a photo drops out of a plastic accordion insert, the way people carried
pictures of loved ones in wartime. Scroll to unfold more photos; click one to turn it over and read
the handwritten note on the back.

- Fully transparent background, floats above other windows, no title bar.
- Clicks on empty space go straight through to whatever is underneath.
- Leather, paper and the aged-photo look are generated on the fly; your photos are untouched.

Two themes (pick one in `wallet.ini`, see below):

| `theme=dracula` (default): romantic gothic                     | `theme=wallet`                                  |
|---------------------------------------------------------------|-------------------------------------------------|
| Dracula's love-letter book: burgundy leather, gold tooling, rose and bat-wing emblem | Worn leather wartime wallet |
| Opens on Hayez's *The Kiss* in an oval gilt frame; a love letter and pressed rose on the page | Dust drifts out as it opens |
| Photos hang in gilded frames on chains, in flickering candlelight | Photos in a plastic accordion insert |
| Warm rose-sepia candlelit grade; rose petals drift down         | Sepia print with a fold crease                  |
| Notes on parchment pinned to the frame back, red ink, rose wax seal | Notes handwritten on the back of the print |

Written in C++17 with [raylib](https://www.raylib.com/) 5.5.

---

## Install on macOS

Requirements: macOS 11 or newer (Intel or Apple Silicon), an internet connection for the first build.

```sh
cd wallet_image
./scripts/setup-macos.sh
```

The script:

1. Installs the Xcode Command Line Tools if missing (a system dialog appears; run the script again
   when it finishes).
2. Installs CMake via [Homebrew](https://brew.sh) if missing.
3. Builds **Wallet Keepsake.app** and installs it to `~/Applications`.
4. Creates your photo folder and opens it in Finder, then launches the app.

Options:

| Command                                   | Does                                               |
|-------------------------------------------|----------------------------------------------------|
| `./scripts/setup-macos.sh --autostart`    | Also start the wallet automatically when you log in |
| `./scripts/setup-macos.sh --no-open`      | Install without launching                          |
| `./scripts/setup-macos.sh --uninstall`    | Remove the app and login item (keeps your photos)  |

Run the script again any time to rebuild after pulling changes.

The app has no Dock icon or menu bar (it's a widget). Open it from `~/Applications` or Spotlight
("Wallet Keepsake"); quit it from its right-click menu or with **Cmd+Q** while it's focused.

---

## Add your photos

Put photos in this folder (the setup script creates it and opens it for you):

| OS      | Folder                                                  |
|---------|---------------------------------------------------------|
| macOS   | `~/Library/Application Support/wallet_image/items/`     |
| Linux   | `~/.config/wallet_image/items/`                         |
| Windows | `%APPDATA%\wallet_image\items\`                         |

On macOS, `Library` is hidden in Finder: use **Go → Go to Folder…** (Shift+Cmd+G) and paste the path.

- Every `.jpg` / `.png` is one photo, ordered by file name, so number them: `01_…`, `02_…`.
  (iPhone `.heic` photos work from a Google Drive folder, which converts them; in a local folder,
  export them as JPEG first.)
- The note on a photo's back is a `.txt` file with the **same name**:

```
items/
  01_mom.jpg
  01_mom.txt      ← written on the back of 01_mom.jpg
  02_home.png     ← no .txt: blank back
  wallet.ini      ← optional settings (below)
```

Notes are plain UTF-8 text. Line breaks are kept and long notes shrink to fit; accented and
Vietnamese characters work with both themes' fonts. Photos are cropped to a landscape frame (portraits keep the top part).

**Restart the app after changing photos** (they're loaded at start).

If the folder is empty, the wallet shows three sample photos.

### Photos from Google Drive

Instead of a local folder, the wallet can show a Google Drive folder:

1. In Google Drive, put the photos and `.txt` notes in a folder (same naming rules as above;
   a `wallet.ini` there works too). Notes must be uploaded `.txt` files, not Google Docs.
2. Share the folder: **Share → General access → Anyone with the link (Viewer)**, copy the link.
3. Add the link to `wallet.ini` in your local photo folder:
   ```ini
   drive=https://drive.google.com/drive/folders/1AbC...xyz?usp=sharing
   ```

On start the wallet shows its cached copy right away, then syncs in the background and updates
itself if anything changed. Right-click → **Refresh photos** syncs again. Offline, it keeps showing
the last copy (stored in `drive/` inside the settings folder).

Downloads use `curl` (built into macOS, Windows 10+ and most Linux systems). Anyone with the link
can see the photos, so only use this for photos you're happy to share that way. Subfolders are ignored.

### Settings: `wallet.ini`

Optional, in the same folder:

```ini
theme=dracula      # dracula (default) or wallet
initials=E. M.     # pressed into the cover (dracula: replaces the "Dracula" title)
aging=0.9          # 0 = fresh print, 1 = very worn
font=myhand.ttf    # your own font for the notes (.ttf, placed in the same folder)
drive=https://drive.google.com/drive/folders/...   # load photos from a shared Drive folder
```

---

## Using it

The wallet docks to the side of the screen; only a strip of leather peeks out.

| Action                          | Result                                           |
|---------------------------------|--------------------------------------------------|
| Click the peeking strip         | Wallet slides in and opens                       |
| Scroll down (or ↓)              | Next photo drops out of the insert               |
| Scroll up (or ↑)                | Folds the last photo back up                     |
| Click a photo                   | Turns it over to read the note                   |
| Click the open wallet           | Photos fold back, it closes and slides away      |
| Drag up/down while docked       | Moves it along the edge                          |
| Drag away from the edge         | Undocks it so it floats anywhere                 |
| Drop it near a screen edge      | Docks to that edge (left or right)               |
| Right-click                     | Menu: keep on top, open/close, refresh photos, quit |
| Esc / Space                     | Close / toggle                                   |
| Cmd+Q (macOS), Ctrl+Q           | Quit                                             |

Its position and dock edge are remembered between runs. With many photos, the wallet unfolds as
far as your screen height allows.

---

## Build on Linux / Windows

Needs CMake 3.20+, a C++17 compiler and git. CMake downloads raylib automatically.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/wallet_image                 # Windows: build\Release\wallet_image.exe
```

- **Linux** needs the X11/GL dev packages:
  `sudo apt install build-essential cmake git libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev`.
  Transparency needs a compositor (default on GNOME/KDE). On Wayland it runs through XWayland.
- Keep the `assets/` folder next to the executable when moving it.

Command-line options (all platforms): a folder path to load photos from instead of the default,
`--open N` to start with N photos hanging, `--flip I` to start with photo I turned over.

---

## Troubleshooting

- **macOS: "Wallet Keepsake can't be opened"** — this happens if you received a pre-built app
  instead of building it. Build it with the setup script, or run
  `xattr -dr com.apple.quarantine ~/Applications/Wallet\ Keepsake.app`.
- **Can't find the wallet** — it's tucked against the right edge of the screen it was last on.
  Delete `state.ini` in the settings folder (one level above `items/`) to reset its position.
- **Scrolling goes the wrong way** — the up/down arrow keys work too. With macOS natural scrolling,
  swipe up to drop the next photo.
- **Photos don't change** — right-click → Refresh photos (or restart the app).
- **Drive photos don't appear** — check the folder is shared as "Anyone with the link", the files are
  .jpg/.png (not Google Photos albums) and notes are .txt files. Run the app from a terminal to see
  the sync message.

---

## Project layout

```
CMakeLists.txt          build (fetches raylib 5.5; makes a .app on macOS)
cmake/Info.plist.in     macOS bundle info (no Dock icon)
scripts/setup-macos.sh  macOS build + install
src/                    C++ sources (window, animation, wallet, photos, notes)
src/gothic.cpp          Dracula theme: grimoire, gilded frames, candles, bats
assets/shaders/         leather, paper, aged-photo, gloss and gilt shaders
assets/fonts/           Patrick Hand, Grenze Gotisch, Cormorant Garamond (all SIL OFL)
assets/gothic/          The Kiss (Hayez, 1859, public domain) for the dracula theme
assets/samples/         sample photos and notes
```

Credits: sample photos from Unsplash via picsum.photos. *Il bacio* (The Kiss) by Francesco Hayez,
1859, public domain (Wikimedia Commons). Fonts under the SIL Open Font License:
Patrick Hand (Patrick Wagesreiter), Grenze Gotisch (Omnibus-Type), Cormorant Garamond
(Christian Thalmann); licence texts in `assets/fonts/`.
