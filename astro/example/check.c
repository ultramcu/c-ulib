/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 *
 * Self-test for the astro module. Reference values come from
 * Meeus, "Astronomical Algorithms", 2nd ed., chapters 12 and 13. */

#include <math.h>
#include <stdio.h>

#include "../astro.h"

static int fails = 0;

#define CHECK(expr) do { \
    if (!(expr)) { \
        printf("  ** FAIL: %s  (%s:%d)\n", #expr, __FILE__, __LINE__); \
        fails++; \
    } \
} while (0)

#define NEAR(a, b, eps) (fabs((a) - (b)) <= (eps))

int main(void)
{
    /* Angle conversion sanity checks. */
    CHECK(NEAR(astro_hms_to_deg(12, 30,  0.0), 187.5, 1e-9));
    CHECK(NEAR(astro_hms_to_deg(24,  0,  0.0), 360.0, 1e-9));
    CHECK(NEAR(astro_hms_to_deg(23,  9, 16.641), 347.31934, 1e-4));

    CHECK(NEAR(astro_dms_to_deg(45, 30,  0.0),  45.5,     1e-9));
    CHECK(NEAR(astro_dms_to_deg(-26, 25, 55.7), -26.43214, 1e-4));
    CHECK(NEAR(astro_dms_to_deg(38, 55, 17.0),  38.92139, 1e-4));

    /* GMST at the J2000.0 epoch (2000-01-01 12:00:00 UT) is exactly
     * 280.46061837 degrees by construction of the polynomial. */
    double gmst_j2000 = astro_gmst_deg(2000, 1, 1, 12, 0, 0);
    CHECK(NEAR(gmst_j2000, 280.46061837, 1e-3));

    /* Synthetic case: the object is directly overhead. We pick any
     * UT time, ask for its LMST at some longitude, then point at
     * RA = LMST and Dec = lat. Hour-angle is zero, so altitude must
     * come out as exactly 90 degrees. This is the sharpest test in
     * the suite -- the formula collapses to cos(0) = 1. */
    {
        double lst = astro_lmst_deg(2024, 6, 21, 18, 0, 0, 100.5);
        double az = -1.0, alt = -1.0;
        astro_eq_to_horizon(2024, 6, 21, 18, 0, 0,
                            /*lat=*/13.75, /*long=*/100.5,
                            /*ra =*/lst,   /*dec =*/13.75,
                            &az, &alt);
        CHECK(NEAR(alt, 90.0, 1e-6));
    }

    /* Meeus, Example 13.b: Venus on 1987-04-10 at 19h 21m 0s UT,
     * observer at the U.S. Naval Observatory (lat = +38d 55' 17",
     * long = 77d 03' 56" west). Apparent RA = 23h 09m 16.641s,
     * Dec = -6d 43' 11.61".
     *
     * Meeus's worked answer: altitude 15.1249 deg, azimuth 68.0337
     * deg measured WEST FROM SOUTH, which is 248.0337 deg measured
     * CW FROM NORTH (the convention used here).
     *
     * We feed mean (not apparent) sidereal time, so allow ~0.05 deg
     * to absorb the equation-of-equinoxes offset. */
    {
        double lat = astro_dms_to_deg(38, 55, 17.0);
        double lon = -astro_dms_to_deg(77,  3, 56.0);   /* east-positive */
        double ra  = astro_hms_to_deg(23,  9, 16.641);
        double dec = -astro_dms_to_deg(6,  43, 11.61);
        double az = 0.0, alt = 0.0;

        astro_eq_to_horizon(1987, 4, 10, 19, 21, 0,
                            lat, lon,
                            ra,  dec,
                            &az, &alt);

        CHECK(NEAR(alt,  15.1249, 0.05));
        CHECK(NEAR(az,  248.0337, 0.05));
    }

    /* NULL output pointers must be tolerated. */
    astro_eq_to_horizon(2026, 5, 4, 13, 30, 0,
                        13.75, 100.5,
                        180.0, 0.0,
                        NULL, NULL);

    if (fails == 0) {
        printf("astro: All OK.\n");
        return 0;
    }
    printf("astro: %d FAILURE(S)\n", fails);
    return 1;
}
