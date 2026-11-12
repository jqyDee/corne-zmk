#!/usr/bin/env bash
# Draw the keymap with keymap-drawer (in Docker):
#
#   ./draw.sh
#
#   draw/keymap.svg    every layer separately, combos on their own layer
#   draw/overview.svg  all layers on one keyboard (other layers in key corners)
#
# PNGs need rsvg-convert (brew install librsvg); without it you get SVGs only.
set -euo pipefail

IMAGE=python:3.12-slim
VOLUME=corne-zmk-draw  # caches pip packages and the QMK layout download
REPO=$(cd "$(dirname "$0")" && pwd)
LAYERS="base Combos nav num sym"
KEYBOARD="-k crkbd/rev1 -l LAYOUT_split_3x6_3"

docker run --rm \
    -v "$VOLUME":/root/.cache \
    -v "$REPO":/repo \
    -e LAYERS="$LAYERS" -e KEYBOARD="$KEYBOARD" \
    -w /repo "$IMAGE" bash -euo pipefail -c '
        pip install -q --root-user-action=ignore --disable-pip-version-check keymap-drawer
        keymap -c draw/config.yaml parse -z config/corne.keymap --virtual-layers Combos > draw/keymap.yaml
        python draw/postprocess.py
        keymap -c draw/config.yaml draw draw/keymap.yaml $KEYBOARD -s $LAYERS > draw/keymap.svg
        keymap -c draw/config.yaml draw draw/overview.yaml $KEYBOARD > draw/overview.svg
    '

if command -v rsvg-convert >/dev/null; then
    for f in keymap overview; do  # gruvbox background
        rsvg-convert -z 2 -b '#282828' "draw/$f.svg" -o "draw/$f.png"
    done
    echo "draw/{keymap,overview}.svg + .png"
else
    echo "draw/{keymap,overview}.svg (install librsvg for PNGs)"
fi
