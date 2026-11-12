#!/usr/bin/env bash
# Build the firmware locally with the same Docker image as GitHub Actions.
#
#   ./build.sh            build every target in build.yaml
#   ./build.sh left       build only targets whose shield matches "left"
#   ./build.sh --update   re-run `west update` (after changing config/west.yml)
#   ./build.sh --clean    delete the cached workspace volume
#
# Firmware lands in ./firmware. ZMK + Zephyr are cached in a Docker volume, so
# only the first run is slow.
set -euo pipefail

IMAGE=zmkfirmware/zmk-build-arm:stable
VOLUME=corne-zmk-ws
REPO=$(cd "$(dirname "$0")" && pwd)

update=0
filter=""
case "${1:-}" in
    --clean) docker volume rm "$VOLUME"; exit ;;
    --update) update=1 ;;
    "") ;;
    *) filter=$1 ;;
esac

# "board|shield" per target from build.yaml
targets=$(awk '
    /board:/  { if (b) print b "|" s; b = $NF; s = "" }
    /shield:/ { sub(/.*shield: */, ""); s = $0 }
    END       { if (b) print b "|" s }
' "$REPO/build.yaml" | grep -- "$filter" || true)
[[ -n $targets ]] || { echo "no target matches '$filter'" >&2; exit 1; }

mkdir -p "$REPO/firmware"

docker run --rm \
    -v "$VOLUME":/ws \
    -v "$REPO":/repo:ro \
    -v "$REPO/firmware":/out \
    -e TARGETS="$targets" -e UPDATE="$update" \
    -w /ws "$IMAGE" bash -euo pipefail -c '
        if [[ ! -d .west ]]; then
            mkdir -p manifest
            cp /repo/config/west.yml manifest/
            west init -l manifest
            UPDATE=1
        fi
        if [[ $UPDATE == 1 ]]; then
            cp /repo/config/west.yml manifest/
            west update --narrow -o=--depth=1
        fi
        west zephyr-export >/dev/null
        mkdir -p build

        failed=0
        while IFS="|" read -r board shield; do
            name="${shield:+${shield// /-}-}$board"
            echo "==> $name"
            if west build -p auto -s zmk/app -d "build/$name" -b "$board" -- \
                ${shield:+-DSHIELD="$shield"} \
                -DZMK_CONFIG=/repo/config -DZMK_EXTRA_MODULES=/repo \
                > "build/$name.log" 2>&1; then
                cp "build/$name/zephyr/zmk.uf2" "/out/$name.uf2"
                echo "    ok -> firmware/$name.uf2"
            else
                cp "build/$name.log" "/out/$name.log"
                grep -iE "error|warning: .*undefined" "build/$name.log" | head -15 || tail -20 "build/$name.log"
                echo "    FAILED -> full log in firmware/$name.log"
                failed=1
            fi
        done <<< "$TARGETS"
        exit $failed
    '
