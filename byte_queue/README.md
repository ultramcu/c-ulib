# byte_queue

A small caller-buffer-only byte ring queue (FIFO). The queue does
no allocation: you supply the backing buffer and capacity at init
time. Useful for UART RX/TX buffering, command queues, anything
that needs a fixed-size byte FIFO.

## API

```c
#include "byte_queue.h"

uint8_t       backing[256];
st_byte_queue q;

bq_init(&q, backing, sizeof backing);

while (uart_byte_available()) {
    if (!bq_put(&q, uart_read())) {
        /* queue full */
        break;
    }
}

uint8_t v;
while (bq_get(&q, &v)) {
    process(v);
}
```

| Function | Purpose |
|---|---|
| `bq_init(q, buffer, cap)` | Wrap a caller-supplied buffer in a queue. Call again to re-target the queue at different storage. |
| `bq_clear(q)` | Drop every byte (count -> 0). Backing buffer untouched. |
| `bq_count(q)` | Bytes currently in the queue. |
| `bq_capacity(q)` | Maximum bytes the queue can hold. |
| `bq_is_empty(q)` | `1` if empty, `0` otherwise. |
| `bq_is_full(q)` | `1` if full, `0` otherwise. |
| `bq_put(q, v)` | Push one byte. Returns `1` on success, `0` if full. |
| `bq_get(q, &out)` | Pop one byte into `*out`. Returns `1` on success, `0` if empty. |

## Notes

- A separate `count` field disambiguates the empty / full states,
  so the queue can hold its full advertised capacity without the
  classic `head == tail` ambiguity.
- **Not thread-safe / interrupt-safe.** If you `bq_put` from a
  thread and `bq_get` from an ISR (or vice versa), wrap the calls
  in a critical section.

## Build the self-test

```sh
make test       # from this directory
```

Prints `byte_queue: All OK.` on success. The test exercises FIFO
order, full / empty boundaries, wrap-around, and the failure modes
(`put` on full, `get` on empty).

## Use in another project

Drop `byte_queue.h` and `byte_queue.c` into your sources.

## License

[MIT](../LICENSE).
