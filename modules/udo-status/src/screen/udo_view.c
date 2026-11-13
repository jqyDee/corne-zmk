#include <stdio.h>
#include <string.h>

#include "udo_view.h"

void udo_view_build(const struct udo_status *s, bool have_data, int64_t age_ms,
                    uint32_t extra_s, struct udo_view *out) {
    memset(out, 0, sizeof(*out));
    if (!have_data || age_ms > UDO_STALE_MS) {
        out->kind = UDO_VIEW_UNKNOWN;
        return;
    }
    if (!s->running) {
        out->kind = UDO_VIEW_IDLE;
        return;
    }
    out->kind = UDO_VIEW_RUNNING;
    uint64_t total = (uint64_t)s->elapsed_s + extra_s;
    unsigned hours = total / 3600 > 999 ? 999 : (unsigned)(total / 3600);
    unsigned minutes = (unsigned)(total / 60 % 60);
    snprintf(out->time, sizeof(out->time), "%u:%02u", hours, minutes);
    memcpy(out->name, s->name, sizeof(out->name));
    out->name[UDO_NAME_LEN] = '\0';
}

uint32_t udo_view_next_tick_s(const struct udo_status *s, bool have_data, int64_t age_ms) {
    if (!have_data || age_ms > UDO_STALE_MS) {
        return 60;
    }
    uint32_t next = 60;
    if (s->running) {
        uint64_t total = (uint64_t)s->elapsed_s + (uint64_t)(age_ms / 1000);
        next = 60 - (uint32_t)(total % 60);
    }
    uint32_t until_stale = (uint32_t)((UDO_STALE_MS - age_ms) / 1000) + 1;
    return until_stale < next ? until_stale : next;
}
