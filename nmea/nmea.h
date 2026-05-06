/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 *
 * nmea.h -- byte-fed NMEA 0183 parser, lite.
 *
 * Three sentences are decoded: RMC (recommended minimum),
 * GGA (fix data) and GSA (DOPs + 3D fix flag). The parser
 * accepts any talker prefix (GP / GL / GA / GN / BD), so a
 * dual-constellation receiver works without changes.
 *
 * The parser does no I/O, no allocations, and no globals. The
 * caller feeds bytes from a UART one at a time (or in bursts);
 * when a sentence finishes and its checksum verifies, the parser
 * fires a caller-supplied event callback and updates the latest
 * snapshot stored in st_nmea. Old snapshots are preserved until
 * a fresh sentence of the same type lands.
 *
 * Sentences with a bad XOR checksum are dropped silently. Lines
 * longer than NMEA_LINE_MAX bytes are truncated and dropped at
 * the next '*'. The state machine is reset to "wait for $" after
 * every sentence, valid or not, so a noisy line will not poison
 * the next one.
 */

#ifndef NMEA_H
#define NMEA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum length of one sentence body (between '$' and '*',
 * exclusive of both). NMEA spec caps a full sentence at 82
 * including the leading '$' and trailing CRLF, so 96 has
 * headroom for ublox PUBX extensions. */
#ifndef NMEA_LINE_MAX
#define NMEA_LINE_MAX 96
#endif

/* Up to this many comma-separated fields are tracked. RMC has
 * 13, GGA has 14, GSA has 17. */
#ifndef NMEA_FIELD_MAX
#define NMEA_FIELD_MAX 20
#endif

typedef struct { uint8_t h, m, s; } st_nmea_time;
typedef struct { uint8_t d, m, y; } st_nmea_date; /* y is two-digit */

/* RMC -- Recommended Minimum specific GNSS Data. */
typedef struct {
    st_nmea_time time;
    st_nmea_date date;
    char    valid;          /* 'A' active, 'V' void */
    char    pos_mode;       /* 'A' autonomous, 'D' DGPS, 'N' invalid, ... */
    char    ns;             /* 'N' or 'S' (latitude hemisphere) */
    char    ew;             /* 'E' or 'W' (longitude hemisphere) */
    double  lat_deg;        /* degrees decimal, signed (S / W are negative) */
    double  lon_deg;
    double  speed_knots;    /* speed over ground */
    double  course_deg;     /* course over ground, true */
} st_nmea_rmc;

/* GGA -- Global Positioning System Fix Data. */
typedef struct {
    st_nmea_time time;
    char    ns;
    char    ew;
    char    fix_status;     /* '0' none, '1' GPS, '2' DGPS, '6' dead-reckoning */
    double  lat_deg;
    double  lon_deg;
    uint8_t sats_used;
    double  hdop;
    double  altitude_m;
} st_nmea_gga;

/* GSA -- GNSS DOP and active satellites. */
typedef struct {
    char    mode;           /* 'M' manual, 'A' auto */
    char    fix_3d;         /* '1' no fix, '2' 2D, '3' 3D */
    double  pdop, hdop, vdop;
} st_nmea_gsa;

typedef enum {
    NMEA_MSG_RMC = 0,
    NMEA_MSG_GGA = 1,
    NMEA_MSG_GSA = 2
} nmea_msg_type;

typedef void (*nmea_event_fn)(nmea_msg_type type, void *cookie);

/* Engine context. Caller-owned, no allocations inside. */
typedef struct {
    /* Latest verified snapshot of each sentence type. Use the
     * event callback to know when a value just changed; reads
     * here outside a callback can race with the parser. */
    st_nmea_rmc rmc;
    st_nmea_gga gga;
    st_nmea_gsa gsa;

    /* --- internal state below; do not poke --- */
    int      state;
    nmea_event_fn on_message;
    void          *cookie;

    char     line[NMEA_LINE_MAX + 1];   /* +1 for terminator */
    uint16_t line_len;
    uint8_t  overflow;
    uint8_t  computed_csum;
    uint8_t  csum_buf[2];
    uint8_t  csum_len;
} st_nmea;

/* Reset all internal state and store the optional event hook.
 * `on_message` may be NULL -- callers can poll the snapshot
 * structs directly if they prefer. `cookie` is passed through
 * untouched and is otherwise ignored. */
void nmea_init(st_nmea *n, nmea_event_fn on_message, void *cookie);

/* Feed one byte from the GNSS receiver. Safe to call from a UART
 * RX ISR. The library never calls back into UART code itself. */
void nmea_feed_byte(st_nmea *n, uint8_t b);

/* Feed a buffer of `len` bytes. Returns the number of complete,
 * verified sentences emitted by this call (a useful smoke test
 * in init code; drop the return value otherwise). */
size_t nmea_feed(st_nmea *n, const uint8_t *buf, size_t len);

/* Apply a signed timezone offset (in minutes; e.g. Bangkok = 420,
 * India = 330, US Eastern = -300, Nepal = 345) to a UTC date and
 * time pair. Handles both forward and backward day rollover, plus
 * February 29 in leap years. Valid for years 1900-2099 because
 * NMEA's two-digit yy field cannot disambiguate further. Either
 * out_date or out_time may be NULL. */
void nmea_shift_tz(st_nmea_date utc_date, st_nmea_time utc_time,
                   int16_t tz_min,
                   st_nmea_date *out_date, st_nmea_time *out_time);

#ifdef __cplusplus
}
#endif

#endif /* NMEA_H */
