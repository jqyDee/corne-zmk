/* Host test: cc, no Zephyr. Run via tests/run.sh */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <udo_status/packet.h>
#include "../src/screen/udo_view.h"

static const struct udo_status RUNNING = {.running = true, .elapsed_s = 5025, .name = "lab 3"};

static void test_view_running_format(void) {
    struct udo_view v;
    udo_view_build(&RUNNING, true, 0, 0, &v);
    assert(v.kind == UDO_VIEW_RUNNING);
    assert(strcmp(v.time, "1:23") == 0);
    assert(strcmp(v.name, "lab 3") == 0);

    udo_view_build(&RUNNING, true, 0, 120, &v);
    assert(strcmp(v.time, "1:25") == 0);

    struct udo_status ten = RUNNING;
    ten.elapsed_s = 36000;
    udo_view_build(&ten, true, 0, 0, &v);
    assert(strcmp(v.time, "10:00") == 0);
}

static void test_view_keeps_24_char_name(void) {
    struct udo_status s = RUNNING;
    strcpy(s.name, "abcdefghijklmnopqrstuvwx");
    struct udo_view v;
    udo_view_build(&s, true, 0, 0, &v);
    assert(strcmp(v.name, "abcdefghijklmnopqrstuvwx") == 0);
}

static void test_view_idle(void) {
    struct udo_status idle = {.running = false};
    struct udo_view v;
    udo_view_build(&idle, true, 0, 0, &v);
    assert(v.kind == UDO_VIEW_IDLE);
}

static void test_stale_after_timeout(void) {
    struct udo_view v;
    udo_view_build(&RUNNING, true, 180001, 0, &v);
    assert(v.kind == UDO_VIEW_UNKNOWN);
    udo_view_build(&RUNNING, true, 179999, 0, &v);
    assert(v.kind == UDO_VIEW_RUNNING);
    udo_view_build(&RUNNING, false, 0, 0, &v);
    assert(v.kind == UDO_VIEW_UNKNOWN);
}

static void test_view_time_no_overflow(void) {
    struct udo_status huge = RUNNING;
    huge.elapsed_s = UINT32_MAX;
    struct udo_view v;
    udo_view_build(&huge, true, 0, UINT32_MAX, &v);
    assert(strlen(v.time) < sizeof(v.time));
}

static void test_next_tick(void) {
    struct udo_status idle = {.running = false};
    /* running: next minute change of the shown timer (5025 s -> 15 s) */
    assert(udo_view_next_tick_s(&RUNNING, true, 0) == 15);
    /* local count included: 5025 s + 10 s -> 5 s to the next minute */
    assert(udo_view_next_tick_s(&RUNNING, true, 10000) == 5);
    /* stale comes first: 180 s limit, 170 s old -> 11 s (just past it) */
    struct udo_status on_minute = RUNNING;
    on_minute.elapsed_s = 3600 - 170;
    assert(udo_view_next_tick_s(&on_minute, true, 170000) == 11);
    /* idle: wake up exactly when it turns stale */
    assert(udo_view_next_tick_s(&idle, true, 150000) == 31);
    /* no data or already stale: nothing changes on its own, slow tick */
    assert(udo_view_next_tick_s(&idle, false, 0) == 60);
    assert(udo_view_next_tick_s(&RUNNING, true, 200000) == 60);
}

int main(void) {
    test_view_running_format();
    test_view_keeps_24_char_name();
    test_view_idle();
    test_stale_after_timeout();
    test_view_time_no_overflow();
    test_next_tick();
    puts("test_view: OK");
    return 0;
}
