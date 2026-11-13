#!/usr/bin/env bash
# Compile and run the host-side unit tests for the pure C parts of udo-status.
set -euo pipefail
cd "$(dirname "$0")"
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
for t in test_packet test_view; do
    cc -std=c11 -Wall -Wextra -Werror -I../include \
        ../src/packet.c ../src/screen/udo_view.c "$t.c" -o "$out/$t"
    "$out/$t"
done
