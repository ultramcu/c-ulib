/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 *
 * mqtt.h -- minimal MQTT 3.1.1 client packet builder.
 *
 * Builds CONNECT, PUBLISH (QoS 0), SUBSCRIBE (QoS 0), UNSUBSCRIBE,
 * PINGREQ and DISCONNECT packets into a caller-supplied buffer.
 * No I/O, no state, no globals, no third-party deps -- you get back
 * a length, you write the bytes to whatever transport you have.
 *
 * Defaults are baked in for the lightweight use case:
 *   MQTT 3.1.1 ("MQTT" / level 4), 60-second keepalive, clean
 *   session, QoS 0 throughout. Username/password are optional per
 *   call (pass NULL to skip). Edit the constants in mqtt.c if you
 *   need different defaults.
 *
 * Every builder returns the number of bytes written, or 0 on any
 * error (NULL/empty required input, buffer too small, length
 * overflow).
 */

#ifndef MQTT_H
#define MQTT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONNECT. `username` and `password` are optional -- pass NULL to
 * skip. Password is ignored if username is NULL. */
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

#ifdef __cplusplus
}
#endif

#endif /* MQTT_H */
