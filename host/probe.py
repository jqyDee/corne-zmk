"""Task 1 spike: check that raw HID reports reach the central (left) half.

The spike firmware echoes every received report back, so this script needs
no serial console:

    host/.venv/bin/python host/probe.py list
    host/.venv/bin/python host/probe.py send

Finding (2026-11-13, macOS, ZMK v0.3 + zmk-raw-hid 6a37765):
    BLE: works with a leading 0x00 report-ID byte (33-byte write).
    USB: works with and without the prefix.
    With USB plugged in, BLE and USB each show up as a device; both work.
    -> host daemon always prefixes 0x00 and writes to every matching device.
"""
import sys

import hid

USAGE_PAGE = 0xFF60
USAGE = 0x61
REPORT_SIZE = 32
# Shared test vector: running, 5025 s, "lab 3"
VECTOR = bytes.fromhex("55010100a11300006c61622033000000" "00000000")


def devices():
    return [d for d in hid.enumerate() if d["usage_page"] == USAGE_PAGE and d["usage"] == USAGE]


def cmd_list():
    found = devices()
    if not found:
        print("no raw HID device (usage page 0xFF60) found")
    for d in found:
        print(f"{d['product_string']!r} vid={d['vendor_id']:#06x} pid={d['product_id']:#06x} "
              f"path={d['path'].decode(errors='replace')}")


def try_variant(path, label, payload):
    dev = hid.device()
    dev.open_path(path)
    try:
        n = dev.write(payload)
        echo = bytes(dev.read(64, 1000))
    finally:
        dev.close()
    ok = echo[: len(VECTOR)] == VECTOR
    print(f"  {label}: wrote {n}, echo {echo[:20].hex(' ') or '(none)'} -> {'OK' if ok else 'no match'}")
    return ok


def cmd_send():
    found = devices()
    if not found:
        sys.exit("no raw HID device found; is the keyboard connected?")
    report = VECTOR + bytes(REPORT_SIZE - len(VECTOR))
    for d in found:
        print(f"{d['product_string']!r} {d['path'].decode(errors='replace')}")
        try_variant(d["path"], "with 0x00 report-ID prefix", b"\x00" + report)
        try_variant(d["path"], "without prefix            ", report)


if __name__ == "__main__":
    {"list": cmd_list, "send": cmd_send}.get(sys.argv[1] if len(sys.argv) > 1 else "", lambda: sys.exit(__doc__))()
