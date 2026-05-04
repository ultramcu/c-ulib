# c-mqtt-lite

A tiny MQTT 3.1.1 **client packet builder** for small CPU / MCU
targets. ~250 lines of C99, no third-party dependencies.

The library does one thing: builds the most common outgoing MQTT
control packets into a caller-supplied buffer. It does no I/O,
owns no state, has no globals. Send the bytes through whatever
transport your project already has — TCP socket, TLS, UART, USB CDC.

## API

```c
/* CONNECT. `username` and `password` are optional -- pass NULL to skip.
 * Password is ignored if username is NULL. */
int mqtt_build_connect    (uint8_t *buf, size_t buf_size,
                           const char *client_id,
                           const char *username,
                           const char *password);

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
error (NULL/empty required input, buffer too small, length overflow).

## Defaults

Baked in to keep the API tiny. Edit the constants in `mqtt.c` if
you need different values.

| Setting | Value |
|---|---|
| Protocol | MQTT 3.1.1 (`"MQTT"` / level 4) |
| Keepalive | 60 seconds |
| Clean session | yes |
| Will / retained | not used |
| QoS | 0 (PUBLISH and SUBSCRIBE) |
| Auth | optional per-call (NULL skips) |

## Quick example

```c
#include "mqtt.h"

uint8_t buf[256];
int     n;

/* No-auth connect. */
n = mqtt_build_connect(buf, sizeof buf, "node-01", NULL, NULL);
if (n > 0) transport_send(buf, n);

/* Auth connect. */
n = mqtt_build_connect(buf, sizeof buf, "node-01", "alice", "hunter2");
if (n > 0) transport_send(buf, n);

n = mqtt_build_publish(buf, sizeof buf,
                       "sensors/temp", "21.4", 4);
if (n > 0) transport_send(buf, n);

n = mqtt_build_subscribe  (buf, sizeof buf, 1, "cmd/node-01");
if (n > 0) transport_send(buf, n);

n = mqtt_build_unsubscribe(buf, sizeof buf, 2, "cmd/node-01");
if (n > 0) transport_send(buf, n);

n = mqtt_build_pingreq   (buf, sizeof buf);
if (n > 0) transport_send(buf, n);

n = mqtt_build_disconnect(buf, sizeof buf);
if (n > 0) transport_send(buf, n);
```

## Build and run the self-test

```sh
make test
```

That builds the library + the example and runs `build_packets`,
which builds each packet type (including the auth flavour of
CONNECT) and compares the output against hand-computed reference
bytes. A clean run prints `All OK.` and exits zero.

To use the library in another project, drop [`mqtt.h`](mqtt.h) and
[`mqtt.c`](mqtt.c) in alongside your other sources.

## Out of scope (intentional)

- **Receive-side parsing.** Broker responses (CONNACK, PUBACK,
  SUBACK, UNSUBACK, PINGRESP, incoming PUBLISH) are 2–6 bytes plus
  an optional payload — easy to hand-decode where needed.
- **QoS > 0** publishing / subscribing.
- **Last Will and Testament**, retained messages.
- **TLS, sockets, transport.**

If you need a fuller MQTT client, [Eclipse Paho's embedded C
client](https://github.com/eclipse/paho.mqtt.embedded-c) is a more
complete option.

## Protocol correctness

This library implements the MQTT 3.1.1 wire format correctly,
including:

- **Variable-length "Remaining Length" encoding** (1–4 bytes per
  spec §2.2.3), so packets larger than 127 bytes work.
- **SUBSCRIBE packets** with the required 2-byte packet identifier
  followed by the topic filter and a per-topic QoS byte.
- **CONNECT auth flags**: password is honoured only when username
  is set, per `MQTT-3.1.2-22`.
- **Bounds checks** on every write against the caller's buffer
  size; on overflow the builder returns `0` instead of corrupting
  memory.

## License

[MIT](LICENSE) — `SPDX-License-Identifier: MIT`.
