/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 *
 * stepper.h -- step-motor position controller.
 *
 * Drives a STEP / DIR / EN-style stepper driver (DRV8825, A4988,
 * TB6600, generic L297 + Darlington, etc.) by calling caller-supplied
 * pin-toggle functions on a periodic tick. Each tick may emit one
 * edge of the STEP pulse: two ticks together produce one full pulse
 * and one step of motion.
 *
 * Position is signed (int32_t), so negative coordinates work without
 * any tricks.
 *
 * Caller responsibilities:
 *   - Provide step_pin / dir_pin / en_pin write functions. en_pin may
 *     be NULL if your driver has no enable line (or you tie it active).
 *   - Decide what `cw_value` and `ccw_value` mean for your wiring -- the
 *     library doesn't know which direction your motor turns when the
 *     DIR pin is high.
 *   - Call stepper_tick() at a fixed frequency (1 kHz from a SysTick
 *     ISR is the typical choice). The output step rate is
 *         tick_rate / 2 / (1 + delay_max).
 */

#ifndef STEPPER_H
#define STEPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*stepper_pin_fn_t)(uint8_t value);

typedef struct {
    stepper_pin_fn_t step_pin;
    stepper_pin_fn_t dir_pin;
    stepper_pin_fn_t en_pin;        /* may be NULL */

    uint8_t  cw_value;              /* DIR-pin value for CW rotation */
    uint8_t  ccw_value;             /* DIR-pin value for CCW rotation */

    int32_t  current_pos;
    int32_t  target_pos;

    uint16_t delay_max;             /* extra ticks between toggles */
    uint16_t delay_cnt;

    uint8_t  step_high;             /* current logical state of STEP pin */
    uint8_t  current_dir;           /* last value driven on DIR */
    uint8_t  dir_initialized;       /* false until we drive DIR once */
} st_stepper;

/* stepper_init clears state and stores the pin-write callbacks.
 * en_pin may be NULL; cw_value / ccw_value are the DIR-pin values
 * that mean clockwise / counter-clockwise on the caller's wiring. */
void stepper_init(st_stepper *s,
                  stepper_pin_fn_t step_fn,
                  stepper_pin_fn_t dir_fn,
                  stepper_pin_fn_t en_fn,
                  uint8_t          cw_value,
                  uint8_t          ccw_value);

/* stepper_tick advances the controller one tick. Call it from a
 * periodic source (timer interrupt, RTOS tick, etc.). Each pair of
 * ticks generates one STEP pulse and changes current_pos by 1 in
 * the direction toward target_pos. When current_pos == target_pos
 * the controller drives STEP low and idles. */
void stepper_tick(st_stepper *s);

/* stepper_move_relative shifts target_pos by `delta_steps` from its
 * current value. Use this for "move N steps in this direction". */
void stepper_move_relative(st_stepper *s, int32_t delta_steps);

/* stepper_move_to sets target_pos to an absolute coordinate. */
void stepper_move_to(st_stepper *s, int32_t target);

/* stepper_set_delay tunes the inter-toggle delay. Output rate is
 * tick_rate / 2 / (1 + delay_max). delay_max == 0 is "as fast as
 * possible" (one toggle per tick). */
void stepper_set_delay(st_stepper *s, uint16_t delay_max);

/* stepper_enable drives the EN pin (if one was registered). */
void stepper_enable(st_stepper *s, int enabled);

/* Read-only accessors. */
int32_t stepper_position(const st_stepper *s);
int32_t stepper_target  (const st_stepper *s);
int     stepper_is_busy (const st_stepper *s);  /* current != target */

#ifdef __cplusplus
}
#endif

#endif /* STEPPER_H */
