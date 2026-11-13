/*
 * GATT service on the peripheral (right half). The central writes udo packets
 * to it over the split connection (see central_relay.c); each valid one is
 * raised as a udo_status_changed event for the screen.
 */
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <udo_status/event.h>
#include <udo_status/packet.h>
#include <udo_status/uuids.h>

static ssize_t write_udo(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
                         uint16_t len, uint16_t offset, uint8_t flags) {
    struct udo_status status;
    if (offset != 0 || udo_packet_decode(buf, len, &status) != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }
    raise_udo_status_changed((struct udo_status_changed){.status = status});
    return len;
}

BT_GATT_SERVICE_DEFINE(udo_svc, BT_GATT_PRIMARY_SERVICE(UDO_SVC_UUID),
                       BT_GATT_CHARACTERISTIC(UDO_CHR_UUID, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                                              BT_GATT_PERM_WRITE_ENCRYPT, NULL, write_udo, NULL));
