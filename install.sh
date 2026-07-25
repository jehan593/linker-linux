#!/usr/bin/env bash
# Per-user install/uninstall for Linker — mirrors the Windows installer's philosophy
# (installer/linker.iss): no admin/root needed, everything lives under the user's own
# prefix, and uninstall never touches saved links/settings.
#
# Usage:
#   ./install.sh [PREFIX]              (default: ~/.local)
#   ./install.sh --uninstall [PREFIX]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

MODE="install"
PREFIX="$HOME/.local"
if [ "${1:-}" = "--uninstall" ]; then
    MODE="uninstall"
    PREFIX="${2:-$HOME/.local}"
elif [ -n "${1:-}" ]; then
    PREFIX="$1"
fi

BIN_DIR="$PREFIX/bin"
ASSETS_DIR="$PREFIX/share/linker"     # bundled fonts/icons only — NOT the runtime data
                                       # dir, even though it's the same path under the
                                       # default prefix (both resolve under
                                       # $XDG_DATA_HOME/linker); linker-data.json /
                                       # settings.json live alongside but are never
                                       # touched here.
APPS_DIR="$PREFIX/share/applications"
ICON_THEME_DIR="$PREFIX/share/icons/hicolor"
DESKTOP_FILE="$APPS_DIR/linker.desktop"

if [ "$MODE" = "uninstall" ]; then
    echo "Uninstalling Linker from $PREFIX..."
    rm -f "$BIN_DIR/linker"
    rm -rf "$ASSETS_DIR/fonts" "$ASSETS_DIR/icons"
    rmdir "$ASSETS_DIR" 2>/dev/null || true
    rm -f "$DESKTOP_FILE"
    for size in 16 32 48 256; do
        rm -f "$ICON_THEME_DIR/${size}x${size}/apps/linker.png"
    done
    command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$APPS_DIR" >/dev/null 2>&1 || true
    command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -t "$ICON_THEME_DIR" >/dev/null 2>&1 || true
    echo "Done. Saved links and Notesnook settings under \$XDG_DATA_HOME/linker were left alone."
    exit 0
fi

if [ ! -x "$SCRIPT_DIR/linker" ]; then
    echo "error: linker binary not found next to install.sh — run 'make' first" >&2
    exit 1
fi

echo "Installing Linker to $PREFIX..."
mkdir -p "$BIN_DIR" "$ASSETS_DIR/fonts" "$ASSETS_DIR/icons" "$APPS_DIR"

install -m 755 "$SCRIPT_DIR/linker" "$BIN_DIR/linker"
cp "$SCRIPT_DIR"/data/fonts/*.ttf "$ASSETS_DIR/fonts/"
cp "$SCRIPT_DIR"/data/fonts/MARTIAN_MONO_LICENSE.txt "$ASSETS_DIR/fonts/" 2>/dev/null || true
cp "$SCRIPT_DIR"/data/icons/*.png "$ASSETS_DIR/icons/"

for size in 16 32 48 256; do
    dir="$ICON_THEME_DIR/${size}x${size}/apps"
    mkdir -p "$dir"
    cp "$SCRIPT_DIR/data/icons/linker-${size}.png" "$dir/linker.png"
done

cat > "$DESKTOP_FILE" <<EOF
[Desktop Entry]
Type=Application
Name=Linker
Comment=Choose which browser opens a link
Exec=$BIN_DIR/linker %u
Icon=linker
Terminal=false
Categories=Network;
MimeType=x-scheme-handler/http;x-scheme-handler/https;
NoDisplay=false
EOF

command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$APPS_DIR" >/dev/null 2>&1 || true
command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -t "$ICON_THEME_DIR" >/dev/null 2>&1 || true

echo "Installed."
echo "Make sure $BIN_DIR is on your PATH, then run 'linker' — or find Linker in your applications menu."
echo "Open Manage Browsers and click \"Set as default browser\" to register Linker as your link handler."
