# astro

Minimal astronomy: angle conversions, mean sidereal time, and
the equatorial-to-horizon transformation. Pure C99, double
precision, link with `-lm`.

This is the math you need to point a small telescope or plot
the sky for a given time and place. It is **not** a full
reduction pipeline — there is no precession, no nutation, no
aberration, and no atmospheric refraction. The RA / Dec you pass
in is treated as the truth; the alt / az you get back is in the
same epoch. Accuracy is about 0.01 degree on recent dates, which
is enough to slew a mount but not enough for arcsecond
astrometry. If you need that, reach for SOFA or NOVAS.

```c
#include "astro.h"

/* Where will Vega (RA = 18h 36m 56s, Dec = +38d 47m 1s) appear
 * over Bangkok at 2026-05-04 13:30 UT? */
double ra_deg  = astro_hms_to_deg(18, 36, 56);
double dec_deg = astro_dms_to_deg(38, 47,  1);
double az, alt;
astro_eq_to_horizon(2026, 5, 4, 13, 30, 0,
                    /*lat =*/ 13.7563,
                    /*long=*/100.5018,
                    ra_deg, dec_deg,
                    &az, &alt);
```

## Functions

| Function | Purpose |
|---|---|
| `astro_hms_to_deg(h, m, s)` | RA in hours / minutes / seconds to decimal degrees. One hour of RA is 15 degrees. |
| `astro_dms_to_deg(d, m, s)` | Signed degrees / arc-minutes / arc-seconds to decimal degrees. |
| `astro_gmst_deg(y, mo, d, h, mi, s)` | Greenwich Mean Sidereal Time, IAU 1982 polynomial, degrees in `[0, 360)`. |
| `astro_lmst_deg(..., long_deg)` | Local Mean Sidereal Time = `astro_gmst_deg(...) + long_deg`, wrapped. |
| `astro_eq_to_horizon(...)` | RA / Dec to azimuth (CW from north) and altitude. Uses `atan2` so there is no special case at the zenith or the celestial poles. |

All angle inputs and outputs are decimal degrees. Longitudes
are **east positive** (the IAU convention). If your value comes
in the navigational "west positive" form, negate it.

Either output pointer of `astro_eq_to_horizon` may be `NULL`.

## Build the self-test

```sh
make test       # from this directory
```

Builds `example/check.c`, runs it, prints `astro: All OK.` on
success and exits zero. The check covers:

- Trivial conversions (`12h 30m 0s` → `187.5 deg`, signed DMS).
- GMST at the J2000.0 epoch (2000-01-01 12:00 UT) evaluates to
  `280.46061837 deg`.
- A synthetic case where the object sits at the zenith
  (`HA = 0`, `dec = lat`) returns `altitude = 90 deg`.
- Meeus, *Astronomical Algorithms* 2nd ed., Example 13.b: Venus
  on 1987-04-10 at the U.S. Naval Observatory. Reference values
  are altitude `15.1249 deg`, azimuth `248.0337 deg` (CW from
  north).

## Use in another project

Copy `astro.h` and `astro.c` into your sources, link with `-lm`.
No other module dependencies.

## License

[MIT](../LICENSE).
