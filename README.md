# c-ulib

A small collection of header-and-`.c`-pair libraries for small CPU
/ MCU C projects. Each module is independent — drop the files for
the one you need into your project, ignore the rest. Pure C99, no
third-party dependencies.

## Modules

| Module | What it does | Files |
|---|---|---|
| [`byte_queue`](byte_queue) | Caller-buffer-only byte ring queue (FIFO). Separate count field disambiguates full / empty so the queue uses every slot. | `byte_queue.h`, `byte_queue.c` |
| [`byte_stream`](byte_stream) | Header-only helpers for sequential read/write of primitive types in a byte buffer. `u8`/`u16`/`u32` (BE + LE) plus signed wrappers and length-prefixed strings. | `byte_stream.h` |
| [`math`](math) | Header-only integer helpers: `int_pow_i32`, `int_clamp_i32`, `int_min/max/abs_i32`. | `intmath.h` |
| [`mqtt`](mqtt) | Minimal MQTT 3.1.1 client packet builder: CONNECT (with optional username/password), PUBLISH, SUBSCRIBE, UNSUBSCRIBE, PINGREQ, DISCONNECT. Caller-buffer only, no I/O. Uses `byte_stream`. | `mqtt.h`, `mqtt.c` (+ `byte_stream.h`) |
| [`stepper`](stepper) | Step-motor position controller. STEP/DIR/EN pin callbacks, signed `int32_t` positions, multi-instance safe. | `stepper.h`, `stepper.c` |
| [`ujson`](ujson) | Byte-fed JSON parser for one flat object per message. Quoted-string keys, string or int32 values, with lookup helpers (`ujson_get_str`, `ujson_get_int`, `ujson_value_eq`). | `ujson.h`, `ujson.c` |
| [`astro`](astro) | Minimal astronomy: HMS / DMS conversions, mean sidereal time (IAU 1982), and the equatorial-to-horizon transform. No precession or nutation; ~0.01° accuracy. Links with `-lm`. | `astro.h`, `astro.c` |
| [`nmea`](nmea) | Byte-fed NMEA 0183 parser for RMC / GGA / GSA. XOR-checksum verified, decoded lat / lon in signed degrees, signed timezone shift with leap-year and back-rollover. Links with `-lm`. | `nmea.h`, `nmea.c` |

Each module's `README.md` has its full API and a build target. Run
all of them from the top of the repo:

```sh
make test
```

Or one at a time from the module directory:

```sh
cd math       && make test     # intmath sanity check
cd byte_queue && make test     # byte ring-queue self-test
cd mqtt       && make test     # MQTT packet build self-test
cd stepper    && make test     # stepper controller self-test
cd ujson      && make test     # ujson demo
cd astro      && make test     # astro math self-test
cd nmea       && make test     # NMEA 0183 parser self-test
```

## Design rules

These are intentional and shared across every module:

- **Caller owns the memory.** No global scratch, no hidden allocations.
- **No transport.** Modules build/parse bytes; the caller moves them.
- **Single-pass, byte-fed where parsing is involved.** Suitable for
  UART / TCP byte streams without intermediate buffering.
- **Small.** Every module is well under 300 lines of source.

If you need richer functionality (full MQTT 3/5 client, full RFC
8259 JSON, etc.) there are good public libraries — these aren't
trying to compete.

## Use in another project

Pick the module(s) you need and copy their files into your project.
Two notes about cross-module deps:

- `mqtt` includes `byte_stream.h`. If you take `mqtt`, take
  `byte_stream/byte_stream.h` along with it (and add an include
  path so `#include "byte_stream.h"` resolves).
- Every other module is fully standalone.

## License

[MIT](LICENSE) — `SPDX-License-Identifier: MIT`. Drop the files
into any project (open or closed, commercial or otherwise).
