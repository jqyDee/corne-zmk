# udo Status on Right nice!view Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Show the currently tracked `udo` task and its elapsed time on the right (peripheral) half's nice!view, in place of the balloon/mountain art, while the left half stays central.

**Architecture:** A macOS daemon polls `udo status --json` and sends a 20-byte packet as a raw HID output report to the left half (central), using the `zmk-raw-hid` module. A custom module on the central forwards that packet unchanged to the right half through its own GATT service on the existing split BLE link. On the peripheral, a custom status screen keeps the stock battery and connection canvas and replaces the art with a udo canvas. The timer counts locally and redraws once a minute.

**Tech Stack:** ZMK `v0.3` (Zephyr 3.5, LVGL 8), C, Zephyr BT GATT, `zzeneg/zmk-raw-hid`, Python 3.13 + `hidapi` (pip), stdlib `unittest`, launchd.

**Spec:** Inline below. Agreed in chat on 2026-11-13. There is no separate spec file.

### Spec (from chat)

- Left half stays central. The right screen currently shows only art and becomes the udo display. The left screen is unchanged.
- Data source: the `udo` CLI (`/opt/homebrew/bin/udo`). `udo status --json` returns `{"running": null}` when idle, or `{"running": {"task": "demo task", "started": "2026-11-13T01:36:34.887+01:00", "ended": null, "minutes": 0, "source": "manual", "owner": "manual"}}`.
- Transport: Option 1. Raw HID from host to central, then a custom split relay service to the peripheral. No ZMK fork.
- Bluetooth impact must stay negligible: send on change plus a sparse heartbeat. Nothing is sent per second. The peripheral counts the timer itself.

## Global Constraints

> **Superseded during execution (2026-11-13):** the packet became 20..32 bytes
> (24-char name, `[8..31]`); the central requests a larger ATT MTU and falls
> back to the 20-byte form with a 12-char name. The udo area is 68x140, not
> 68x68. See readme "udo on the right screen" for the final format.

- ZMK stays pinned to `revision: v0.3` in `config/west.yml`. Pin `zmk-raw-hid` to a commit SHA, not `main`.
- The packet is exactly 20 bytes, so it fits one ATT write at the default MTU of 23. The raw HID report is that packet zero-padded to `CONFIG_RAW_HID_REPORT_SIZE=32`.
- Packet layout (little-endian): `[0]=0x55` magic, `[1]=0x01` version, `[2]` state (`0`=idle, `1`=running), `[3]=0x00` reserved, `[4..7]` elapsed seconds as `uint32`, `[8..19]` task name as 12 bytes of ASCII, NUL-padded with no terminator required.
- Raw HID: usage page `0xFF60`, usage `0x61` (module defaults). The host matches the device by usage page and usage.
- GATT service UUID `26931a33-2592-4c15-a55b-e029354b8347`. Characteristic UUID `f2d74a4b-73b0-4dae-91e3-27ecbb34c1b5`. Properties: `WRITE_WITHOUT_RESP`, permission `BT_GATT_PERM_WRITE_ENCRYPT`.
- Daemon timing: poll udo every 10 s, send immediately on change, heartbeat every 60 s. Peripheral timing: redraw every 60 s while running, and treat the data as stale after 180 s without a packet.
- Fonts: only built-in LVGL Montserrat (ASCII), so the host transliterates task names to ASCII.
- `raw_hid_adapter` shield and `CONFIG_RAW_HID=y` go on the **left build only**. `CONFIG_NICE_VIEW_WIDGET_STATUS=n` goes on the **right build only**. Both are set through `cmake-args` in `build.yaml`.
- No personal data in committed files. The launchd label is `local.corne.udo-status`.

## Review Focus

1. **Keyboard asleep or disconnected** (`CONFIG_ZMK_SLEEP=y`). The HID device vanishes and writes raise. Expected: the daemon logs once, retries opening every 10 s, and never crashes. Pinned by `test_send_failure_reopens` (Task 2).
2. **Daemon not running, or host on another BLE profile.** Expected: the right screen shows `udo ?` after 180 s instead of a frozen timer. Pinned by `test_stale_after_timeout` (Task 3).
3. **Non-ASCII or long task names** (German umlauts, emoji, more than 12 bytes). Expected: `ä→ae`, `ö→oe`, `ü→ue`, `ß→ss`, other non-ASCII characters dropped, cut at 12 bytes, no garbage glyphs. Pinned by `test_name_transliterated_and_truncated` (Task 2).
4. **Right half reconnects or power-cycles while a task runs.** Expected: the screen shows the correct task within 60 s, through the heartbeat plus rediscovery on `security_changed`. Pinned by manual check M3 (Task 5).
5. **Foreign raw HID reports on `0xFF60`** (for example from qmk-hid-host or VIA-style tools). Expected: the central ignores any packet without magic `0x55`/version `0x01`. Pinned by `test_decode_rejects_bad_magic` (Task 3).

---

## File Structure

```
CMakeLists.txt                       NEW  root: add_subdirectory for each module
Kconfig                              NEW  root: rsource each module's Kconfig
zephyr/module.yml                    MOD  cmake: . / kconfig: Kconfig
modules/udo-status/
  CMakeLists.txt                     NEW  role-based source selection
  Kconfig                            NEW  CONFIG_UDO_STATUS + font selects
  include/udo_status/packet.h        NEW  struct + decode (pure C, host-testable)
  include/udo_status/uuids.h         NEW  service/char UUIDs
  include/udo_status/event.h         NEW  zmk event udo_status_changed
  src/packet.c                       NEW  udo_packet_decode()
  src/event.c                        NEW  ZMK_EVENT_IMPL
  src/central_relay.c                NEW  raw HID listener → GATT write
  src/peripheral_service.c           NEW  GATT service → raise event
  src/screen/custom_status_screen.c  NEW  zmk_display_status_screen()
  src/screen/udo_widget.c/.h         NEW  udo canvas + 60 s tick + stale
  src/screen/peripheral_status.c/.h  NEW  vendored from ZMK v0.3 nice_view, art removed
  src/screen/util.c/.h, bolt.c       NEW  vendored unchanged from ZMK v0.3 nice_view (MIT)
  src/screen/udo_view.c/.h           NEW  pure formatting: udo_view_text() (host-testable)
  tests/test_packet.c                NEW  host cc test
  tests/test_view.c                  NEW  host cc test
  tests/run.sh                       NEW  compile + run host tests with cc
host/
  udo_keyboard.py                    NEW  daemon (poll, encode, send)
  test_udo_keyboard.py               NEW  unittest
  requirements.txt                   NEW  hidapi
  local.corne.udo-status.plist       NEW  LaunchAgent template
  probe.py                           NEW  Task 1 spike: list + send test report
config/west.yml                      MOD  add zmk-raw-hid
build.yaml                           MOD  shields + cmake-args per half
build.sh                             MOD  parse cmake-args
.github/workflows/build.yml          MOD  trigger on modules/**, CMakeLists.txt, Kconfig, zephyr/**
readme.md                            MOD  section "udo on the right screen"
```

Shared test vector, used by C and Python tests alike. Running, 5025 s, name `"lab 3"`:
`55 01 01 00 a1 13 00 00 6c 61 62 20 33 00 00 00 00 00 00 00`

---

### Task 1: Spike — raw HID reaches the central over BLE on macOS

This task is the risk gate. If it fails, stop and report back before doing Tasks 3–5.

**Files:**
- Modify: `config/west.yml`, `build.yaml`, `build.sh`, `.github/workflows/build.yml`
- Create: `host/probe.py`, `host/requirements.txt`

**Interfaces:**
- Produces: a build where the left half has `zmk-raw-hid`, and per-target `cmake-args` support in `build.sh`. It also produces a finding written into `host/probe.py`'s docstring: whether macOS needs a leading report-ID byte `0x00` when writing over BLE and over USB.

- [ ] **Step 1:** In `config/west.yml`, add remote `zzeneg` (`https://github.com/zzeneg`) and project `zmk-raw-hid`, pinned to the current `main` SHA (`gh api repos/zzeneg/zmk-raw-hid/commits/main --jq .sha`).
- [ ] **Step 2:** In `build.yaml`:
  - Left: `shield: corne_left nice_view_adapter nice_view raw_hid_adapter` plus `cmake-args: -DCONFIG_RAW_HID=y`.
  - Right: `cmake-args: -DCONFIG_NICE_VIEW_WIDGET_STATUS=n`. Leave that line commented out until Task 4.
- [ ] **Step 3:** Change `build.sh`'s awk so it outputs `board|shield|cmake_args`. Pass `$cmake_args` after `-DZMK_EXTRA_MODULES=/repo`.
- [ ] **Step 4:** Add `modules/**`, `CMakeLists.txt`, `Kconfig`, `zephyr/**` to the workflow's `paths`.
- [ ] **Step 5:** Run `./build.sh --update left`. Expected: `ok -> firmware/corne_left-nice_view_adapter-nice_view-raw_hid_adapter-nice_nano_v2.uf2`. If it fails with an API error, `zmk-raw-hid` main is incompatible with v0.3. Bisect its history for the newest commit that builds and pin that SHA.
- [ ] **Step 6:** Write `host/probe.py` with two modes:
  - `list`: print every `hid.enumerate()` entry with `usage_page == 0xFF60`.
  - `send`: open that entry and write the shared test vector padded to 32 bytes, once with and once without a leading `0x00`, printing each return value.
- [ ] **Step 7:** Temporarily add a `LOG_INF` listener on the central for `raw_hid_received_event`. Flash the left half, then run `python3 host/probe.py send` over BLE and again over USB. Expected: the USB log (`CONFIG_ZMK_USB_LOGGING=y` for this spike only) shows `55 01 01 00 a1 13 …`. Write down in the docstring which variant arrived intact. Revert the logging listener and the config.
- [ ] **Step 8: Commit** `build: add zmk-raw-hid to left half, cmake-args in build.sh`.

### Task 2: Host daemon

**Files:**
- Create: `host/udo_keyboard.py`, `host/test_udo_keyboard.py`, `host/local.corne.udo-status.plist`
- Modify: `host/requirements.txt`

**Interfaces:**
- Produces:
  - `encode(status: dict | None, now: datetime) -> bytes`, always 20 bytes. `status` is the `running` object, or `None`.
  - `ascii_name(task: str) -> bytes`, up to 12 bytes.
  - `class Sender` with `send(packet: bytes) -> bool`. It opens the device lazily (usage page `0xFF60`, usage `0x61`) and applies Task 1's report-ID finding. On `OSError` it closes the device, returns `False`, and reopens on the next call.
  - `main()` loop: poll every 10 s, send immediately when state or name differs from the last sent packet, otherwise every 60 s.
  - Elapsed time is `now - started` (parsed with `datetime.fromisoformat`), clamped at `0` and at `2**32-1`.

- [ ] **Step 1: Write failing tests** in `test_udo_keyboard.py`:
  - `test_encode_shared_vector`: started 5025 s before `now`, task `"lab 3"`, result equals the shared vector.
  - `test_encode_idle`: `encode(None, now) == bytes([0x55,1,0,0]) + bytes(16)`.
  - `test_name_transliterated_and_truncated`:
    - `ascii_name("Übung Größe 🚀 extra") == b"Uebung Groes"` (ö→oe, then cut at 12)
    - `len(ascii_name("x"*40)) == 12`
  - `test_negative_elapsed_clamped`: `started` in the future gives elapsed bytes `00 00 00 00`.
  - `test_send_failure_reopens`: a fake `hid.device` whose `write` raises `OSError` once and then works. The first `send` returns `False`, the second returns `True`, and `open_path` is called twice.
- [ ] **Step 2:** Run `python3 -m unittest host/test_udo_keyboard.py`. Expected: FAIL (ImportError).
- [ ] **Step 3:** Implement `host/udo_keyboard.py`. Get status with `subprocess.run(["udo","status","--json"], capture_output=True, timeout=5)`. If udo fails or prints bad JSON, log it and skip that cycle without sending. `hid` is imported inside `Sender`, so the tests run without hidapi installed.
- [ ] **Step 4:** Run the tests. Expected: 5 tests OK.
- [ ] **Step 5:** Write the LaunchAgent template with `KeepAlive=true`, `RunAtLoad=true`, `ProgramArguments=[__PYTHON__, __REPO__/host/udo_keyboard.py]`, and `PATH` including `/opt/homebrew/bin`. Logs go to `/tmp/udo-status.log`.
- [ ] **Step 6: Commit** `feat(host): udo status daemon sending raw HID packets`.

### Task 3: Module skeleton, packet decode, view formatting (host-tested C)

**Files:**
- Create: root `CMakeLists.txt` and `Kconfig`, `modules/udo-status/{CMakeLists.txt,Kconfig}`, `include/udo_status/{packet.h,uuids.h,event.h}`, `src/{packet.c,event.c}`, `src/screen/udo_view.{c,h}`, `tests/{test_packet.c,test_view.c,run.sh}`
- Modify: `zephyr/module.yml`

**Interfaces:**
- Produces:
  ```c
  // packet.h
  #define UDO_PACKET_LEN 20
  #define UDO_NAME_LEN 12
  struct udo_status { bool running; uint32_t elapsed_s; char name[UDO_NAME_LEN + 1]; };
  int udo_packet_decode(const uint8_t *buf, size_t len, struct udo_status *out); // 0 or -EINVAL
  // event.h — ZMK event, payload copied
  struct udo_status_changed { struct udo_status status; };
  ZMK_EVENT_DECLARE(udo_status_changed);
  // udo_view.h — pure, no LVGL
  enum udo_view_kind { UDO_VIEW_UNKNOWN, UDO_VIEW_IDLE, UDO_VIEW_RUNNING };
  struct udo_view { enum udo_view_kind kind; char time[8]; char name[UDO_NAME_LEN + 1]; };
  void udo_view_build(const struct udo_status *s, bool have_data, int64_t age_ms,
                      uint32_t extra_s, struct udo_view *out);
  ```
  - `uuids.h` defines `UDO_SVC_UUID` and `UDO_CHR_UUID` with `BT_UUID_128_ENCODE`, using the UUIDs from Global Constraints.
  - Kconfig adds `config UDO_STATUS` (bool, `default n`, `depends on ZMK_SPLIT`). On the peripheral it also selects `LV_FONT_MONTSERRAT_14` and `LV_USE_CANVAS`.
  - `CMakeLists.txt` compiles:
    - always: `packet.c` and `event.c`
    - central: `central_relay.c`
    - peripheral: `peripheral_service.c`, plus everything in `screen/` when `CONFIG_ZMK_DISPLAY` is set

- [ ] **Step 1: Write failing tests** (`tests/test_packet.c`, `tests/test_view.c`, plain `assert`):
  - `test_decode_shared_vector`: running is true, elapsed is 5025, name is `"lab 3"`.
  - `test_decode_rejects_bad_magic`: byte 0 = `0x00` returns `-EINVAL`. Version `0x02` returns `-EINVAL`. `len` 19 returns `-EINVAL`. `len` 32 (padded HID report) returns 0.
  - `test_decode_name_full_12_no_nul`: 12 non-NUL bytes give `strlen(name) == 12` (terminated).
  - `test_view_running_format`: elapsed 5025, extra 0 gives `time == "1:23"`. Elapsed 5025, extra 120 gives `"1:25"`. 36000 s gives `"10:00"`.
  - `test_view_idle`: kind is `UDO_VIEW_IDLE`.
  - `test_stale_after_timeout`: `age_ms` 180001 gives `UDO_VIEW_UNKNOWN`. `age_ms` 179999 gives running. `have_data=false` gives `UNKNOWN`.
- [ ] **Step 2:** Write `tests/run.sh`, which runs `cc -std=c11 -Wall -Werror -I../include ../src/packet.c ../src/screen/udo_view.c test_x.c` and then the result, for each test. Run it. Expected: compile FAIL (missing sources).
- [ ] **Step 3:** Implement `packet.c` and `udo_view.c`. Keep them free of Zephyr headers. They include only `<stdint.h>`, `<stdbool.h>`, `<string.h>`, `<errno.h>`, `<stdio.h>`, so they compile on both host and target. The time format is `"%u:%02u"` (hours:minutes).
- [ ] **Step 4:** Run `modules/udo-status/tests/run.sh`. Expected: all pass.
- [ ] **Step 5:** Set up the root `CMakeLists.txt` (`add_subdirectory(vendor/zmk-auto-layer)`, `add_subdirectory(modules/udo-status)`) and root `Kconfig` (`rsource` both). Change `zephyr/module.yml` to `cmake: .` and `kconfig: Kconfig`, keeping `dts_root: vendor/zmk-auto-layer`. Add `CONFIG_UDO_STATUS=y` to `config/corne.conf`.
- [ ] **Step 6:** Run `./build.sh`. Expected: all 3 targets `ok`. Check that `num_word` still compiles in on the left: `grep -c behavior_auto_layer` in the left build log is greater than 0.
- [ ] **Step 7: Commit** `feat(udo-status): module skeleton, packet decode, view model`.

### Task 4: Peripheral GATT service + udo screen on the right half

**Files:**
- Create: `src/peripheral_service.c`, `src/screen/*` (vendored files and new files listed in File Structure)
- Modify: `build.yaml` (uncomment the right half's `cmake-args`)

**Interfaces:**
- Consumes: `udo_packet_decode`, `udo_status_changed`, `udo_view_build`, `UDO_SVC_UUID`/`UDO_CHR_UUID`.
- Produces: a GATT service on the peripheral. Its write handler decodes the packet and raises `udo_status_changed` via `raise_udo_status_changed(...)`. It returns `BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED)` if decoding fails.

- [ ] **Step 1:** Vendor `custom_status_screen.c`, `widgets/util.{c,h}`, `widgets/bolt.c`, `widgets/peripheral_status.{c,h}` from `zmkfirmware/zmk` tag `v0.3`, `app/boards/shields/nice_view/`, into `src/screen/`. Add an MIT header note with the source URL. In `peripheral_status.c`, delete the `art` image and `LV_IMG_DECLARE`s and call `zmk_widget_udo_init(widget->obj)` instead.
- [ ] **Step 2:** Implement the udo widget (`udo_widget.c`).
  - Layout: one 68×68 canvas aligned `LV_ALIGN_TOP_RIGHT, -68, 0`, next to the top canvas, with its own `cbuf`. Draw it portrait, then call `rotate_canvas()`.
  - Drawing, by view kind:
    - Running: `▶` (`LV_SYMBOL_PLAY`) plus the time in `montserrat_16` at y=0, and the name in `montserrat_14` at y=24, wrapped to a 68 px width.
    - Idle: `"idle"`.
    - Unknown: `"udo ?"`.
  - State: last status, `received_at = k_uptime_get()`, `have_data`.
  - Updates: a `ZMK_DISPLAY_WIDGET_LISTENER` on `udo_status_changed` redraws immediately. A `k_work_delayable` on `zmk_display_work_q()` reschedules itself every 60 s and redraws with `extra_s = age_ms/1000`. That covers both the timer and the stale check.
- [ ] **Step 3:** Implement `peripheral_service.c` with `BT_GATT_SERVICE_DEFINE`, using the properties and permissions from Global Constraints.
- [ ] **Step 4:** Uncomment `-DCONFIG_NICE_VIEW_WIDGET_STATUS=n` on the right half. Run `./build.sh right`. Expected: `ok` (no duplicate `zmk_display_status_screen` link error).
- [ ] **Step 5 (manual M1):** Flash the right half. Expected: the top canvas still shows battery and connection status, and the udo area shows `udo ?`.
- [ ] **Step 6: Commit** `feat(udo-status): peripheral GATT service and udo screen`.

### Task 5: Central relay + end-to-end

**Files:**
- Create: `src/central_relay.c`
- Modify: `readme.md`

**Interfaces:**
- Consumes: `raw_hid_received_event` (`<raw_hid/events.h>`: `uint8_t *data; uint8_t length;`), `udo_packet_decode`, the UUIDs.
- Produces: forwarding of valid packets to the peripheral. No new symbols.

- [ ] **Step 1:** Implement `central_relay.c`.
  - Raw HID: a `ZMK_LISTENER` on `raw_hid_received_event` checks the packet with `udo_packet_decode`, ignores invalid ones, and returns `ZMK_EV_EVENT_BUBBLE`. When the link to the right half is up and `chr_handle != 0`, it sends the first 20 bytes with `bt_gatt_write_without_response(conn, chr_handle, data, 20, false)`.
  - Connection callbacks (`BT_CONN_CB_DEFINE`):
    - `security_changed`: if role is `BT_CONN_ROLE_CENTRAL` and level ≥ `BT_SECURITY_L2`, store the conn ref and schedule discovery after 1 s.
    - `disconnected`: drop the ref and set `chr_handle=0`.
  - Discovery: `bt_gatt_discover` with type `BT_GATT_DISCOVER_CHARACTERISTIC` and `uuid=UDO_CHR_UUID`. Retry up to 3 times, 2 s apart, if it errors or finds nothing.
- [ ] **Step 2:** Run `./build.sh`. Expected: all targets `ok`.
- [ ] **Step 3:** Flash both halves. Install the daemon:
  - `python3 -m venv host/.venv && host/.venv/bin/pip install -r host/requirements.txt`
  - Render the plist into `~/Library/LaunchAgents/`.
  - `launchctl bootstrap gui/$(id -u) …`
- [ ] **Step 4 (manual checks):**
  - **M2:** `udo start <task>` → the right screen shows the name and `0:00` within 10 s. After `udo stop` it shows `idle` within 10 s. The timer advances once a minute.
  - **M3:** Power-cycle the right half while a task runs → the correct task is back within 60 s.
  - **M4:** Kill the daemon → `udo ?` after 3 min or less. Typing latency and range feel unchanged over 1 day of use.
  - **M5:** Let the keyboard deep-sleep, then wake it → the daemon log shows reconnect, and the screen updates within 60 s.
- [ ] **Step 5:** In `readme.md`, add a section "udo on the right screen": architecture in 3 lines, daemon install/uninstall commands, and the packet format reference. Add `host/.venv` to `.gitignore`.
- [ ] **Step 6: Commit** `feat(udo-status): relay raw HID packets from central to peripheral`.
