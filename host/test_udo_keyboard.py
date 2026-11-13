"""Tests for udo_keyboard. Run: python3 -m unittest host/test_udo_keyboard.py"""
import os
import sys
import unittest
from datetime import datetime, timedelta, timezone

sys.path.insert(0, os.path.dirname(__file__))
import udo_keyboard as uk  # noqa: E402

NOW = datetime(2026, 11, 13, 12, 0, 0, tzinfo=timezone.utc)
# Shared with the firmware tests: running, 5025 s, "lab 3"
SHARED_VECTOR = bytes.fromhex("55010100a11300006c61622033000000" "00000000")


def running(task, started):
    return {"task": task, "started": started.isoformat(), "ended": None, "minutes": 0}


class EncodeTest(unittest.TestCase):
    def test_encode_shared_vector(self):
        packet = uk.encode(running("lab 3", NOW - timedelta(seconds=5025)), NOW)
        self.assertEqual(packet, SHARED_VECTOR)

    def test_encode_idle(self):
        self.assertEqual(uk.encode(None, NOW), bytes([0x55, 1, 0, 0]) + bytes(16))

    def test_name_transliterated_and_truncated(self):
        self.assertEqual(uk.ascii_name("Übung Größe 🚀 extra"), b"Uebung Groes")
        self.assertEqual(len(uk.ascii_name("x" * 40)), 12)

    def test_negative_elapsed_clamped(self):
        packet = uk.encode(running("t", NOW + timedelta(minutes=5)), NOW)
        self.assertEqual(packet[4:8], bytes(4))

    def test_started_with_offset_parses(self):
        # udo prints local time with offset, e.g. 2026-11-13T01:36:34.887+01:00
        packet = uk.encode({"task": "t", "started": "2026-11-13T12:59:00.000+01:00"}, NOW)
        self.assertEqual(int.from_bytes(packet[4:8], "little"), 60)


class FakeDevice:
    def __init__(self, log):
        self.log = log

    def open_path(self, path):
        self.log.append(("open", path))

    def write(self, data):
        self.log.append(("write", bytes(data)))
        if self.log.count(("fail",)) == 0:
            self.log.append(("fail",))
            raise OSError("device gone")
        return len(data)

    def close(self):
        self.log.append(("close",))


class FakeHid:
    def __init__(self):
        self.log = []

    def enumerate(self):
        return [{"usage_page": 0xFF60, "usage": 0x61, "path": b"dev1"},
                {"usage_page": 0x0001, "usage": 0x06, "path": b"keyboard"}]

    def device(self):
        return FakeDevice(self.log)


class SenderTest(unittest.TestCase):
    def test_send_failure_reopens(self):
        fake = FakeHid()
        sender = uk.Sender(hid_module=fake)
        self.assertFalse(sender.send(SHARED_VECTOR))
        self.assertTrue(sender.send(SHARED_VECTOR))
        self.assertEqual([e for e in fake.log if e[0] == "open"], [("open", b"dev1")] * 2)

    def test_report_is_prefixed_and_padded(self):
        fake = FakeHid()
        sender = uk.Sender(hid_module=fake)
        sender.send(SHARED_VECTOR)
        sender.send(SHARED_VECTOR)
        writes = [e[1] for e in fake.log if e[0] == "write"]
        self.assertEqual(writes[-1], b"\x00" + SHARED_VECTOR + bytes(12))

    def test_no_device_returns_false(self):
        fake = FakeHid()
        fake.enumerate = lambda: []
        self.assertFalse(uk.Sender(hid_module=fake).send(SHARED_VECTOR))


class ChangeTest(unittest.TestCase):
    def test_changed_ignores_elapsed(self):
        a = uk.encode(running("lab 3", NOW - timedelta(seconds=10)), NOW)
        b = uk.encode(running("lab 3", NOW - timedelta(seconds=70)), NOW)
        c = uk.encode(running("lab 4", NOW - timedelta(seconds=70)), NOW)
        self.assertFalse(uk.changed(a, b))
        self.assertTrue(uk.changed(a, c))
        self.assertTrue(uk.changed(None, a))


if __name__ == "__main__":
    unittest.main()
