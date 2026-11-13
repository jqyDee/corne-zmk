/* TEMPORARY Task 1 spike: send every received raw HID report straight back,
 * so host/probe.py can verify the host -> central path without a serial log. */
#include <zmk/event_manager.h>
#include <raw_hid/events.h>

static int echo_listener(const zmk_event_t *eh) {
    struct raw_hid_received_event *ev = as_raw_hid_received_event(eh);
    if (ev) {
        raise_raw_hid_sent_event((struct raw_hid_sent_event){.data = ev->data, .length = ev->length});
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(udo_spike_echo, echo_listener);
ZMK_SUBSCRIPTION(udo_spike_echo, raw_hid_received_event);
