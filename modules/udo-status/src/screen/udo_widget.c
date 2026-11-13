/*
 * udo area of the right nice!view: task + timer, "idle" or "udo ?".
 *
 * Packets arrive as udo_status_changed events (see peripheral_service.c). The
 * timer counts locally from the last packet and redraws on each minute change;
 * a 60 s tick also turns the view to "udo ?" once data is stale.
 */
#include <string.h>

#include <zephyr/kernel.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>

#include <udo_status/event.h>

#include "udo_view.h"
#include "udo_widget.h"
#include "util.h"

struct udo_widget_state {
    struct udo_status status;
    bool have_data;
    int64_t received_at;
};

static lv_obj_t *canvas;
static lv_color_t cbuf[CANVAS_SIZE * CANVAS_SIZE];
static struct udo_widget_state shown; /* display thread only */
static struct udo_view drawn;
static bool drawn_valid;

static void tick_cb(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(tick_work, tick_cb);

static void draw_view(const struct udo_view *v) {
    lv_draw_rect_dsc_t bg;
    init_rect_dsc(&bg, LVGL_BACKGROUND);
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &bg);

    lv_draw_label_dsc_t big, small;
    init_label_dsc(&big, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_LEFT);
    init_label_dsc(&small, LVGL_FOREGROUND, &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);

    switch (v->kind) {
    case UDO_VIEW_RUNNING: {
        char line[16];
        snprintf(line, sizeof(line), LV_SYMBOL_PLAY " %s", v->time);
        lv_canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &big, line);
        lv_canvas_draw_text(canvas, 0, 24, CANVAS_SIZE, &small, v->name);
        break;
    }
    case UDO_VIEW_IDLE:
        lv_canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &big, LV_SYMBOL_PAUSE " idle");
        break;
    default:
        lv_canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &small, "udo ?");
        break;
    }
    rotate_canvas(canvas, cbuf);
}

/* Redraw if the view changed; return seconds until the next minute change. */
static uint32_t refresh(void) {
    int64_t age_ms = shown.have_data ? k_uptime_get() - shown.received_at : 0;
    uint32_t extra_s = (uint32_t)(age_ms / 1000);
    struct udo_view v;
    udo_view_build(&shown.status, shown.have_data, age_ms, extra_s, &v);

    if (!drawn_valid || memcmp(&v, &drawn, sizeof(v)) != 0) {
        draw_view(&v);
        drawn = v;
        drawn_valid = true;
    }
    if (v.kind == UDO_VIEW_RUNNING) {
        return 60 - (shown.status.elapsed_s + extra_s) % 60;
    }
    return 60;
}

static void tick_cb(struct k_work *work) {
    uint32_t next_s = refresh();
    k_work_schedule_for_queue(zmk_display_work_q(), &tick_work, K_SECONDS(next_s));
}

static void udo_update_cb(struct udo_widget_state state) {
    if (!state.have_data) {
        return; /* init call before any packet */
    }
    shown = state;
    uint32_t next_s = refresh();
    k_work_reschedule_for_queue(zmk_display_work_q(), &tick_work, K_SECONDS(next_s));
}

static struct udo_widget_state udo_get_state(const zmk_event_t *eh) {
    const struct udo_status_changed *ev = eh ? as_udo_status_changed(eh) : NULL;
    if (!ev) {
        return (struct udo_widget_state){0};
    }
    return (struct udo_widget_state){
        .status = ev->status, .have_data = true, .received_at = k_uptime_get()};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_udo, struct udo_widget_state, udo_update_cb, udo_get_state)
ZMK_SUBSCRIPTION(widget_udo, udo_status_changed);

void zmk_widget_udo_init(lv_obj_t *parent) {
    canvas = lv_canvas_create(parent);
    lv_obj_align(canvas, LV_ALIGN_TOP_RIGHT, -CANVAS_SIZE, 0);
    lv_canvas_set_buffer(canvas, cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    widget_udo_init();
    refresh(); /* "udo ?" until the first packet */
    k_work_schedule_for_queue(zmk_display_work_q(), &tick_work, K_SECONDS(60));
}
