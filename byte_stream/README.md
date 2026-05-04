# byte_stream

Header-only helpers for sequential read/write of primitive types
into / out of a byte buffer with a moving offset. Pure C99, no
dependencies, no `.c` file to compile.

```c
#include "byte_stream.h"

size_t  off = 0;
uint8_t buf[64];

bs_put_u8        (buf, &off, 0xAB);
bs_put_u16_be    (buf, &off, 1234);            /* network byte order */
bs_put_u32_le    (buf, &off, 0xDEADBEEFu);     /* on-disk formats    */
bs_put_i32_be    (buf, &off, -1);              /* signed wrappers    */
bs_put_bytes     (buf, &off, "hello", 5);
bs_put_str_u16_be(buf, &off, "topic", 5);      /* 2-byte length + bytes */

off = 0;
uint8_t  a = bs_get_u8(buf, &off);
uint16_t b = bs_get_u16_be(buf, &off);
```

## Functions

| Width | Big-endian | Little-endian | Signed wrapper |
|---|---|---|---|
| 8-bit | `bs_put_u8` / `bs_get_u8` | (n/a) | `bs_put_i8` / `bs_get_i8` |
| 16-bit | `bs_put_u16_be` / `bs_get_u16_be` | `bs_put_u16_le` / `bs_get_u16_le` | `bs_put_i16_*` / `bs_get_i16_*` |
| 32-bit | `bs_put_u32_be` / `bs_get_u32_be` | `bs_put_u32_le` / `bs_get_u32_le` | `bs_put_i32_*` / `bs_get_i32_*` |

Plus:

- `bs_put_bytes(buf, &off, src, len)`, `bs_get_bytes(dst, buf, &off, len)`
- `bs_put_str_u8(buf, &off, s, len)` — 1-byte length + bytes
- `bs_put_str_u16_be(buf, &off, s, len)` — 2-byte BE length + bytes
  (the form used by MQTT, NATS, and most network text protocols)

## Bounds

None of these helpers check `off + N` against a buffer size — same
contract as `memcpy`. Pair them with whatever
`if (off + N > sz) return 0;` style guard makes sense for your code.

## Use in another project

Drop the single header (`byte_stream.h`) into your sources and
`#include "byte_stream.h"`. There is no library to link.

## License

[MIT](../LICENSE).
