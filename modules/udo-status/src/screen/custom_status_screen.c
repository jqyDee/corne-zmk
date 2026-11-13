/*
 * Status screen of the peripheral (right half) when udo-status is enabled:
 * the stock battery/connection canvas plus the udo widget instead of the art.
 * Based on ZMK v0.3 app/boards/shields/nice_view/custom_status_screen.c (MIT).
 */
#include "peripheral_status.h"

static struct zmk_widget_status status_widget;

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);
    zmk_widget_status_init(&status_widget, screen);
    lv_obj_align(zmk_widget_status_obj(&status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
    return screen;
}
