/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 *
 * Self-test for the nmea module. Builds valid NMEA sentences on
 * the fly (computing the XOR checksum), feeds them through the
 * parser, and asserts that the right callback fires and the
 * snapshot fields decode to the expected values. Also covers
 * the "bad checksum -> no callback" path and the timezone
 * helper across a leap-year and a year-boundary rollback. */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../nmea.h"

static int fails = 0;

#define CHECK(expr) do { \
    if (!(expr)) { \
        printf("  ** FAIL: %s  (%s:%d)\n", #expr, __FILE__, __LINE__); \
        fails++; \
    } \
} while (0)

#define NEAR(a, b, eps) (fabs((a) - (b)) <= (eps))

/* ------------------------------------------------------------ */
/* Test plumbing                                                */
/* ------------------------------------------------------------ */

static int g_fired_rmc;
static int g_fired_gga;
static int g_fired_gsa;

static void on_msg(nmea_msg_type t, void *cookie)
{
    (void)cookie;
    switch (t) {
        case NMEA_MSG_RMC: g_fired_rmc++; break;
        case NMEA_MSG_GGA: g_fired_gga++; break;
        case NMEA_MSG_GSA: g_fired_gsa++; break;
    }
}

static void reset_counters(void)
{
    g_fired_rmc = g_fired_gga = g_fired_gsa = 0;
}

/* Compute the XOR checksum of every byte between the leading '$'
 * and the trailing '*'. */
static uint8_t xor_csum(const char *body)
{
    uint8_t c = 0;
    for (; *body; body++) c ^= (uint8_t)*body;
    return c;
}

/* Wrap `body` into a full sentence: "$body*XX\r\n". Returns the
 * number of bytes written (excluding the trailing NUL). */
static size_t make_sentence(const char *body, char *out, size_t cap)
{
    int n = snprintf(out, cap, "$%s*%02X\r\n", body, xor_csum(body));
    return (n < 0) ? 0 : (size_t)n;
}

static void feed_str(st_nmea *n, const char *s)
{
    nmea_feed(n, (const uint8_t *)s, strlen(s));
}

/* ------------------------------------------------------------ */
/* Tests                                                        */
/* ------------------------------------------------------------ */

static void test_rmc(void)
{
    printf("-- RMC happy path --\n");
    st_nmea n;
    nmea_init(&n, on_msg, NULL);
    reset_counters();

    char buf[128];
    /* From u-blox NMEA examples. Lat 47 17.11437' N = 47.285239 deg.
     * Lon 8 33.91522' E = 8.565254 deg. Date 09/12/02 = 2002-12-09. */
    make_sentence("GPRMC,083559.00,A,4717.11437,N,00833.91522,E,"
                  "0.004,77.52,091202,,,A", buf, sizeof(buf));
    feed_str(&n, buf);

    CHECK(g_fired_rmc == 1);
    CHECK(g_fired_gga == 0);
    CHECK(g_fired_gsa == 0);
    CHECK(n.rmc.valid == 'A');
    CHECK(n.rmc.pos_mode == 'A');
    CHECK(n.rmc.ns == 'N');
    CHECK(n.rmc.ew == 'E');
    CHECK(n.rmc.time.h == 8 && n.rmc.time.m == 35 && n.rmc.time.s == 59);
    CHECK(n.rmc.date.d == 9 && n.rmc.date.m == 12 && n.rmc.date.y == 2);
    CHECK(NEAR(n.rmc.lat_deg,  47.285239, 1e-5));
    CHECK(NEAR(n.rmc.lon_deg,   8.565254, 1e-5));
    CHECK(NEAR(n.rmc.speed_knots, 0.004,  1e-6));
    CHECK(NEAR(n.rmc.course_deg, 77.52,   1e-3));
}

static void test_gga(void)
{
    printf("-- GGA happy path --\n");
    st_nmea n;
    nmea_init(&n, on_msg, NULL);
    reset_counters();

    char buf[128];
    make_sentence("GPGGA,092725.00,4717.11399,N,00833.91590,E,"
                  "1,8,1.01,499.6,M,48.0,M,,", buf, sizeof(buf));
    feed_str(&n, buf);

    CHECK(g_fired_gga == 1);
    CHECK(g_fired_rmc == 0);
    CHECK(n.gga.fix_status == '1');
    CHECK(n.gga.sats_used  == 8);
    CHECK(n.gga.ns == 'N');
    CHECK(n.gga.ew == 'E');
    CHECK(n.gga.time.h == 9 && n.gga.time.m == 27 && n.gga.time.s == 25);
    CHECK(NEAR(n.gga.lat_deg,    47.285233, 1e-5));
    CHECK(NEAR(n.gga.lon_deg,     8.565265, 1e-5));
    CHECK(NEAR(n.gga.hdop,        1.01,     1e-3));
    CHECK(NEAR(n.gga.altitude_m, 499.6,     1e-2));
}

static void test_gsa(void)
{
    printf("-- GSA happy path --\n");
    st_nmea n;
    nmea_init(&n, on_msg, NULL);
    reset_counters();

    char buf[128];
    make_sentence("GPGSA,A,3,23,29,07,08,09,18,26,28,,,,,"
                  "1.94,1.18,1.54", buf, sizeof(buf));
    feed_str(&n, buf);

    CHECK(g_fired_gsa == 1);
    CHECK(n.gsa.mode   == 'A');
    CHECK(n.gsa.fix_3d == '3');
    CHECK(NEAR(n.gsa.pdop, 1.94, 1e-3));
    CHECK(NEAR(n.gsa.hdop, 1.18, 1e-3));
    CHECK(NEAR(n.gsa.vdop, 1.54, 1e-3));
}

static void test_southern_hemisphere(void)
{
    printf("-- southern / western hemispheres are negative --\n");
    st_nmea n;
    nmea_init(&n, on_msg, NULL);
    reset_counters();

    /* Sao Paulo, roughly: lat 23 33.0' S, lon 46 38.0' W. */
    char buf[128];
    make_sentence("GPRMC,120000.00,A,2333.000,S,04638.000,W,"
                  "0.0,0.0,010125,,,A", buf, sizeof(buf));
    feed_str(&n, buf);

    CHECK(g_fired_rmc == 1);
    CHECK(n.rmc.lat_deg < 0);
    CHECK(n.rmc.lon_deg < 0);
    CHECK(NEAR(n.rmc.lat_deg, -23.55,    1e-4));
    CHECK(NEAR(n.rmc.lon_deg, -46.6333,  1e-4));
}

static void test_bad_checksum(void)
{
    printf("-- bad checksum -> sentence dropped --\n");
    st_nmea n;
    nmea_init(&n, on_msg, NULL);
    reset_counters();

    char buf[128];
    make_sentence("GPRMC,083559.00,A,4717.11437,N,00833.91522,E,"
                  "0.004,77.52,091202,,,A", buf, sizeof(buf));

    /* Flip a bit in one of the two checksum hex digits. The
     * sentence body is followed by '*' then two hex chars. */
    char *star = strchr(buf, '*');
    CHECK(star != NULL);
    star[1] ^= 0x01;
    feed_str(&n, buf);

    CHECK(g_fired_rmc == 0);
}

static void test_burst(void)
{
    printf("-- burst feed: two sentences in one buffer --\n");
    st_nmea n;
    nmea_init(&n, on_msg, NULL);
    reset_counters();

    char rmc[128], gga[128];
    size_t a = make_sentence("GPRMC,083559.00,A,4717.11437,N,00833.91522,E,"
                             "0.004,77.52,091202,,,A", rmc, sizeof(rmc));
    size_t b = make_sentence("GPGGA,092725.00,4717.11399,N,00833.91590,E,"
                             "1,8,1.01,499.6,M,48.0,M,,", gga, sizeof(gga));

    char joined[300];
    memcpy(joined,         rmc, a);
    memcpy(joined + a,     gga, b);
    nmea_feed(&n, (const uint8_t *)joined, a + b);

    CHECK(g_fired_rmc == 1);
    CHECK(g_fired_gga == 1);
}

static void test_noise_before_dollar(void)
{
    printf("-- noise before '$' is ignored --\n");
    st_nmea n;
    nmea_init(&n, on_msg, NULL);
    reset_counters();

    char buf[128];
    make_sentence("GPRMC,083559.00,A,4717.11437,N,00833.91522,E,"
                  "0.004,77.52,091202,,,A", buf, sizeof(buf));

    /* Prepend leftover bytes from a previous truncated sentence. */
    char noisy[200];
    int  n_noise = snprintf(noisy, sizeof(noisy),
                            "junk,more junk,blah*ZZ\r\n%s", buf);
    nmea_feed(&n, (const uint8_t *)noisy, (size_t)n_noise);

    CHECK(g_fired_rmc == 1);
}

static void test_tz_forward_leap(void)
{
    printf("-- timezone +07:00 across leap-year February 29 --\n");
    st_nmea_date u = { .d = 28, .m = 2, .y = 24 };   /* 2024 is leap */
    st_nmea_time t = { .h = 22, .m = 0,  .s = 0  };
    st_nmea_date od; st_nmea_time ot;
    nmea_shift_tz(u, t, 7 * 60, &od, &ot);

    CHECK(od.d == 29);
    CHECK(od.m == 2);
    CHECK(od.y == 24);
    CHECK(ot.h == 5 && ot.m == 0 && ot.s == 0);
}

static void test_tz_forward_nonleap(void)
{
    printf("-- timezone +07:00 across non-leap February 28 --\n");
    st_nmea_date u = { .d = 28, .m = 2, .y = 23 };   /* 2023 not leap */
    st_nmea_time t = { .h = 22, .m = 0,  .s = 0  };
    st_nmea_date od; st_nmea_time ot;
    nmea_shift_tz(u, t, 7 * 60, &od, &ot);

    CHECK(od.d == 1);
    CHECK(od.m == 3);
    CHECK(od.y == 23);
    CHECK(ot.h == 5);
}

static void test_tz_backward_year_boundary(void)
{
    printf("-- timezone -05:00 across new-year boundary --\n");
    st_nmea_date u = { .d = 1, .m = 1, .y = 26 };    /* 2026-01-01 */
    st_nmea_time t = { .h = 3, .m = 0, .s = 0  };
    st_nmea_date od; st_nmea_time ot;
    nmea_shift_tz(u, t, -5 * 60, &od, &ot);

    CHECK(od.d == 31);
    CHECK(od.m == 12);
    CHECK(od.y == 25);                              /* 2025-12-31 */
    CHECK(ot.h == 22 && ot.m == 0 && ot.s == 0);
}

static void test_tz_minute_resolution(void)
{
    printf("-- timezone +05:30 (India) keeps minutes --\n");
    st_nmea_date u = { .d = 15, .m = 6, .y = 24 };
    st_nmea_time t = { .h = 12, .m = 0, .s = 0  };
    st_nmea_date od; st_nmea_time ot;
    nmea_shift_tz(u, t, 5 * 60 + 30, &od, &ot);

    CHECK(od.d == 15 && od.m == 6 && od.y == 24);
    CHECK(ot.h == 17 && ot.m == 30);
}

int main(void)
{
    test_rmc();
    test_gga();
    test_gsa();
    test_southern_hemisphere();
    test_bad_checksum();
    test_burst();
    test_noise_before_dollar();
    test_tz_forward_leap();
    test_tz_forward_nonleap();
    test_tz_backward_year_boundary();
    test_tz_minute_resolution();

    if (fails == 0) {
        printf("nmea: All OK.\n");
        return 0;
    }
    printf("nmea: %d FAILURE(S)\n", fails);
    return 1;
}
