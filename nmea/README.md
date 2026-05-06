# nmea

Byte-fed parser for the three NMEA 0183 sentences that cover
99% of GPS / GNSS application needs: RMC (recommended minimum),
GGA (fix data), and GSA (DOPs and 3D fix). Pure C99, link with
`-lm`.

The parser is single-pass and byte-fed — it accepts characters
straight from the UART RX path, one at a time or in bursts. When
a complete sentence arrives and its XOR checksum verifies, it
fires a caller-supplied event callback and updates the latest
snapshot stored in `st_nmea`. Bad checksums and truncated lines
are dropped silently; a noisy stream cannot poison the next
sentence.

```c
#include "nmea.h"

static void on_msg(nmea_msg_type t, void *cookie)
{
    st_nmea *n = cookie;
    if (t == NMEA_MSG_RMC && n->rmc.valid == 'A') {
        printf("fix at %f, %f\n", n->rmc.lat_deg, n->rmc.lon_deg);
    }
}

st_nmea n;
nmea_init(&n, on_msg, &n);

/* In the UART RX ISR (or polling loop): */
nmea_feed_byte(&n, byte_from_uart);
```

## Functions

| Function | Purpose |
|---|---|
| `nmea_init(n, on_msg, cookie)` | Reset all state and store the optional event callback. |
| `nmea_feed_byte(n, b)` | Push one received byte. Safe to call from a UART RX ISR. |
| `nmea_feed(n, buf, len)` | Push `len` bytes in one go. |
| `nmea_shift_tz(date, time, tz_min, *out_date, *out_time)` | Apply a signed timezone offset (in minutes — Bangkok = 420, India = 330, US Eastern = -300, Nepal = 345) to a UTC date / time pair. Both forward and backward day rollover are handled, including leap-year February. |

## Decoded fields

All angles are in **decimal degrees**, signed (south and west are
negative). All times are UTC unless you have already applied
`nmea_shift_tz`.

```c
typedef struct {                /* RMC -- recommended minimum */
    st_nmea_time time;
    st_nmea_date date;
    char    valid;              /* 'A' active, 'V' void */
    char    pos_mode;
    char    ns, ew;
    double  lat_deg, lon_deg;
    double  speed_knots;
    double  course_deg;
} st_nmea_rmc;

typedef struct {                /* GGA -- fix data */
    st_nmea_time time;
    char    ns, ew;
    char    fix_status;         /* '0' none .. '6' dead-reckoning */
    double  lat_deg, lon_deg;
    uint8_t sats_used;
    double  hdop;
    double  altitude_m;
} st_nmea_gga;

typedef struct {                /* GSA -- DOPs and 3D fix */
    char    mode;               /* 'M' manual, 'A' auto */
    char    fix_3d;             /* '1' none, '2' 2D, '3' 3D */
    double  pdop, hdop, vdop;
} st_nmea_gsa;
```

The talker prefix is ignored, so `$GPRMC`, `$GLRMC`, `$GNRMC`
and so on are all accepted — useful for dual-constellation
receivers (u-blox NEO-M8N etc.) that emit `$GN…` lines.

Sentences other than RMC / GGA / GSA are silently skipped. The
parser does not allocate, log, or call back into your UART
code; the only output path is the event callback you registered
in `nmea_init`.

## Build the self-test

```sh
make test
```

Builds `example/check.c`, runs it, prints `nmea: All OK.` on
success and exits zero. The check covers:

- Happy-path decoding for RMC / GGA / GSA against reference
  numbers from u-blox docs (lat/lon at 1e-5 deg, speed / DOPs
  at 1e-3).
- Negative-hemisphere decoding (Sao Paulo, lat / lon < 0).
- A bad-XOR-checksum sentence is dropped (no callback fired).
- A buffer carrying two sentences back-to-back fires two callbacks.
- Junk bytes before the leading `$` are ignored.
- `nmea_shift_tz` rollover: forward across leap-year Feb 29,
  forward across non-leap Feb 28 → March 1, backward across the
  new-year boundary, and minute-precision offsets (India +05:30).

## Use in another project

Drop `nmea.h` and `nmea.c` into your sources, link with `-lm`.
No other module dependencies.

## License

[MIT](../LICENSE).
