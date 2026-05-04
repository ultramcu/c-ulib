/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 *
 * Self-test using mock STEP / DIR / EN pin callbacks. Each callback
 * just records the value into a global for the test to inspect.
 * Runs the controller through some moves and verifies (a) the call
 * sequence on STEP/DIR/EN, (b) the position counter, and (c) the
 * direction handling at zero crossings. */

#include <stdint.h>
#include <stdio.h>

#include "../stepper.h"

static int g_step_calls = 0;
static int g_dir_calls  = 0;
static int g_en_calls   = 0;
static uint8_t g_step_value, g_dir_value, g_en_value;

/* Record the high/low pattern of STEP so we can sanity-check the
 * pulse train. */
#define STEP_TRACE_MAX 64
static uint8_t g_step_trace[STEP_TRACE_MAX];
static int     g_step_trace_n;

static void mock_step(uint8_t v)
{
    g_step_value = v;
    g_step_calls++;
    if (g_step_trace_n < STEP_TRACE_MAX) {
        g_step_trace[g_step_trace_n++] = v;
    }
}
static void mock_dir(uint8_t v)
{
    g_dir_value = v;
    g_dir_calls++;
}
static void mock_en(uint8_t v)
{
    g_en_value = v;
    g_en_calls++;
}

#define CHECK(expr) do { \
    if (!(expr)) { \
        printf("  ** FAIL: %s  (%s:%d)\n", #expr, __FILE__, __LINE__); \
        fails++; \
    } \
} while (0)

static void reset_mocks(void)
{
    g_step_calls = g_dir_calls = g_en_calls = 0;
    g_step_value = g_dir_value = g_en_value = 0;
    g_step_trace_n = 0;
}

int main(void)
{
    int fails = 0;
    st_stepper s;

    /* CW = 1, CCW = 0 on this fake board. */
    stepper_init(&s, mock_step, mock_dir, mock_en, /*cw=*/1, /*ccw=*/0);

    /* Fresh controller is at position 0, idle. */
    CHECK(stepper_position(&s) == 0);
    CHECK(stepper_target(&s)   == 0);
    CHECK(!stepper_is_busy(&s));

    /* Idle ticks should not produce any pulses. */
    reset_mocks();
    for (int i = 0; i < 5; i++) stepper_tick(&s);
    CHECK(g_step_calls == 0);
    CHECK(g_dir_calls  == 0);

    /* Move forward 3 steps. We expect:
     *   - DIR set to CW=1 once (transition from "uninitialised").
     *   - 6 STEP toggles total: HIGH-LOW-HIGH-LOW-HIGH-LOW.
     *   - Position increments on the rising edges. */
    reset_mocks();
    stepper_move_relative(&s, 3);
    CHECK(stepper_target(&s) == 3);
    CHECK(stepper_is_busy(&s));

    for (int i = 0; i < 6; i++) stepper_tick(&s);

    CHECK(g_dir_calls == 1);
    CHECK(g_dir_value == 1);                     /* CW */
    CHECK(g_step_calls == 6);

    /* Trace must alternate 1, 0, 1, 0, 1, 0. */
    static const uint8_t want_cw[6] = { 1, 0, 1, 0, 1, 0 };
    for (int i = 0; i < 6; i++) {
        CHECK(g_step_trace[i] == want_cw[i]);
    }

    CHECK(stepper_position(&s) == 3);
    CHECK(!stepper_is_busy(&s));

    /* Idle again -- ticks now drive STEP low and don't toggle further. */
    reset_mocks();
    for (int i = 0; i < 4; i++) stepper_tick(&s);
    CHECK(g_step_calls == 0);                    /* already low */

    /* Move back to position 1 (target = 1). Direction transitions to
     * CCW = 0, with one DIR write before any STEP toggles. */
    reset_mocks();
    stepper_move_to(&s, 1);
    CHECK(stepper_target(&s) == 1);

    /* 2 steps to go (3 -> 1) = 4 ticks. */
    for (int i = 0; i < 4; i++) stepper_tick(&s);

    CHECK(g_dir_calls == 1);
    CHECK(g_dir_value == 0);                     /* CCW */
    CHECK(g_step_calls == 4);
    CHECK(stepper_position(&s) == 1);
    CHECK(!stepper_is_busy(&s));

    /* delay_max = 1 means each toggle takes 2 ticks to land. So one
     * full step needs 4 ticks total (toggle every 2nd tick, two
     * toggles per step). */
    reset_mocks();
    stepper_set_delay(&s, 1);
    stepper_move_relative(&s, 1);

    for (int i = 0; i < 3; i++) stepper_tick(&s);
    CHECK(g_step_calls == 1);                    /* only 1 toggle in 3 ticks */
    for (int i = 0; i < 1; i++) stepper_tick(&s);
    /* By now (4 ticks total) we should have 2 toggles -> 1 step done */
    CHECK(g_step_calls == 2);
    CHECK(stepper_position(&s) == 2);

    /* Enable hook routes through the EN callback. */
    reset_mocks();
    stepper_enable(&s, 1);
    CHECK(g_en_calls == 1);
    CHECK(g_en_value == 1);
    stepper_enable(&s, 0);
    CHECK(g_en_calls == 2);
    CHECK(g_en_value == 0);

    /* NULL en_pin: stepper_enable should be a no-op. */
    st_stepper noenable;
    stepper_init(&noenable, mock_step, mock_dir, NULL, 1, 0);
    reset_mocks();
    stepper_enable(&noenable, 1);
    CHECK(g_en_calls == 0);

    if (fails == 0) {
        printf("stepper: All OK.\n");
        return 0;
    }
    printf("stepper: %d FAILURE(S)\n", fails);
    return 1;
}
