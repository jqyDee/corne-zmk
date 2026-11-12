"""Post-process the parsed keymap (run by draw.sh inside the container).

keymap.yaml:  move all combos onto the virtual "Combos" layer.
overview.yaml: one combined layer, base legend in the middle and the other
               layers in the key corners (colors set in draw/config.yaml).
"""
import copy

import yaml

# layer -> key corner; must match the text.tl/tr/bl colors in config.yaml
CORNERS = {"sym": "tl", "num": "tr", "nav": "bl"}

# shorter corner legends so they fit next to the homerow-mod labels
SHORT = {
    "Mouse ↑": "Ms↑", "Mouse ↓": "Ms↓", "Mouse ←": "Ms←", "Mouse →": "Ms→",
    "Scroll ↑": "Sc↑", "Scroll ↓": "Sc↓", "Scroll ←": "Sc←", "Scroll →": "Sc→",
    "Left Click": "LClk", "Right Click": "RClk", "BT Clear": "BTclr",
}

path = "draw/keymap.yaml"
d = yaml.safe_load(open(path))
for c in d.get("combos", []):
    c["l"] = ["Combos"]
yaml.safe_dump(d, open(path, "w"), allow_unicode=True, sort_keys=False)


def label(key):
    """Tap legend of a key, or None if it just passes through."""
    if isinstance(key, dict):
        if key.get("type") in ("trans", "held"):
            return None
        return key.get("t")
    return key


overview = []
for i, key in enumerate(d["layers"]["base"]):
    key = copy.deepcopy(key) if isinstance(key, dict) else {"t": key}
    for layer, corner in CORNERS.items():
        legend = label(d["layers"][layer][i])
        if legend not in (None, ""):
            key[corner] = SHORT.get(legend, legend)
    overview.append(key)

o = copy.deepcopy(d)
title = "overview:  " + "   ".join(
    f"{'↖↗↙↘'['tl tr bl br'.split().index(c)]} {l}" for l, c in CORNERS.items()
)
o["layers"] = {title: overview}
for c in o.get("combos", []):  # combos drawn right on the overview keyboard
    c["l"] = [title]
# bigger keys for the corner legends, gaps between keys for the combo boxes.
# The QMK layout uses key_h as the unit in both directions (key_w is ignored);
# visible key = unit - 2 * inner_pad.
o["draw_config"] = {
    "key_h": 104, "inner_pad_w": 13, "inner_pad_h": 13,
    "combo_w": 22, "combo_h": 18, "n_columns": 1,
}
yaml.safe_dump(o, open("draw/overview.yaml", "w"), allow_unicode=True, sort_keys=False)
