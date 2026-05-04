# math

Header-only integer math helpers. Five static-inline functions
covering the everyday cases that come up in sensor / control code.

```c
#include "intmath.h"

int32_t a = int_pow_i32(10, 3);                 /* 1000 */
int32_t b = int_clamp_i32(reading, 0, 1023);    /* keep in 10-bit range */
int32_t c = int_min_i32(left_speed, right_speed);
int32_t d = int_max_i32(measured, threshold);
int32_t e = int_abs_i32(error);                 /* signed -> magnitude */
```

## Functions

| Function | Purpose |
|---|---|
| `int_pow_i32(base, exponent)` | base raised to the power exponent (uint8_t). exponent == 0 returns 1. Wraps on int32 overflow. |
| `int_clamp_i32(v, lo, hi)` | v constrained to [lo, hi]. Caller must ensure lo <= hi. |
| `int_min_i32(a, b)` | smaller of a and b |
| `int_max_i32(a, b)` | larger of a and b |
| `int_abs_i32(v)` | absolute value of v |

All five are `static inline` — there is no `.c` file to compile.
At `-O2` the calls disappear into the call site.

## Build the self-test

```sh
make test       # from this directory
```

Builds `example/check.c`, runs it, prints `intmath: All OK.` on
success and exits zero. The check exercises all five helpers
against hand-computed expected values, including negative bases
and clamp / abs edge cases.

## Use in another project

Drop `intmath.h` into your sources and `#include "intmath.h"`. No
linkage required.

## License

[MIT](../LICENSE).
