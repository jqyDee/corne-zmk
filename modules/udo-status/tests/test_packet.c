/* Host test: cc, no Zephyr. Run via tests/run.sh */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <udo_status/packet.h>

/* Shared with the daemon's test_udo_keyboard.py (dotfiles, ~/.config/udo-keyboard):
 * running, 5025 s, "lab 3" (32 bytes) */
static const uint8_t VECTOR[UDO_PACKET_MAX_LEN] = {0x55, 0x01, 0x01, 0x00, 0xa1, 0x13, 0x00,
                                               0x00, 'l',  'a',  'b',  ' ',  '3'};

static void test_decode_shared_vector(void) {
    struct udo_status s;
    assert(udo_packet_decode(VECTOR, sizeof(VECTOR), &s) == 0);
    assert(s.running);
    assert(s.elapsed_s == 5025);
    assert(strcmp(s.name, "lab 3") == 0);
}

static void test_decode_rejects_bad_magic(void) {
    struct udo_status s;
    uint8_t buf[32] = {0};
    memcpy(buf, VECTOR, sizeof(VECTOR));

    buf[0] = 0x00;
    assert(udo_packet_decode(buf, UDO_PACKET_MIN_LEN, &s) == -EINVAL);
    buf[0] = 0x55;
    buf[1] = 0x02;
    assert(udo_packet_decode(buf, UDO_PACKET_MIN_LEN, &s) == -EINVAL);
    buf[1] = 0x01;
    assert(udo_packet_decode(buf, 19, &s) == -EINVAL);
    assert(udo_packet_decode(buf, 20, &s) == 0); /* short form (small MTU) */
    assert(udo_packet_decode(buf, 32, &s) == 0); /* full raw HID report */
}

static void test_decode_name_full_24_no_nul(void) {
    struct udo_status s;
    uint8_t buf[UDO_PACKET_MAX_LEN];
    memcpy(buf, VECTOR, sizeof(VECTOR));
    memcpy(buf + 8, "abcdefghijklmnopqrstuvwx", 24);
    assert(udo_packet_decode(buf, sizeof(buf), &s) == 0);
    assert(strcmp(s.name, "abcdefghijklmnopqrstuvwx") == 0);
}

static void test_decode_short_form_cuts_name_at_12(void) {
    struct udo_status s;
    uint8_t buf[UDO_PACKET_MAX_LEN];
    memcpy(buf, VECTOR, sizeof(VECTOR));
    memcpy(buf + 8, "abcdefghijklmnopqrstuvwx", 24);
    assert(udo_packet_decode(buf, UDO_PACKET_MIN_LEN, &s) == 0);
    assert(strcmp(s.name, "abcdefghijkl") == 0);
}

static void test_decode_ignores_bytes_past_max(void) {
    struct udo_status s;
    uint8_t buf[UDO_PACKET_MAX_LEN + 8];
    memset(buf, 'z', sizeof(buf));
    memcpy(buf, VECTOR, sizeof(VECTOR));
    assert(udo_packet_decode(buf, sizeof(buf), &s) == 0);
    assert(strcmp(s.name, "lab 3") == 0);
}

static void test_decode_idle(void) {
    struct udo_status s;
    uint8_t buf[UDO_PACKET_MAX_LEN] = {0x55, 0x01, 0x00};
    assert(udo_packet_decode(buf, sizeof(buf), &s) == 0);
    assert(!s.running);
    assert(s.name[0] == '\0');
}

int main(void) {
    test_decode_shared_vector();
    test_decode_rejects_bad_magic();
    test_decode_name_full_24_no_nul();
    test_decode_short_form_cuts_name_at_12();
    test_decode_ignores_bytes_past_max();
    test_decode_idle();
    puts("test_packet: OK");
    return 0;
}
