/* Gamepad input -> menu action translation (src/pad.c). */
#include "test.h"
#include "pad.h"

static void test_controller_a_launches_and_b_closes_info_panel(void)
{
    CHECK_INT(pad_button(PAD_BTN_A), PAD_ACT_LAUNCH);
    CHECK_INT(pad_button(PAD_BTN_B), PAD_ACT_CLOSE);
}

static void test_controller_dpad_moves_selection(void)
{
    CHECK_INT(pad_button(PAD_BTN_DPAD_UP), PAD_ACT_UP);
    CHECK_INT(pad_button(PAD_BTN_DPAD_DOWN), PAD_ACT_DOWN);
    CHECK_INT(pad_button(PAD_BTN_DPAD_LEFT), PAD_ACT_LEFT);
    CHECK_INT(pad_button(PAD_BTN_DPAD_RIGHT), PAD_ACT_RIGHT);
    CHECK_INT(pad_button(PAD_BTN_OTHER), PAD_ACT_NONE);
}

static void test_plain_joystick_first_button_launches_second_closes(void)
{
    CHECK_INT(pad_joy_button(0), PAD_ACT_LAUNCH);
    CHECK_INT(pad_joy_button(1), PAD_ACT_CLOSE);
    CHECK_INT(pad_joy_button(2), PAD_ACT_NONE);
    CHECK_INT(pad_joy_button(9), PAD_ACT_NONE);
}

static void test_hat_moves_once_per_press(void)
{
    PadNav nav;
    pad_init(&nav);
    CHECK_INT(pad_hat(&nav, 7, PAD_HAT_UP), PAD_ACT_UP);
    CHECK_INT(pad_hat(&nav, 7, PAD_HAT_UP), PAD_ACT_NONE);
    CHECK_INT(pad_hat(&nav, 7, PAD_HAT_CENTERED), PAD_ACT_NONE);
    CHECK_INT(pad_hat(&nav, 7, PAD_HAT_UP), PAD_ACT_UP);
    CHECK_INT(pad_hat(&nav, 7, PAD_HAT_CENTERED), PAD_ACT_NONE);
    CHECK_INT(pad_hat(&nav, 7, PAD_HAT_DOWN), PAD_ACT_DOWN);
    CHECK_INT(pad_hat(&nav, 7, PAD_HAT_LEFT), PAD_ACT_LEFT);
    CHECK_INT(pad_hat(&nav, 7, PAD_HAT_RIGHT), PAD_ACT_RIGHT);
}

static void test_hat_rolling_into_diagonal_moves_along_new_direction(void)
{
    PadNav nav;
    pad_init(&nav);
    CHECK_INT(pad_hat(&nav, 1, PAD_HAT_UP), PAD_ACT_UP);
    CHECK_INT(pad_hat(&nav, 1, PAD_HAT_UP | PAD_HAT_RIGHT), PAD_ACT_RIGHT);
    CHECK_INT(pad_hat(&nav, 1, PAD_HAT_RIGHT), PAD_ACT_NONE);
}

static void test_two_pads_hats_are_independent(void)
{
    PadNav nav;
    pad_init(&nav);
    CHECK_INT(pad_hat(&nav, 1, PAD_HAT_DOWN), PAD_ACT_DOWN);
    CHECK_INT(pad_hat(&nav, 2, PAD_HAT_DOWN), PAD_ACT_DOWN);
    CHECK_INT(pad_hat(&nav, 1, PAD_HAT_DOWN), PAD_ACT_NONE);
}

static void test_stick_inside_deadzone_does_nothing(void)
{
    PadNav nav;
    pad_init(&nav);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_X, 3000), PAD_ACT_NONE);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_X, -9000), PAD_ACT_NONE);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_Y, 12000), PAD_ACT_NONE);
}

static void test_stick_push_moves_once_until_released(void)
{
    PadNav nav;
    pad_init(&nav);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_X, 25000), PAD_ACT_RIGHT);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_X, 32767), PAD_ACT_NONE);
    /* wobbling near the threshold is not a new push */
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_X, 14000), PAD_ACT_NONE);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_X, 20000), PAD_ACT_NONE);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_X, 0), PAD_ACT_NONE);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_X, 30000), PAD_ACT_RIGHT);
}

static void test_stick_flicked_to_opposite_side_moves_back(void)
{
    PadNav nav;
    pad_init(&nav);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_X, -32768), PAD_ACT_LEFT);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_X, 32767), PAD_ACT_RIGHT);
}

static void test_stick_up_is_negative_y(void)
{
    PadNav nav;
    pad_init(&nav);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_Y, -32768), PAD_ACT_UP);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_Y, 0), PAD_ACT_NONE);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_Y, 32767), PAD_ACT_DOWN);
}

static void test_stick_axes_are_independent(void)
{
    PadNav nav;
    pad_init(&nav);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_X, 32767), PAD_ACT_RIGHT);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_Y, 32767), PAD_ACT_DOWN);
    CHECK_INT(pad_axis(&nav, 1, PAD_AXIS_X, 32767), PAD_ACT_NONE);
}

static void test_replugged_pad_starts_released(void)
{
    PadNav nav;
    pad_init(&nav);
    CHECK_INT(pad_axis(&nav, 3, PAD_AXIS_Y, 32767), PAD_ACT_DOWN);
    pad_forget(&nav, 3);
    CHECK_INT(pad_axis(&nav, 3, PAD_AXIS_Y, 32767), PAD_ACT_DOWN);
}

static void test_many_plug_cycles_keep_connected_pad_state(void)
{
    PadNav nav;
    pad_init(&nav);
    CHECK_INT(pad_hat(&nav, 1, PAD_HAT_UP), PAD_ACT_UP);
    for (long id = 100; id < 140; id++) {
        CHECK_INT(pad_hat(&nav, id, PAD_HAT_LEFT), PAD_ACT_LEFT);
        pad_forget(&nav, id);
    }
    CHECK_INT(pad_hat(&nav, 1, PAD_HAT_UP), PAD_ACT_NONE);
}

static void test_forgetting_unknown_pad_is_harmless(void)
{
    PadNav nav;
    pad_init(&nav);
    pad_forget(&nav, 42);
    CHECK_INT(pad_hat(&nav, 42, PAD_HAT_UP), PAD_ACT_UP);
}

int main(void)
{
    RUN_TEST(test_controller_a_launches_and_b_closes_info_panel);
    RUN_TEST(test_controller_dpad_moves_selection);
    RUN_TEST(test_plain_joystick_first_button_launches_second_closes);
    RUN_TEST(test_hat_moves_once_per_press);
    RUN_TEST(test_hat_rolling_into_diagonal_moves_along_new_direction);
    RUN_TEST(test_two_pads_hats_are_independent);
    RUN_TEST(test_stick_inside_deadzone_does_nothing);
    RUN_TEST(test_stick_push_moves_once_until_released);
    RUN_TEST(test_stick_flicked_to_opposite_side_moves_back);
    RUN_TEST(test_stick_up_is_negative_y);
    RUN_TEST(test_stick_axes_are_independent);
    RUN_TEST(test_replugged_pad_starts_released);
    RUN_TEST(test_many_plug_cycles_keep_connected_pad_state);
    RUN_TEST(test_forgetting_unknown_pad_is_harmless);
    return TEST_REPORT();
}
