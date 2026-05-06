/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd */

#include "nmea.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------ */
/* States                                                       */
/* ------------------------------------------------------------ */

enum {
    NM_WAIT_DOLLAR = 0,
    NM_IN_BODY,         /* between '$' and '*' */
    NM_AFTER_STAR,      /* collecting two checksum hex chars */
    NM_AFTER_CSUM       /* csum bytes complete; await CR/LF */
};

/* ------------------------------------------------------------ */
/* Small helpers                                                */
/* ------------------------------------------------------------ */

static int hex_nibble(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static void reset_state(st_nmea *n)
{
    n->state         = NM_WAIT_DOLLAR;
    n->line_len      = 0;
    n->overflow      = 0;
    n->computed_csum = 0;
    n->csum_len      = 0;
}

/* field_at returns a pointer into n->line for the n-th comma-
 * separated field, writing the field length to *out_len. Field 0
 * is the talker+id (e.g. "GPRMC"). Returns NULL if n is past the
 * last field. The returned pointer is NOT NUL-terminated. */
static const char *field_at(const char *line, uint16_t line_len,
                            uint16_t which, size_t *out_len)
{
    uint16_t start = 0;
    uint16_t k     = 0;

    for (uint16_t i = 0; i <= line_len; i++) {
        if (i == line_len || line[i] == ',') {
            if (k == which) {
                *out_len = (size_t)(i - start);
                return &line[start];
            }
            k++;
            start = (uint16_t)(i + 1);
        }
    }
    return NULL;
}

/* field_copy copies a field into a NUL-terminated buffer for use
 * with atoi / strtod. Returns 0 on success, -1 if the field
 * doesn't fit. */
static int field_copy(const char *f, size_t len, char *buf, size_t cap)
{
    if (len + 1 > cap) return -1;
    memcpy(buf, f, len);
    buf[len] = '\0';
    return 0;
}

static double field_to_double(const char *f, size_t len)
{
    char buf[24];
    if (field_copy(f, len, buf, sizeof(buf)) < 0) return 0.0;
    return strtod(buf, NULL);
}

static int field_to_int(const char *f, size_t len)
{
    char buf[12];
    if (field_copy(f, len, buf, sizeof(buf)) < 0) return 0;
    return atoi(buf);
}

/* parse_dmm decodes "DDMM.MMMM" or "DDDMM.MMMM" into degrees
 * decimal. The integer part of minutes is the two characters
 * immediately before the dot; whatever comes before is degrees.
 * If there is no dot the whole field is treated as MM (rare). */
static double parse_dmm(const char *s, size_t len)
{
    if (len < 3) return 0.0;

    size_t dot = len;
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '.') { dot = i; break; }
    }
    if (dot < 2) return 0.0;
    size_t deg_chars = dot - 2;

    char buf[8];
    if (deg_chars >= sizeof(buf)) return 0.0;
    memcpy(buf, s, deg_chars);
    buf[deg_chars] = '\0';
    int deg = atoi(buf);

    double minutes = field_to_double(s + deg_chars, len - deg_chars);
    return (double)deg + minutes / 60.0;
}

static int parse_hms(const char *s, size_t len, st_nmea_time *t)
{
    if (len < 6) return -1;
    char b[3] = { 0, 0, 0 };
    b[0] = s[0]; b[1] = s[1]; t->h = (uint8_t)atoi(b);
    b[0] = s[2]; b[1] = s[3]; t->m = (uint8_t)atoi(b);
    b[0] = s[4]; b[1] = s[5]; t->s = (uint8_t)atoi(b);
    return 0;
}

static int parse_dmy(const char *s, size_t len, st_nmea_date *d)
{
    if (len < 6) return -1;
    char b[3] = { 0, 0, 0 };
    b[0] = s[0]; b[1] = s[1]; d->d = (uint8_t)atoi(b);
    b[0] = s[2]; b[1] = s[3]; d->m = (uint8_t)atoi(b);
    b[0] = s[4]; b[1] = s[5]; d->y = (uint8_t)atoi(b);
    return 0;
}

/* ------------------------------------------------------------ */
/* Sentence decoders                                            */
/* ------------------------------------------------------------ */

/* RMC field layout (idx 0 is the $xxRMC tag itself):
 *   1  time HHMMSS(.SSS)
 *   2  data valid 'A' / 'V'
 *   3  latitude  DDMM.MMMM
 *   4  N / S
 *   5  longitude DDDMM.MMMM
 *   6  E / W
 *   7  speed (knots)
 *   8  course over ground (degrees true)
 *   9  date DDMMYY
 *  10  magnetic variation magnitude (often empty)
 *  11  magnetic variation E / W (often empty)
 *  12  positioning mode (NMEA 2.3+; 'A' / 'D' / 'N')
 */
static void decode_rmc(st_nmea *n)
{
    st_nmea_rmc r;
    memset(&r, 0, sizeof(r));

    size_t      len;
    const char *f;

    if ((f = field_at(n->line, n->line_len, 1, &len)) && len) parse_hms(f, len, &r.time);
    if ((f = field_at(n->line, n->line_len, 2, &len)) && len) r.valid = f[0];

    const char *flat = field_at(n->line, n->line_len, 3, &len);
    size_t      lat_len = len;
    f = field_at(n->line, n->line_len, 4, &len);
    r.ns = (f && len) ? f[0] : 0;
    if (flat && lat_len) {
        r.lat_deg = parse_dmm(flat, lat_len);
        if (r.ns == 'S' || r.ns == 's') r.lat_deg = -r.lat_deg;
    }

    const char *flon = field_at(n->line, n->line_len, 5, &len);
    size_t      lon_len = len;
    f = field_at(n->line, n->line_len, 6, &len);
    r.ew = (f && len) ? f[0] : 0;
    if (flon && lon_len) {
        r.lon_deg = parse_dmm(flon, lon_len);
        if (r.ew == 'W' || r.ew == 'w') r.lon_deg = -r.lon_deg;
    }

    if ((f = field_at(n->line, n->line_len, 7, &len)) && len) r.speed_knots = field_to_double(f, len);
    if ((f = field_at(n->line, n->line_len, 8, &len)) && len) r.course_deg  = field_to_double(f, len);
    if ((f = field_at(n->line, n->line_len, 9, &len)) && len) parse_dmy(f, len, &r.date);
    if ((f = field_at(n->line, n->line_len, 12, &len)) && len) r.pos_mode = f[0];

    n->rmc = r;
}

/* GGA field layout:
 *   1  time HHMMSS(.SSS)
 *   2  latitude
 *   3  N / S
 *   4  longitude
 *   5  E / W
 *   6  fix status '0'..'6'
 *   7  satellites used (count)
 *   8  HDOP
 *   9  altitude
 *  10  altitude unit ('M', ignored)
 *  11  geoid separation
 *  12  geoid separation unit (ignored)
 *  13  DGPS age
 *  14  DGPS station ID
 */
static void decode_gga(st_nmea *n)
{
    st_nmea_gga g;
    memset(&g, 0, sizeof(g));

    size_t      len;
    const char *f;

    if ((f = field_at(n->line, n->line_len, 1, &len)) && len) parse_hms(f, len, &g.time);

    const char *flat = field_at(n->line, n->line_len, 2, &len);
    size_t      lat_len = len;
    f = field_at(n->line, n->line_len, 3, &len);
    g.ns = (f && len) ? f[0] : 0;
    if (flat && lat_len) {
        g.lat_deg = parse_dmm(flat, lat_len);
        if (g.ns == 'S' || g.ns == 's') g.lat_deg = -g.lat_deg;
    }

    const char *flon = field_at(n->line, n->line_len, 4, &len);
    size_t      lon_len = len;
    f = field_at(n->line, n->line_len, 5, &len);
    g.ew = (f && len) ? f[0] : 0;
    if (flon && lon_len) {
        g.lon_deg = parse_dmm(flon, lon_len);
        if (g.ew == 'W' || g.ew == 'w') g.lon_deg = -g.lon_deg;
    }

    if ((f = field_at(n->line, n->line_len, 6, &len)) && len) g.fix_status = f[0];
    if ((f = field_at(n->line, n->line_len, 7, &len)) && len) g.sats_used  = (uint8_t)field_to_int(f, len);
    if ((f = field_at(n->line, n->line_len, 8, &len)) && len) g.hdop       = field_to_double(f, len);
    if ((f = field_at(n->line, n->line_len, 9, &len)) && len) g.altitude_m = field_to_double(f, len);

    n->gga = g;
}

/* GSA field layout:
 *   1  mode 'M' / 'A'
 *   2  fix '1' (none) / '2' (2D) / '3' (3D)
 *   3..14 satellite IDs in use (we ignore)
 *   15  PDOP
 *   16  HDOP
 *   17  VDOP
 */
static void decode_gsa(st_nmea *n)
{
    st_nmea_gsa s;
    memset(&s, 0, sizeof(s));

    size_t      len;
    const char *f;

    if ((f = field_at(n->line, n->line_len, 1,  &len)) && len) s.mode   = f[0];
    if ((f = field_at(n->line, n->line_len, 2,  &len)) && len) s.fix_3d = f[0];
    if ((f = field_at(n->line, n->line_len, 15, &len)) && len) s.pdop   = field_to_double(f, len);
    if ((f = field_at(n->line, n->line_len, 16, &len)) && len) s.hdop   = field_to_double(f, len);
    if ((f = field_at(n->line, n->line_len, 17, &len)) && len) s.vdop   = field_to_double(f, len);

    n->gsa = s;
}

/* ------------------------------------------------------------ */
/* Dispatch                                                     */
/* ------------------------------------------------------------ */

static void dispatch_line(st_nmea *n)
{
    /* Talker ID is 2 chars (GP / GL / GA / GN / BD / ...), then
     * a 3-char message ID, then a comma. So we expect at least
     * "XXyyy," before any field data. */
    if (n->line_len < 6)      return;
    if (n->line[5] != ',')    return;

    const char *id = &n->line[2];   /* "RMC" / "GGA" / "GSA" / ... */
    nmea_msg_type type;

    if      (id[0] == 'R' && id[1] == 'M' && id[2] == 'C') {
        decode_rmc(n);
        type = NMEA_MSG_RMC;
    } else if (id[0] == 'G' && id[1] == 'G' && id[2] == 'A') {
        decode_gga(n);
        type = NMEA_MSG_GGA;
    } else if (id[0] == 'G' && id[1] == 'S' && id[2] == 'A') {
        decode_gsa(n);
        type = NMEA_MSG_GSA;
    } else {
        return;
    }

    if (n->on_message) n->on_message(type, n->cookie);
}

/* ------------------------------------------------------------ */
/* Public entry points                                          */
/* ------------------------------------------------------------ */

void nmea_init(st_nmea *n, nmea_event_fn on, void *cookie)
{
    memset(n, 0, sizeof(*n));
    n->on_message = on;
    n->cookie     = cookie;
    n->state      = NM_WAIT_DOLLAR;
}

void nmea_feed_byte(st_nmea *n, uint8_t b)
{
    switch (n->state) {

    case NM_WAIT_DOLLAR:
        if (b == '$') {
            n->state         = NM_IN_BODY;
            n->line_len      = 0;
            n->overflow      = 0;
            n->computed_csum = 0;
            n->csum_len      = 0;
        }
        break;

    case NM_IN_BODY:
        if (b == '*') {
            n->state = NM_AFTER_STAR;
        } else if (b == '\r' || b == '\n') {
            /* Sentence ended without '*' -- malformed. */
            reset_state(n);
        } else {
            if (n->line_len < NMEA_LINE_MAX) {
                n->line[n->line_len++] = (char)b;
                n->computed_csum     ^= b;
            } else {
                n->overflow = 1;
            }
        }
        break;

    case NM_AFTER_STAR:
        if (b == '\r' || b == '\n') {
            /* CRLF before two csum chars -- malformed. */
            reset_state(n);
        } else if (n->csum_len < 2) {
            n->csum_buf[n->csum_len++] = b;
            if (n->csum_len == 2) n->state = NM_AFTER_CSUM;
        }
        break;

    case NM_AFTER_CSUM:
        /* Bytes other than CR/LF after the checksum are ignored
         * (some receivers stuff a comma + extra). The dispatch
         * fires on the first CR or LF and the state resets. */
        if (b == '\r' || b == '\n') {
            if (!n->overflow) {
                int hi = hex_nibble(n->csum_buf[0]);
                int lo = hex_nibble(n->csum_buf[1]);
                if (hi >= 0 && lo >= 0) {
                    uint8_t expected = (uint8_t)((hi << 4) | lo);
                    if (expected == n->computed_csum) {
                        dispatch_line(n);
                    }
                }
            }
            reset_state(n);
        }
        break;

    default:
        reset_state(n);
        break;
    }
}

size_t nmea_feed(st_nmea *n, const uint8_t *buf, size_t len)
{
    /* Implementation note: returning the number of completed
     * sentences would need an internal counter shared with
     * dispatch_line, which would race if nmea_feed_byte is also
     * being called from an ISR. To keep the byte path safe to
     * call from interrupts without locking, we don't track a
     * count here. Use the event callback for "fired exactly N
     * times" assertions. */
    for (size_t i = 0; i < len; i++) {
        nmea_feed_byte(n, buf[i]);
    }
    return 0;
}

/* ------------------------------------------------------------ */
/* Timezone helper                                              */
/* ------------------------------------------------------------ */

static int leap_year(uint16_t y)
{
    return (y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0));
}

static uint8_t days_in_month(uint8_t month, uint16_t year_full)
{
    static const uint8_t base[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    if (month < 1 || month > 12) return 30;
    if (month == 2) return leap_year(year_full) ? 29 : 28;
    return base[month - 1];
}

void nmea_shift_tz(st_nmea_date utc_date, st_nmea_time utc_time,
                   int16_t tz_min,
                   st_nmea_date *out_date, st_nmea_time *out_time)
{
    /* Accumulate minutes-of-day with the offset, then split out
     * any over- or under-flow into a signed day delta. */
    int32_t total = (int32_t)utc_time.h * 60
                  + (int32_t)utc_time.m
                  + (int32_t)tz_min;
    int32_t day_delta = 0;

    while (total < 0)     { total += 1440; day_delta--; }
    while (total >= 1440) { total -= 1440; day_delta++; }

    st_nmea_time t;
    t.h = (uint8_t)(total / 60);
    t.m = (uint8_t)(total % 60);
    t.s = utc_time.s;

    /* NMEA carries a two-digit year. By convention 70-99 is
     * 1970-1999, 00-69 is 2000-2069. Within either window the
     * Gregorian rule degenerates to "leap iff y % 4 == 0" except
     * for year 1900 (which the convention excludes anyway). */
    uint16_t year = (utc_date.y >= 70)
                  ? (uint16_t)(1900 + utc_date.y)
                  : (uint16_t)(2000 + utc_date.y);

    int32_t day = utc_date.d;
    uint8_t mon = utc_date.m;

    while (day_delta > 0) {
        day++;
        if (day > days_in_month(mon, year)) {
            day = 1;
            mon++;
            if (mon > 12) { mon = 1; year++; }
        }
        day_delta--;
    }
    while (day_delta < 0) {
        day--;
        if (day < 1) {
            mon--;
            if (mon < 1) { mon = 12; year--; }
            day = days_in_month(mon, year);
        }
        day_delta++;
    }

    st_nmea_date d;
    d.d = (uint8_t)day;
    d.m = mon;
    d.y = (uint8_t)(year % 100);

    if (out_date) *out_date = d;
    if (out_time) *out_time = t;
}
