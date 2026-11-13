/* When does the menu need its next frame? Ticks are SDL_GetTicks()-style ms. */
#include "test.h"
#include "redraw.h"

static void test_still_menu_waits_for_next_event(void)
{
    Redraw r;
    redraw_begin(&r, 5000);
    CHECK_INT(redraw_wait_ms(&r, 5016), REDRAW_WAIT_FOREVER);
}

static void test_playing_preview_draws_next_frame_now(void)
{
    Redraw r;
    redraw_begin(&r, 5000);
    redraw_animating(&r);
    CHECK_INT(redraw_wait_ms(&r, 5016), 0);
}

static void test_waits_until_hovered_preview_is_due_to_start(void)
{
    Redraw r;
    redraw_begin(&r, 5000);
    redraw_at(&r, 5400); /* hover started at 5000, preview delay 400 ms */
    CHECK_INT(redraw_wait_ms(&r, 5100), 300);
}

static void test_earliest_pending_change_wins(void)
{
    Redraw r;
    redraw_begin(&r, 5000);
    redraw_at(&r, 8500); /* status message disappears */
    redraw_at(&r, 5260); /* hover grace after leaving a tile ends */
    redraw_at(&r, 6000);
    CHECK_INT(redraw_wait_ms(&r, 5010), 250);
}

static void test_change_already_drawn_does_not_wake_menu(void)
{
    Redraw r;
    redraw_begin(&r, 9000);
    redraw_at(&r, 8500); /* status expired before the last frame was drawn */
    redraw_at(&r, 9000);
    CHECK_INT(redraw_wait_ms(&r, 9020), REDRAW_WAIT_FOREVER);
}

static void test_overdue_change_after_last_frame_draws_now(void)
{
    Redraw r;
    redraw_begin(&r, 5000);
    redraw_at(&r, 5400);
    CHECK_INT(redraw_wait_ms(&r, 5450), 0);
}

static void test_pending_change_survives_tick_wraparound(void)
{
    Redraw r;
    redraw_begin(&r, 0xFFFFFF00u);
    redraw_at(&r, 0x00000100u); /* 512 ms later, after the 32-bit tick wraps */
    CHECK_INT(redraw_wait_ms(&r, 0xFFFFFFF0u), 272);
}

int main(void)
{
    RUN_TEST(test_still_menu_waits_for_next_event);
    RUN_TEST(test_playing_preview_draws_next_frame_now);
    RUN_TEST(test_waits_until_hovered_preview_is_due_to_start);
    RUN_TEST(test_earliest_pending_change_wins);
    RUN_TEST(test_change_already_drawn_does_not_wake_menu);
    RUN_TEST(test_overdue_change_after_last_frame_draws_now);
    RUN_TEST(test_pending_change_survives_tick_wraparound);
    return TEST_REPORT();
}
