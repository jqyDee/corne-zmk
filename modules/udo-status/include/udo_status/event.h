#pragma once

#include <zmk/event_manager.h>
#include <udo_status/packet.h>

/* Raised on the peripheral when a new udo packet arrived from the central. */
struct udo_status_changed {
    struct udo_status status;
};

ZMK_EVENT_DECLARE(udo_status_changed);
