# c-ulib

A small collection of header-and-`.c`-pair libraries for small CPU
/ MCU C projects. Each module is independent — drop the files for
the one you need into your project, ignore the rest. Pure C99, no
third-party dependencies.

## Modules

| Module | What it does | Files |
|---|---|---|
| [`byte_stream`](byte_stream) | Header-only helpers for sequential read/write of primitive types in a byte buffer. `u8`/`u16`/`u32` (BE + LE) plus signed wrappers and length-prefixed strings. | `byte_stream.h` |
| [`mqtt`](mqtt) | Minimal MQTT 3.1.1 client packet builder: CONNECT (with optional username/password), PUBLISH, SUBSCRIBE, UNSUBSCRIBE, PINGREQ, DISCONNECT. Caller-buffer only, no I/O. Uses `byte_stream`. | `mqtt.h`, `mqtt.c` (+ `byte_stream.h`) |
| [`ujson`](ujson) | Byte-fed JSON parser for one flat object per message. Quoted-string keys, string or int32 values, with lookup helpers (`ujson_get_str`, `ujson_get_int`, `ujson_value_eq`). | `ujson.h`, `ujson.c` |

Each module's `README.md` has its full API and a build target. Run
the per-module self-test from the module directory:

```sh
cd mqtt  &&  make test       # MQTT packet build self-test
cd ujson &&  make test       # ujson demo
```

Or run them all from the top of the repo:

```sh
make test
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
- `ujson` is fully standalone — no other module needed.

## License

[MIT](LICENSE) — `SPDX-License-Identifier: MIT`. Drop the files
into any project (open or closed, commercial or otherwise).
