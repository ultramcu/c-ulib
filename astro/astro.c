/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd */

#include "astro.h"

#include <math.h>

#define ASTRO_PI       3.14159265358979323846
#define ASTRO_DEG2RAD  (ASTRO_PI / 180.0)
#define ASTRO_RAD2DEG  (180.0 / ASTRO_PI)

static double wrap360(double x)
{
    x = fmod(x, 360.0);
    if (x < 0.0) x += 360.0;
    return x;
}

static double clamp_unit(double v)
{
    if (v < -1.0) return -1.0;
    if (v >  1.0) return  1.0;
    return v;
}

/* Julian Date for a Gregorian UT date and time. Standard Meeus
 * algorithm (no fold for the 1582 calendar transition -- callers
 * outside the Gregorian range should not be using this library
 * anyway). */
static double julian_date(int year, int month, int day,
                          int hour, int minute, int second)
{
    if (month <= 2) {
        year  -= 1;
        month += 12;
    }

    int a = year / 100;
    int b = 2 - a + (a / 4);

    double frac = ((double)hour
                + (double)minute / 60.0
                + (double)second / 3600.0) / 24.0;

    return floor(365.25 * (double)(year + 4716))
         + floor(30.6001 * (double)(month + 1))
         + (double)day + (double)b - 1524.5
         + frac;
}

double astro_hms_to_deg(double hour, double minute, double second)
{
    double sign = (hour < 0.0) ? -1.0 : 1.0;
    double h    = fabs(hour);
    return sign * 15.0 * (h + minute / 60.0 + second / 3600.0);
}

double astro_dms_to_deg(double degree, double minute, double second)
{
    double sign = (degree < 0.0) ? -1.0 : 1.0;
    double d    = fabs(degree);
    return sign * (d + minute / 60.0 + second / 3600.0);
}

double astro_gmst_deg(int year, int month, int day,
                      int hour, int minute, int second)
{
    double jd = julian_date(year, month, day, hour, minute, second);
    double d  = jd - 2451545.0;
    double t  = d / 36525.0;

    double mst = 280.46061837
               + 360.98564736629 * d
               + 0.000387933 * t * t
               - (t * t * t) / 38710000.0;

    return wrap360(mst);
}

double astro_lmst_deg(int year, int month, int day,
                      int hour, int minute, int second,
                      double long_deg)
{
    double gmst = astro_gmst_deg(year, month, day,
                                 hour, minute, second);
    return wrap360(gmst + long_deg);
}

void astro_eq_to_horizon(int year, int month, int day,
                         int hour, int minute, int second,
                         double lat_deg, double long_deg,
                         double ra_deg,  double dec_deg,
                         double *out_az_deg,
                         double *out_alt_deg)
{
    double lst    = astro_lmst_deg(year, month, day,
                                   hour, minute, second, long_deg);
    double ha_deg = wrap360(lst - ra_deg);

    double ha  = ha_deg  * ASTRO_DEG2RAD;
    double dec = dec_deg * ASTRO_DEG2RAD;
    double lat = lat_deg * ASTRO_DEG2RAD;

    double sd = sin(dec), cd = cos(dec);
    double sl = sin(lat), cl = cos(lat);
    double sh = sin(ha),  ch = cos(ha);

    double sin_alt = clamp_unit(sd * sl + cd * cl * ch);
    double alt     = asin(sin_alt);

    /* Direction cosines in the local horizontal frame
     * (x = east, y = north, z = up). atan2(east, north)
     * then yields azimuth measured CW from north. */
    double east  = -cd * sh;
    double north =  sd * cl - cd * ch * sl;
    double az    = atan2(east, north);

    if (out_alt_deg) *out_alt_deg = alt * ASTRO_RAD2DEG;
    if (out_az_deg)  *out_az_deg  = wrap360(az * ASTRO_RAD2DEG);
}
