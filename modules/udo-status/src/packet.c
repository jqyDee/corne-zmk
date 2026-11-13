#include <errno.h>
#include <string.h>

#include <udo_status/packet.h>

int udo_packet_decode(const uint8_t *buf, size_t len, struct udo_status *out) {
    if (len < UDO_PACKET_LEN || buf[0] != UDO_PACKET_MAGIC || buf[1] != UDO_PACKET_VERSION) {
        return -EINVAL;
    }
    out->running = buf[2] == 1;
    out->elapsed_s = (uint32_t)buf[4] | (uint32_t)buf[5] << 8 | (uint32_t)buf[6] << 16 |
                     (uint32_t)buf[7] << 24;
    memcpy(out->name, buf + 8, UDO_NAME_LEN);
    out->name[UDO_NAME_LEN] = '\0';
    return 0;
}
