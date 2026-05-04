# stepper

Step-motor position controller. Drives a `STEP` / `DIR` / `EN`
style driver chip (DRV8825, A4988, TB6600, generic L297 +
Darlington, …) by calling caller-supplied pin-write functions on a
periodic tick. Position is signed `int32_t`, so negative
coordinates work natively.

The controller is dirt simple: you tell it where to go with
`stepper_move_relative` / `stepper_move_to`, it generates STEP
pulses on each subsequent `stepper_tick` until current == target.
No acceleration / deceleration profile (constant rate); add one in
your application code if you need it.

## Quick example

```c
#include "stepper.h"

static void step_pin(uint8_t v) { GPIO_WritePin(STEP_PORT, STEP_PIN, v); }
static void dir_pin (uint8_t v) { GPIO_WritePin(DIR_PORT,  DIR_PIN,  v); }
static void en_pin  (uint8_t v) { GPIO_WritePin(EN_PORT,   EN_PIN,   v); }

st_stepper motor;

void init_motor(void)
{
    /* On this wiring, DIR=1 turns clockwise. Pass NULL for the EN
     * callback if your driver has no enable line. */
    stepper_init(&motor, step_pin, dir_pin, en_pin,
                 /* cw  */ 1,
                 /* ccw */ 0);

    /* Tune speed: tick_rate / 2 / (1 + delay_max). On a 1 kHz tick:
     *   delay_max=0 -> 500 step/s
     *   delay_max=1 -> 250 step/s
     *   delay_max=4 -> 100 step/s */
    stepper_set_delay(&motor, 1);
    stepper_enable(&motor, 1);
}

void timer_1ms_isr(void) { stepper_tick(&motor); }   /* 1 kHz */

void some_command(void)
{
    stepper_move_relative(&motor, 200);              /* turn 200 steps CW */
    while (stepper_is_busy(&motor)) {
        /* let the ISR work; do other things */
    }
}
```

## API

| Function | Purpose |
|---|---|
| `stepper_init(s, step_fn, dir_fn, en_fn, cw, ccw)` | Wire up the three pin callbacks (en may be NULL) and the DIR-pin values that mean clockwise / counter-clockwise on your board. |
| `stepper_tick(s)` | Call from a periodic source. Each tick toggles STEP up to once; two ticks = one full pulse = one step of motion. |
| `stepper_move_relative(s, delta)` | Adjust target by `delta` (signed). |
| `stepper_move_to(s, target)` | Set absolute target position. |
| `stepper_set_delay(s, n)` | Inter-toggle delay. Output rate = `tick_rate / 2 / (1 + n)`. |
| `stepper_enable(s, on)` | Drive the EN pin via the registered callback. No-op if no EN was registered. |
| `stepper_position(s)` | Current position. |
| `stepper_target(s)` | Current target. |
| `stepper_is_busy(s)` | `1` if current != target. |

## Notes

- **Multi-instance safe.** All state lives in `st_stepper`; you can
  drive several motors from the same `tick` ISR by calling
  `stepper_tick` once per motor.
- **No acceleration profile.** Constant-rate from current_pos to
  target_pos. If you need trapezoidal / S-curve ramping, layer it
  on top by adjusting `stepper_set_delay` over time.
- **Speed control = `delay_max`.** Output rate is
  `tick_rate / 2 / (1 + delay_max)`. With a 1 kHz tick this gives
  500 / 250 / 167 / 125 / 100 Hz at delay_max = 0..4.
- The DIR pin is updated only on direction transitions, giving
  driver chips like DRV8825 their required setup-time margin
  before the next rising STEP edge.

## Build the self-test

```sh
make test       # from this directory
```

Drives a virtual controller through several moves with mock pin
callbacks and verifies the STEP / DIR call sequences, the position
counter, the wrap from CW to CCW direction, the `delay_max` timing,
and the EN-pin pass-through. Prints `stepper: All OK.` on success.

## Use in another project

Drop `stepper.h` and `stepper.c` into your sources.

## License

[MIT](../LICENSE).
