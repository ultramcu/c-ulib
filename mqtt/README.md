# mqtt

Minimal MQTT 3.1.1 **client packet builder** for small CPU / MCU
targets. Builds CONNECT, PUBLISH (QoS 0), SUBSCRIBE, UNSUBSCRIBE,
PINGREQ, and DISCONNECT packets into a caller-supplied buffer.
No I/O, no state, no globals.

Depends on the [`byte_stream`](../byte_stream) header in this
umbrella repo.

## API

```c
int mqtt_build_connect    (uint8_t *buf, size_t buf_size,
                           const char *client_id,
                           const char *username,    /* NULL = no auth */
                           const char *password);   /* NULL = no auth */

int mqtt_build_publish    (uint8_t *buf, size_t buf_size,
                           const char *topic,
                           const void *payload, size_t payload_len);

int mqtt_build_subscribe  (uint8_t *buf, size_t buf_size,
                           uint16_t packet_id,
                           const char *topic);

int mqtt_build_unsubscribe(uint8_t *buf, size_t buf_size,
                           uint16_t packet_id,
                           const char *topic);

int mqtt_build_pingreq    (uint8_t *buf, size_t buf_size);

int mqtt_build_disconnect (uint8_t *buf, size_t buf_size);
```

Every builder returns the number of bytes written, or `0` on any
error.

## Defaults

| Setting | Value |
|---|---|
| Protocol | MQTT 3.1.1 (`"MQTT"` / level 4) |
| Keepalive | 60 seconds |
| Clean session | yes |
| QoS | 0 (PUBLISH and SUBSCRIBE) |
| Auth | optional per-call (NULL skips) |

Edit the constants at the top of `mqtt.c` for different defaults.

## Build the self-test

```sh
make test       # from this directory
```

Builds the lib + the demo and compares each packet against
hand-computed reference bytes. Prints `All OK.` on success.

## Use in another project

Copy `mqtt.h`, `mqtt.c`, and the sibling `../byte_stream/byte_stream.h`
into your sources. Add `-I/path/to/byte_stream` to your CFLAGS so
`mqtt.c`'s `#include "byte_stream.h"` resolves.

## Out of scope (intentional)

- Receive-side parsing (broker responses are easy to hand-decode).
- QoS > 0, Last Will, retained messages.
- TLS / sockets / transport.

If you need a fuller MQTT client, [Eclipse Paho's embedded C
client](https://github.com/eclipse/paho.mqtt.embedded-c) is more
complete.

## License

[MIT](../LICENSE).
