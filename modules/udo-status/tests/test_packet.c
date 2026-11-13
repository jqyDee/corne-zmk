/* Host test: cc, no Zephyr. Run via tests/run.sh */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <udo_status/packet.h>

/* Shared with host/test_udo_keyboard.py: running, 5025 s, "lab 3" */
static const uint8_t VECTOR[UDO_PACKET_LEN] = {0x55, 0x01, 0x01, 0x00, 0xa1, 0x13, 0x00,
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
    assert(udo_packet_decode(buf, UDO_PACKET_LEN, &s) == -EINVAL);
    buf[0] = 0x55;
    buf[1] = 0x02;
    assert(udo_packet_decode(buf, UDO_PACKET_LEN, &s) == -EINVAL);
    buf[1] = 0x01;
    assert(udo_packet_decode(buf, 19, &s) == -EINVAL);
    assert(udo_packet_decode(buf, 32, &s) == 0); /* padded HID report */
}

static void test_decode_name_full_12_no_nul(void) {
    struct udo_status s;
    uint8_t buf[UDO_PACKET_LEN];
    memcpy(buf, VECTOR, sizeof(VECTOR));
    memcpy(buf + 8, "abcdefghijkl", 12);
    assert(udo_packet_decode(buf, sizeof(buf), &s) == 0);
    assert(strlen(s.name) == 12);
}

static void test_decode_idle(void) {
    struct udo_status s;
    uint8_t buf[UDO_PACKET_LEN] = {0x55, 0x01, 0x00};
    assert(udo_packet_decode(buf, sizeof(buf), &s) == 0);
    assert(!s.running);
    assert(s.name[0] == '\0');
}

int main(void) {
    test_decode_shared_vector();
    test_decode_rejects_bad_magic();
    test_decode_name_full_12_no_nul();
    test_decode_idle();
    puts("test_packet: OK");
    return 0;
}
