# ujson

A tiny, byte-fed JSON parser for small CPU / MCU targets.

Parses one **flat** object per message of the form:

```
{"key1":"value1", "key2":42, "key3":-7, "key4":"value4"}
```

- **Keys** are quoted strings.
- **Values** can be either a quoted string *or* a signed 32-bit
  integer. The string form is always preserved in `value[i].str`;
  for integers, `value[i].is_numeric` is set and `value[i].i` holds
  the parsed `int32_t`.
- ASCII whitespace is allowed between structural tokens.
- No nested objects, arrays, floats / exponents, booleans, `null`,
  or escape sequences.

## API

```c
#include "ujson.h"

st_ujson_parser parser;

void on_message(st_ujson_parser *p) {
    for (uint8_t i = 0; i < p->pair_count; i++) {
        if (p->value[i].is_numeric) {
            printf("%s = %d (int)\n", p->key[i].str, p->value[i].i);
        } else {
            printf("%s = %s\n",       p->key[i].str, p->value[i].str);
        }
    }
}

ujson_init(&parser, on_message);
while (have_byte()) {
    ujson_feed(&parser, read_byte());
}
```

### Core functions

| Function | Purpose |
|---|---|
| `ujson_init(p, cb)` | Reset the parser, install completion callback (may be `NULL`). |
| `ujson_reset(p)` | Reset state and zero buffers; preserves callback. |
| `ujson_feed(p, b)` | Feed one byte; returns 1 on progress, 0 on parse error. Fires `cb(p)` on `}`. |

### Lookup helpers

| Function | Purpose |
|---|---|
| `ujson_find(p, "name") -> int` | Index of key, or -1. |
| `ujson_get_str(p, "name", &s) -> 1/0` | Sets `*s` to value's string form. |
| `ujson_get_int(p, "name", &out) -> 1/0` | Returns 1 only when the key was a numeric value. |
| `ujson_value_eq(p, "name", "exp")` | 1 if key exists with that exact string value. |

## Footprint

Per-instance memory at default settings:

```
UJSON_MAX_KV * (UJSON_STR_MAX           // key
                + UJSON_STR_MAX + 5)    // value (str + i + is_numeric)
              + UJSON_MSG_MAX
   = 8 * (64 + 64 + 5)  +  256          ≈ 1330 bytes
```

Tune with compile-time `-D` defines:

| Macro | Default | Meaning |
|---|---|---|
| `UJSON_MAX_KV` | 8 | Max key/value pairs per object |
| `UJSON_STR_MAX` | 64 | Max bytes per key / value (incl. NUL) |
| `UJSON_MSG_MAX` | 256 | Raw-message capture buffer (incl. NUL) |

## Build the demo

```sh
make test       # from this directory
```

Builds the lib + the example, runs it, prints `ujson demo OK` if
the run completed cleanly.

## Use in another project

Copy `ujson.h` and `ujson.c` into your sources. Pure C99, no other
dependencies.

## License

[MIT](../LICENSE).
