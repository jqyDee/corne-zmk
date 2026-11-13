/*
 * Central (left half): forward udo packets from the host (raw HID) to the
 * peripheral's udo GATT characteristic (see peripheral_service.c).
 *
 * The characteristic is discovered a few seconds after the split connection
 * comes up (ZMK runs its own discovery first) and forgotten on disconnect.
 * The host re-sends every 60 s, so nothing is cached here.
 */
#include <string.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <raw_hid/events.h>

#include <udo_status/packet.h>
#include <udo_status/uuids.h>

#define DISCOVER_DELAY K_SECONDS(3)
#define DISCOVER_RETRY K_SECONDS(2)
#define DISCOVER_ATTEMPTS 5

static struct bt_conn *peripheral_conn;
static uint16_t chr_handle;
static int attempts_left;
static struct bt_gatt_discover_params discover_params;

static void discover_work_cb(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(discover_work, discover_work_cb);

static uint8_t pending[UDO_PACKET_LEN];
static void send_work_cb(struct k_work *work);
static K_WORK_DEFINE(send_work, send_work_cb);

static void retry_discovery(void) {
    if (peripheral_conn && --attempts_left > 0) {
        k_work_reschedule(&discover_work, DISCOVER_RETRY);
    } else if (peripheral_conn) {
        LOG_WRN("udo: characteristic not found on peripheral");
    }
}

static uint8_t discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                           struct bt_gatt_discover_params *params) {
    if (!attr) {
        if (!chr_handle) {
            retry_discovery();
        }
        return BT_GATT_ITER_STOP;
    }
    const struct bt_gatt_chrc *chrc = attr->user_data;
    chr_handle = chrc->value_handle;
    LOG_INF("udo: characteristic at handle %u", chr_handle);
    return BT_GATT_ITER_STOP;
}

static void discover_work_cb(struct k_work *work) {
    if (!peripheral_conn || chr_handle) {
        return;
    }
    discover_params = (struct bt_gatt_discover_params){
        .uuid = UDO_CHR_UUID,
        .func = discover_cb,
        .start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
        .end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
        .type = BT_GATT_DISCOVER_CHARACTERISTIC,
    };
    int err = bt_gatt_discover(peripheral_conn, &discover_params);
    if (err) {
        LOG_DBG("udo: discover failed (%d), retrying", err);
        retry_discovery();
    }
}

static void connected(struct bt_conn *conn, uint8_t err) {
    struct bt_conn_info info;
    if (err || bt_conn_get_info(conn, &info) || info.role != BT_CONN_ROLE_CENTRAL ||
        peripheral_conn) {
        return; /* only our link to the peripheral, not the host connections */
    }
    peripheral_conn = bt_conn_ref(conn);
    chr_handle = 0;
    attempts_left = DISCOVER_ATTEMPTS;
    k_work_reschedule(&discover_work, DISCOVER_DELAY);
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    if (conn != peripheral_conn) {
        return;
    }
    k_work_cancel_delayable(&discover_work);
    bt_conn_unref(peripheral_conn);
    peripheral_conn = NULL;
    chr_handle = 0;
}

BT_CONN_CB_DEFINE(udo_conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

static void send_work_cb(struct k_work *work) {
    if (!peripheral_conn || !chr_handle) {
        return;
    }
    int err = bt_gatt_write_without_response(peripheral_conn, chr_handle, pending,
                                             sizeof(pending), false);
    if (err) {
        LOG_DBG("udo: write failed (%d)", err);
    }
}

static int raw_hid_listener(const zmk_event_t *eh) {
    const struct raw_hid_received_event *ev = as_raw_hid_received_event(eh);
    struct udo_status ignored;
    if (ev && udo_packet_decode(ev->data, ev->length, &ignored) == 0) {
        memcpy(pending, ev->data, UDO_PACKET_LEN);
        k_work_submit(&send_work);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(udo_central_relay, raw_hid_listener);
ZMK_SUBSCRIPTION(udo_central_relay, raw_hid_received_event);
