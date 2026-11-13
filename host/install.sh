#!/usr/bin/env bash
# Install (or with --uninstall remove) the udo-status LaunchAgent, which runs
# host/udo_keyboard.py at login and keeps it running.
set -euo pipefail

REPO=$(cd "$(dirname "$0")/.." && pwd)
LABEL=local.corne.udo-status
PLIST=~/Library/LaunchAgents/$LABEL.plist
DOMAIN=gui/$(id -u)

launchctl bootout "$DOMAIN/$LABEL" 2>/dev/null || true
if [[ ${1:-} == --uninstall ]]; then
    rm -f "$PLIST"
    echo "removed $LABEL"
    exit
fi

[[ -x $REPO/host/.venv/bin/python ]] || python3 -m venv "$REPO/host/.venv"
"$REPO/host/.venv/bin/pip" install -q -r "$REPO/host/requirements.txt"

mkdir -p ~/Library/LaunchAgents
sed -e "s|__PYTHON__|$REPO/host/.venv/bin/python|" -e "s|__REPO__|$REPO|" \
    "$REPO/host/$LABEL.plist" > "$PLIST"
launchctl bootstrap "$DOMAIN" "$PLIST"
echo "installed $LABEL (log: /tmp/udo-status.log)"
