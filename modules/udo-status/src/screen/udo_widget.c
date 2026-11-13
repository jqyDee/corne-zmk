/*
 * udo area of the right nice!view: task + timer, "idle" or "udo ?".
 *
 * Packets arrive as udo_status_changed events (see peripheral_service.c). The
 * timer counts locally from the last packet and redraws on each minute change;
 * a 60 s tick also turns the view to "udo ?" once data is stale.
 *
 * Layout: the area is 68 wide x 140 tall as you look at the keyboard,
 * starting right under the battery/connection row of the top canvas. LVGL
 * draws it upright into a hidden portrait canvas, which is then rotated by
 * 90 degrees (like util.c's rotate_canvas, but not square) into the visible
 * landscape canvas, because the nice!view is mounted sideways.
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

#define AREA_W CANVAS_SIZE /* portrait width */
#define AREA_H 140         /* portrait height: 160 minus the 20 px status row */

static lv_obj_t *canvas;  /* portrait, hidden, drawn by LVGL */
static lv_obj_t *display; /* landscape, visible */
static lv_color_t cbuf[AREA_W * AREA_H];
static lv_color_t dbuf[AREA_H * AREA_W];
static struct udo_widget_state shown; /* display thread only */
static struct udo_view drawn;
static bool drawn_valid;

static void tick_cb(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(tick_work, tick_cb);

/* Portrait (x, y) -> landscape (AREA_H - 1 - y, x): 90 degrees clockwise. */
static void rotate_into_display(void) {
    for (int y = 0; y < AREA_H; y++) {
        for (int x = 0; x < AREA_W; x++) {
            dbuf[x * AREA_H + (AREA_H - 1 - y)] = cbuf[y * AREA_W + x];
        }
    }
    lv_obj_invalidate(display);
}

static void draw_view(const struct udo_view *v) {
    lv_draw_rect_dsc_t bg;
    init_rect_dsc(&bg, LVGL_BACKGROUND);
    lv_canvas_draw_rect(canvas, 0, 0, AREA_W, AREA_H, &bg);

    lv_draw_label_dsc_t big, small;
    init_label_dsc(&big, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_LEFT);
    init_label_dsc(&small, LVGL_FOREGROUND, &lv_font_montserrat_14, LV_TEXT_ALIGN_LEFT);

    switch (v->kind) {
    case UDO_VIEW_RUNNING: {
        char line[16];
        snprintf(line, sizeof(line), LV_SYMBOL_PLAY " %s", v->time);
        lv_canvas_draw_text(canvas, 0, 0, AREA_W, &big, line);
        lv_canvas_draw_text(canvas, 0, 22, AREA_W, &small, v->name);
        break;
    }
    case UDO_VIEW_IDLE:
        lv_canvas_draw_text(canvas, 0, 0, AREA_W, &big, LV_SYMBOL_PAUSE " idle");
        break;
    default:
        lv_canvas_draw_text(canvas, 0, 0, AREA_W, &small, "udo ?");
        break;
    }
    rotate_into_display();
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
    lv_canvas_set_buffer(canvas, cbuf, AREA_W, AREA_H, LV_IMG_CF_TRUE_COLOR);
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);

    /* Created after the top canvas, so it covers that canvas' empty part. */
    display = lv_canvas_create(parent);
    lv_canvas_set_buffer(display, dbuf, AREA_H, AREA_W, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(display, LV_ALIGN_TOP_LEFT, 0, 0);

    widget_udo_init();
    refresh(); /* "udo ?" until the first packet */
    k_work_schedule_for_queue(zmk_display_work_q(), &tick_work, K_SECONDS(60));
}
