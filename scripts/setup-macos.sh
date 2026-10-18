#!/usr/bin/env bash
# Build and install Wallet Keepsake on macOS.
#
#   ./scripts/setup-macos.sh               build, install to ~/Applications, launch
#   ./scripts/setup-macos.sh --autostart   also start it automatically at login
#   ./scripts/setup-macos.sh --no-open     don't launch after installing
#   ./scripts/setup-macos.sh --uninstall   remove the app and login item (keeps your photos)
set -euo pipefail

APP_NAME="Wallet Keepsake"
LABEL="com.walletkeepsake.app"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build-macos"
DEST="$HOME/Applications/$APP_NAME.app"
ITEMS="$HOME/Library/Application Support/wallet_image/items"
AGENT="$HOME/Library/LaunchAgents/$LABEL.plist"

AUTOSTART=0
OPEN_APP=1
UNINSTALL=0
for arg in "$@"; do
  case "$arg" in
    --autostart) AUTOSTART=1 ;;
    --no-open) OPEN_APP=0 ;;
    --uninstall) UNINSTALL=1 ;;
    -h|--help) sed -n '2,8p' "$0"; exit 0 ;;
    *) echo "Unknown option: $arg" >&2; exit 2 ;;
  esac
done

say()  { printf '\033[1;33m==>\033[0m %s\n' "$*"; }
die()  { printf '\033[1;31merror:\033[0m %s\n' "$*" >&2; exit 1; }

[ "$(uname -s)" = "Darwin" ] || die "This script is for macOS. See README.md for Linux/Windows."

if [ "$UNINSTALL" = 1 ]; then
  say "Removing $APP_NAME"
  pkill -x "$APP_NAME" >/dev/null 2>&1 || true
  if [ -f "$AGENT" ]; then
    launchctl bootout "gui/$(id -u)" "$AGENT" >/dev/null 2>&1 || true
    rm -f "$AGENT"
  fi
  rm -rf "$DEST"
  say "Done. Your photos are still in: $ITEMS"
  exit 0
fi

# 1. Toolchain -------------------------------------------------------------
if ! xcode-select -p >/dev/null 2>&1; then
  say "Installing Xcode Command Line Tools (a system dialog will open)"
  xcode-select --install || true
  die "Finish the Command Line Tools install, then run this script again."
fi
command -v git >/dev/null 2>&1 || die "git not found (it comes with the Command Line Tools)."

CMAKE="$(command -v cmake || true)"
if [ -z "$CMAKE" ] && [ -x "/Applications/CMake.app/Contents/bin/cmake" ]; then
  CMAKE="/Applications/CMake.app/Contents/bin/cmake"
fi
if [ -z "$CMAKE" ]; then
  if command -v brew >/dev/null 2>&1; then
    say "Installing CMake with Homebrew"
    brew install cmake
    CMAKE="$(command -v cmake)"
  else
    die "CMake not found. Install Homebrew (https://brew.sh) and re-run, or install CMake from https://cmake.org/download/"
  fi
fi

# 2. Build -----------------------------------------------------------------
say "Building (first build downloads raylib, takes a minute)"
"$CMAKE" -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
"$CMAKE" --build "$BUILD_DIR" -j "$(sysctl -n hw.ncpu)"

BUILT="$BUILD_DIR/$APP_NAME.app"
[ -d "$BUILT" ] || die "Build finished but $BUILT is missing."

# 3. Install ---------------------------------------------------------------
say "Installing to $DEST"
pkill -x "$APP_NAME" >/dev/null 2>&1 || true
mkdir -p "$HOME/Applications"
rm -rf "$DEST"
ditto "$BUILT" "$DEST"
# Ad-hoc signature so Apple Silicon runs the bundle after assets were copied in.
codesign --force --deep --sign - "$DEST" >/dev/null 2>&1 || say "codesign skipped (app may still run fine)"
xattr -dr com.apple.quarantine "$DEST" >/dev/null 2>&1 || true

mkdir -p "$ITEMS"

# 4. Start at login (optional) ---------------------------------------------
if [ "$AUTOSTART" = 1 ]; then
  say "Adding login item"
  mkdir -p "$(dirname "$AGENT")"
  cat > "$AGENT" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key><string>$LABEL</string>
    <key>ProgramArguments</key>
    <array><string>/usr/bin/open</string><string>-a</string><string>$DEST</string></array>
    <key>RunAtLoad</key><true/>
</dict>
</plist>
PLIST
fi

say "Installed. Put your photos and notes in:"
echo "    $ITEMS"
if [ "$OPEN_APP" = 1 ]; then
  open "$DEST"
  open "$ITEMS"
  say "Look for a strip of leather at the right edge of your screen and click it."
fi
