#!/usr/bin/env bash
# Set draw/overview.svg as the macOS wallpaper on every Space and display.
#
#   ./draw.sh && ./wallpaper.sh
#
# The image is rendered at the native resolution of RENDER_FOR (falls back to
# the first display if it's not connected), centered on black. SCALE is kept
# small enough that the drawing isn't cropped on the narrower MacBook screen.
#
# AppleScript can only change the visible Space, so this writes the picture
# into macOS' wallpaper store for all Spaces/displays (plus the default for new
# Spaces) and restarts WallpaperAgent. The store is backed up to Index.plist.bak.
set -euo pipefail

RENDER_FOR=${1:-LG Monitor}
SCALE=0.75                     # share of the screen width the keymap may cover
BG=000000                      # wallpaper background
REPO=$(cd "$(dirname "$0")" && pwd)
SVG=$REPO/draw/overview.svg
OUT_DIR=$HOME/Pictures/corne-keymap
STORE="$HOME/Library/Application Support/com.apple.wallpaper/Store/Index.plist"

[[ -f $SVG ]] || { echo "missing $SVG, run ./draw.sh first" >&2; exit 1; }

# native pixel resolution, e.g. "6720 3780" (named display, else the first one)
read -r W H < <(system_profiler SPDisplaysDataType | awk -v name="$RENDER_FOR:" '
    { sub(/^ +/, "") }
    $0 == name { found = 1 }
    /^Resolution:/ { if (!first) first = $2 " " $4; if (found) { print $2, $4; done = 1; exit } }
    END { if (!done) print first }
')

# fit the drawing into SCALE of the screen, keeping its aspect ratio
read -r SVG_W SVG_H < <(sed -n 's/.*<svg width="\([0-9.]*\)" height="\([0-9.]*\)".*/\1 \2/p' "$SVG" | head -1)
IMG_W=$(awk -v w="$W" -v h="$H" -v sw="$SVG_W" -v sh="$SVG_H" -v s="$SCALE" \
    'BEGIN { f = s * w / sw; g = s * h / sh; if (g < f) f = g; printf "%d", sw * f }')

# unique file name, otherwise macOS keeps showing its cached old picture
mkdir -p "$OUT_DIR"
OLD=$(ls "$OUT_DIR"/keymap-*.png 2>/dev/null || true)
OUT=$OUT_DIR/keymap-$(date +%Y%m%d-%H%M%S).png
rsvg-convert -w "$IMG_W" -b "#$BG" "$SVG" -o "$OUT"
sips --padToHeightWidth "$H" "$W" --padColor "$BG" "$OUT" >/dev/null 2>&1

# point every image wallpaper entry in the store at the new picture
cp "$STORE" "$STORE.bak"
/usr/bin/python3 -I - "$STORE" "$OUT" <<'EOF'
import plistlib, sys, urllib.parse
store, picture = sys.argv[1], sys.argv[2]
url = "file://" + urllib.parse.quote(picture)
data = plistlib.load(open(store, "rb"))
count = 0

def walk(node):
    global count
    if isinstance(node, dict):
        if node.get("Provider") == "com.apple.wallpaper.choice.image":
            node["Files"] = [{"relative": url}]
            count += 1
        for value in node.values():
            walk(value)
    elif isinstance(node, list):
        for value in node:
            walk(value)

walk(data)
plistlib.dump(data, open(store, "wb"), fmt=plistlib.FMT_BINARY)
print(f"updated {count} wallpaper entries")
EOF
killall WallpaperAgent 2>/dev/null || true

# remove pictures from earlier runs (no longer referenced)
[[ -n $OLD ]] && rm -f $OLD
echo "wallpaper (${W}x${H}) on all Spaces: $OUT"
