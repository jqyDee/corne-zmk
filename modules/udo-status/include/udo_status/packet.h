/*
 * udo status packet, sent host -> central (raw HID) -> peripheral (GATT).
 *
 * Little-endian, 20..32 bytes: [0]=0x55 magic, [1]=0x01 version, [2] state
 * (0 idle, 1 running), [3] reserved, [4..7] elapsed seconds (uint32),
 * [8..] task name, ASCII, NUL-padded (no terminator required), as many bytes
 * as the packet carries: 24 in a full packet, 12 in the 20-byte short form
 * the central falls back to when the split link's ATT MTU is the default 23.
 *
 * Pure C: compiles on the host for tests (tests/run.sh) as well as in ZMK.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UDO_PACKET_MAGIC 0x55
#define UDO_PACKET_VERSION 0x01
#define UDO_PACKET_MIN_LEN 20
#define UDO_PACKET_MAX_LEN 32
#define UDO_NAME_OFFSET 8
#define UDO_NAME_LEN (UDO_PACKET_MAX_LEN - UDO_NAME_OFFSET)

struct udo_status {
    bool running;
    uint32_t elapsed_s;
    char name[UDO_NAME_LEN + 1];
};

/* Decode buf (UDO_PACKET_MIN_LEN or more bytes; past UDO_PACKET_MAX_LEN ignored).
 * Returns 0, or -EINVAL for short / foreign packets. */
int udo_packet_decode(const uint8_t *buf, size_t len, struct udo_status *out);
