/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd */

#include "stepper.h"

#include <stddef.h>

void stepper_init(st_stepper *s,
                  stepper_pin_fn_t step_fn,
                  stepper_pin_fn_t dir_fn,
                  stepper_pin_fn_t en_fn,
                  uint8_t          cw_value,
                  uint8_t          ccw_value)
{
    if (s == NULL) return;

    s->step_pin  = step_fn;
    s->dir_pin   = dir_fn;
    s->en_pin    = en_fn;

    s->cw_value  = cw_value;
    s->ccw_value = ccw_value;

    s->current_pos = 0;
    s->target_pos  = 0;

    s->delay_max = 0;
    s->delay_cnt = 0;

    s->step_high        = 0;
    s->current_dir      = 0;
    s->dir_initialized  = 0;
}

void stepper_tick(st_stepper *s)
{
    if (s == NULL || s->step_pin == NULL || s->dir_pin == NULL) {
        return;
    }

    /* Honour the per-toggle delay. */
    if (s->delay_cnt < s->delay_max) {
        s->delay_cnt++;
        return;
    }
    s->delay_cnt = 0;

    /* At target -- drive STEP low and idle. */
    if (s->target_pos == s->current_pos) {
        if (s->step_high) {
            s->step_pin(0);
            s->step_high = 0;
        }
        return;
    }

    /* Make sure DIR matches the way we need to move. The DIR pin is
     * touched only on transitions, not every tick, to minimise wasted
     * GPIO writes (and to give the driver chip the spec's setup-time
     * margin before the next rising STEP edge). */
    const uint8_t want_dir = (s->target_pos > s->current_pos)
                                ? s->cw_value
                                : s->ccw_value;
    if (!s->dir_initialized || want_dir != s->current_dir) {
        s->dir_pin(want_dir);
        s->current_dir     = want_dir;
        s->dir_initialized = 1;
    }

    /* Two ticks per pulse: rise on one tick, fall on the next.
     * Position is incremented on the rising edge -- that's when the
     * driver chip actually advances the motor. */
    if (!s->step_high) {
        s->step_pin(1);
        s->step_high = 1;
        if (s->target_pos > s->current_pos) {
            s->current_pos++;
        } else {
            s->current_pos--;
        }
    } else {
        s->step_pin(0);
        s->step_high = 0;
    }
}

void stepper_move_relative(st_stepper *s, int32_t delta_steps)
{
    if (s == NULL) return;
    s->target_pos += delta_steps;
}

void stepper_move_to(st_stepper *s, int32_t target)
{
    if (s == NULL) return;
    s->target_pos = target;
}

void stepper_set_delay(st_stepper *s, uint16_t delay_max)
{
    if (s == NULL) return;
    s->delay_max = delay_max;
}

void stepper_enable(st_stepper *s, int enabled)
{
    if (s == NULL || s->en_pin == NULL) return;
    s->en_pin(enabled ? 1u : 0u);
}

int32_t stepper_position(const st_stepper *s)
{
    return s != NULL ? s->current_pos : 0;
}

int32_t stepper_target(const st_stepper *s)
{
    return s != NULL ? s->target_pos : 0;
}

int stepper_is_busy(const st_stepper *s)
{
    return s != NULL && s->current_pos != s->target_pos;
}
