/* What the udo area of the right screen shows. Pure C, host-tested. */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <udo_status/packet.h>

/* No packet for this long: the daemon is gone or the host is elsewhere. */
#define UDO_STALE_MS 180000

enum udo_view_kind { UDO_VIEW_UNKNOWN, UDO_VIEW_IDLE, UDO_VIEW_RUNNING };

struct udo_view {
    enum udo_view_kind kind;
    char time[8]; /* "H:MM", hours capped at 999 */
    char name[UDO_NAME_LEN + 1];
};

/* s: last received status; have_data: anything received yet; age_ms: time
 * since it arrived; extra_s: seconds to add to s->elapsed_s (local count). */
void udo_view_build(const struct udo_status *s, bool have_data, int64_t age_ms,
                    uint32_t extra_s, struct udo_view *out);

/* Seconds until the view next changes by itself: the shown minute ticks over
 * (running) or the data turns stale, whichever is first; 60 if neither. */
uint32_t udo_view_next_tick_s(const struct udo_status *s, bool have_data, int64_t age_ms);
