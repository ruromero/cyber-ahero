/* Attract mode timing. Ticks are SDL_GetTicks()-style ms. */
#include "test.h"
#include "attract.h"

#define MIN 60000u

static void test_starts_after_idle_minutes_with_first_entry(void)
{
    Attract a;
    attract_init(&a, 3, 1000);
    CHECK_INT(attract_update(&a, 1000 + 3 * MIN - 1, 5), ATTRACT_NONE);
    CHECK_INT(attract_active(&a), 0);
    CHECK_INT(attract_update(&a, 1000 + 3 * MIN, 5), ATTRACT_SHOW);
    CHECK_INT(attract_active(&a), 1);
    CHECK_INT(attract_entry(&a), 0);
}

static void test_zero_minutes_never_starts(void)
{
    Attract a;
    attract_init(&a, 0, 1000);
    CHECK_INT(attract_update(&a, 1000 + 600 * MIN, 5), ATTRACT_NONE);
    CHECK_INT(attract_active(&a), 0);
}

static void test_one_minute_setting_starts_after_one_minute(void)
{
    Attract a;
    attract_init(&a, 1, 0);
    CHECK_INT(attract_update(&a, MIN - 1, 5), ATTRACT_NONE);
    CHECK_INT(attract_update(&a, MIN, 5), ATTRACT_SHOW);
}

static void test_input_in_the_menu_restarts_idle_time_and_is_not_consumed(void)
{
    Attract a;
    attract_init(&a, 3, 0);
    CHECK_INT(attract_input(&a, 2 * MIN), 0);
    CHECK_INT(attract_update(&a, 3 * MIN, 5), ATTRACT_NONE);
    CHECK_INT(attract_update(&a, 5 * MIN, 5), ATTRACT_SHOW);
}

static void test_input_during_attract_returns_to_menu_and_is_consumed(void)
{
    Attract a;
    attract_init(&a, 3, 0);
    attract_update(&a, 3 * MIN, 5);
    CHECK_INT(attract_input(&a, 3 * MIN + 500), 1);
    CHECK_INT(attract_active(&a), 0);
    /* the next input after the wake-up guard is a normal menu input */
    CHECK_INT(attract_input(&a, 3 * MIN + 500 + ATTRACT_WAKE_GUARD_MS), 0);
    /* and the idle time counts again from the wake-up */
    CHECK_INT(attract_update(&a, 6 * MIN, 5), ATTRACT_NONE);
    CHECK_INT(attract_update(&a, 6 * MIN + 500 + ATTRACT_WAKE_GUARD_MS, 5), ATTRACT_SHOW);
}

static void test_empty_library_never_starts(void)
{
    Attract a;
    attract_init(&a, 1, 0);
    CHECK_INT(attract_update(&a, 10 * MIN, 0), ATTRACT_NONE);
    CHECK_INT(attract_active(&a), 0);
}

static void test_idle_time_survives_tick_wraparound(void)
{
    Attract a;
    attract_init(&a, 1, 0xFFFFFFFFu - 1000);
    CHECK_INT(attract_update(&a, 1000, 5), ATTRACT_NONE);
    CHECK_INT(attract_update(&a, MIN - 1001, 5), ATTRACT_SHOW);
}

/* Starts Attract mode at tick 3 min with entry_count Entries. */
static uint32_t start(Attract *a, int entry_count)
{
    attract_init(a, 3, 0);
    attract_update(a, 3 * MIN, entry_count);
    return 3 * MIN;
}

static void test_entry_with_preview_shows_for_twenty_seconds(void)
{
    Attract a;
    uint32_t t = start(&a, 5);
    attract_shown(&a, 1);
    CHECK_INT(attract_update(&a, t + 19999, 5), ATTRACT_NONE);
    CHECK_INT(attract_update(&a, t + 20000, 5), ATTRACT_SHOW);
    CHECK_INT(attract_entry(&a), 1);
}

static void test_cover_only_entry_shows_for_eight_seconds(void)
{
    Attract a;
    uint32_t t = start(&a, 5);
    attract_shown(&a, 0);
    CHECK_INT(attract_update(&a, t + 7999, 5), ATTRACT_NONE);
    CHECK_INT(attract_update(&a, t + 8000, 5), ATTRACT_SHOW);
    CHECK_INT(attract_entry(&a), 1);
}

static void test_preview_ending_early_moves_to_next_entry(void)
{
    Attract a;
    uint32_t t = start(&a, 5);
    attract_shown(&a, 1);
    CHECK_INT(attract_update(&a, t + 5000, 5), ATTRACT_NONE);
    attract_preview_ended(&a);
    CHECK_INT(attract_update(&a, t + 5016, 5), ATTRACT_SHOW);
    CHECK_INT(attract_entry(&a), 1);
    /* the next Entry gets its own full time */
    attract_shown(&a, 1);
    CHECK_INT(attract_update(&a, t + 5016 + 19999, 5), ATTRACT_NONE);
    CHECK_INT(attract_update(&a, t + 5016 + 20000, 5), ATTRACT_SHOW);
}

static void test_cycles_through_entries_in_library_order_and_wraps(void)
{
    Attract a;
    uint32_t t = start(&a, 3);
    int seen[5];
    for (int i = 0; i < 5; i++) {
        seen[i] = attract_entry(&a);
        attract_shown(&a, 0);
        t += ATTRACT_COVER_MS;
        CHECK_INT(attract_update(&a, t, 3), ATTRACT_SHOW);
    }
    CHECK_INT(seen[0], 0);
    CHECK_INT(seen[1], 1);
    CHECK_INT(seen[2], 2);
    CHECK_INT(seen[3], 0);
    CHECK_INT(seen[4], 1);
}

static void test_restarts_from_first_entry_each_time(void)
{
    Attract a;
    uint32_t t = start(&a, 5);
    attract_shown(&a, 0);
    attract_update(&a, t + ATTRACT_COVER_MS, 5);
    CHECK_INT(attract_entry(&a), 1);
    attract_input(&a, t + 9000);
    CHECK_INT(attract_update(&a, t + 9000 + 3 * MIN, 5), ATTRACT_SHOW);
    CHECK_INT(attract_entry(&a), 0);
}

static void test_library_shrinking_to_nothing_returns_to_menu(void)
{
    Attract a;
    uint32_t t = start(&a, 2);
    CHECK_INT(attract_update(&a, t + 100, 0), ATTRACT_STOP);
    CHECK_INT(attract_active(&a), 0);
}

static void test_library_shrinking_wraps_to_first_entry(void)
{
    Attract a;
    uint32_t t = start(&a, 5);
    for (int i = 0; i < 3; i++) {
        attract_shown(&a, 0);
        t += ATTRACT_COVER_MS;
        attract_update(&a, t, 5);
    }
    CHECK_INT(attract_entry(&a), 3);
    attract_shown(&a, 0);
    CHECK_INT(attract_update(&a, t + ATTRACT_COVER_MS, 2), ATTRACT_SHOW);
    CHECK_INT(attract_entry(&a), 0);
}

static void test_never_starts_while_a_game_runs(void)
{
    Attract a;
    attract_init(&a, 1, 0);
    CHECK_INT(attract_game_running(&a, 1, 100), 0);
    CHECK_INT(attract_update(&a, 90 * MIN, 5), ATTRACT_NONE);
    CHECK_INT(attract_active(&a), 0);
}

static void test_idle_time_counts_from_when_the_game_stops(void)
{
    Attract a;
    attract_init(&a, 1, 0);
    attract_game_running(&a, 1, 100);
    attract_game_running(&a, 0, 30 * MIN);
    CHECK_INT(attract_update(&a, 31 * MIN - 1, 5), ATTRACT_NONE);
    CHECK_INT(attract_update(&a, 31 * MIN, 5), ATTRACT_SHOW);
}

static void test_game_starting_during_attract_returns_to_menu(void)
{
    Attract a;
    uint32_t t = start(&a, 5);
    CHECK_INT(attract_game_running(&a, 1, t + 10), 1);
    CHECK_INT(attract_active(&a), 0);
}

static void test_deadline_in_menu_is_end_of_idle_time(void)
{
    Attract a;
    uint32_t when = 0;
    attract_init(&a, 3, 1000);
    CHECK_INT(attract_deadline(&a, &when), 1);
    CHECK_INT((long)when, (long)(1000 + 3 * MIN));
    attract_input(&a, 5000);
    attract_deadline(&a, &when);
    CHECK_INT((long)when, (long)(5000 + 3 * MIN));
}

static void test_no_deadline_when_off_or_game_running(void)
{
    Attract a;
    uint32_t when = 0;
    attract_init(&a, 0, 1000);
    CHECK_INT(attract_deadline(&a, &when), 0);
    attract_init(&a, 3, 1000);
    attract_game_running(&a, 1, 1000);
    CHECK_INT(attract_deadline(&a, &when), 0);
}

static void test_deadline_during_attract_is_end_of_entry(void)
{
    Attract a;
    uint32_t when = 0;
    uint32_t t = start(&a, 5);
    attract_shown(&a, 0);
    CHECK_INT(attract_deadline(&a, &when), 1);
    CHECK_INT((long)when, (long)(t + ATTRACT_COVER_MS));
    attract_shown(&a, 1);
    attract_deadline(&a, &when);
    CHECK_INT((long)when, (long)(t + ATTRACT_PREVIEW_MS));
}

static void test_reporting_no_game_every_frame_does_not_delay_attract(void)
{
    Attract a;
    attract_init(&a, 1, 0);
    for (uint32_t t = 0; t < MIN; t += 1000)
        attract_game_running(&a, 0, t);
    CHECK_INT(attract_update(&a, MIN, 5), ATTRACT_SHOW);
}

static void test_input_right_after_waking_is_also_consumed(void)
{
    /* one pad press arrives as two events; a double-click as two clicks */
    Attract a;
    uint32_t t = start(&a, 5);
    CHECK_INT(attract_input(&a, t + 1000), 1);
    CHECK_INT(attract_input(&a, t + 1000), 1);
    CHECK_INT(attract_input(&a, t + 1000 + ATTRACT_WAKE_GUARD_MS - 1), 1);
    CHECK_INT(attract_active(&a), 0);
    CHECK_INT(attract_input(&a, t + 1000 + ATTRACT_WAKE_GUARD_MS), 0);
}

int main(void)
{
    RUN_TEST(test_starts_after_idle_minutes_with_first_entry);
    RUN_TEST(test_zero_minutes_never_starts);
    RUN_TEST(test_one_minute_setting_starts_after_one_minute);
    RUN_TEST(test_input_in_the_menu_restarts_idle_time_and_is_not_consumed);
    RUN_TEST(test_input_during_attract_returns_to_menu_and_is_consumed);
    RUN_TEST(test_empty_library_never_starts);
    RUN_TEST(test_idle_time_survives_tick_wraparound);
    RUN_TEST(test_entry_with_preview_shows_for_twenty_seconds);
    RUN_TEST(test_cover_only_entry_shows_for_eight_seconds);
    RUN_TEST(test_preview_ending_early_moves_to_next_entry);
    RUN_TEST(test_cycles_through_entries_in_library_order_and_wraps);
    RUN_TEST(test_restarts_from_first_entry_each_time);
    RUN_TEST(test_library_shrinking_to_nothing_returns_to_menu);
    RUN_TEST(test_library_shrinking_wraps_to_first_entry);
    RUN_TEST(test_never_starts_while_a_game_runs);
    RUN_TEST(test_idle_time_counts_from_when_the_game_stops);
    RUN_TEST(test_game_starting_during_attract_returns_to_menu);
    RUN_TEST(test_deadline_in_menu_is_end_of_idle_time);
    RUN_TEST(test_no_deadline_when_off_or_game_running);
    RUN_TEST(test_deadline_during_attract_is_end_of_entry);
    RUN_TEST(test_reporting_no_game_every_frame_does_not_delay_attract);
    RUN_TEST(test_input_right_after_waking_is_also_consumed);
    return TEST_REPORT();
}
