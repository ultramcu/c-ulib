/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 *
 * astro.h -- minimal astronomy: angle conversions, mean sidereal
 * time, and the equatorial-to-horizon transformation.
 *
 * Scope is intentionally small. There is no precession, no
 * nutation, no aberration, and no atmospheric refraction. The
 * RA / Dec you pass in is treated as the truth and the alt / az
 * you get back is in the same epoch. Accuracy is roughly 0.01
 * degree on recent dates -- enough to point a small telescope,
 * not enough for arcsecond astrometry. If you need that, reach
 * for SOFA or NOVAS.
 *
 * Pure C99, double precision, no allocations, no globals. Link
 * with -lm.
 *
 * Conventions:
 *   - All angles in / out are decimal degrees.
 *   - Longitudes are EAST POSITIVE (IAU). Negate values given
 *     in the navigational "west positive" form.
 *   - Azimuth is measured CLOCKWISE FROM NORTH, in [0, 360).
 *   - Time inputs are UT (UTC is close enough for this accuracy).
 */

#ifndef ASTRO_H
#define ASTRO_H

#ifdef __cplusplus
extern "C" {
#endif

/* astro_hms_to_deg converts a right-ascension expressed as
 * (hours, minutes, seconds) to decimal degrees. One hour of RA
 * is 15 degrees. If `hour` is negative, the sign is applied to
 * the whole bundle. */
double astro_hms_to_deg(double hour, double minute, double second);

/* astro_dms_to_deg converts a (degrees, arc-minutes, arc-seconds)
 * triple to decimal degrees. The sign of the result follows the
 * sign of `degree`. To express a southern angle whose integer
 * part is zero (e.g. -0 deg 25 min), negate the call result:
 *     dec = -astro_dms_to_deg(0, 25, 30);                       */
double astro_dms_to_deg(double degree, double minute, double second);

/* astro_gmst_deg returns Greenwich Mean Sidereal Time in degrees
 * for the given UT date and time, wrapped to [0, 360). Uses the
 * IAU 1982 polynomial; better than 0.001 degrees over 1900-2100. */
double astro_gmst_deg(int year, int month, int day,
                      int hour, int minute, int second);

/* astro_lmst_deg = GMST + longitude (east positive), wrapped to
 * [0, 360). */
double astro_lmst_deg(int year, int month, int day,
                      int hour, int minute, int second,
                      double long_deg);

/* astro_eq_to_horizon converts equatorial coordinates (RA, Dec)
 * to local horizontal coordinates (azimuth, altitude) for the
 * given UT date / time and observer location.
 *
 *   ra_deg   : right ascension in degrees [0, 360). Use
 *              astro_hms_to_deg() if you have it as h/m/s.
 *   dec_deg  : declination in degrees [-90, +90].
 *   lat_deg  : observer latitude, north positive.
 *   long_deg : observer longitude, east positive.
 *
 * Outputs (either pointer may be NULL):
 *   *out_az_deg  : azimuth, degrees CW from north [0, 360).
 *   *out_alt_deg : altitude, degrees [-90, +90]. Negative means
 *                  the object is below the horizon.
 *
 * Uses atan2 internally, so there is no special case at the
 * zenith or the celestial poles. */
void astro_eq_to_horizon(int year, int month, int day,
                         int hour, int minute, int second,
                         double lat_deg, double long_deg,
                         double ra_deg,  double dec_deg,
                         double *out_az_deg,
                         double *out_alt_deg);

#ifdef __cplusplus
}
#endif

#endif /* ASTRO_H */
