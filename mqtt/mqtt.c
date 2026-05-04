/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 */

#include "mqtt.h"

#include "byte_stream.h"

#define MQTT_KEEPALIVE_S    60
#define MQTT_FLAG_CLEAN_SES 0x02

/* Write the MQTT "Remaining Length" varint. 1..4 bytes; max value
 * 268,435,455. Returns bytes written, or 0 on overflow. */
static int put_varint(uint8_t *out, uint32_t value)
{
    int n = 0;
    do {
        uint8_t byte = (uint8_t)(value & 0x7Fu);
        value >>= 7;
        if (value > 0) byte |= 0x80u;
        out[n++] = byte;
    } while (value > 0 && n < 4);
    return value > 0 ? 0 : n;
}

/* Bytes the varint will use for `value`. */
static int varint_size(uint32_t value)
{
    if (value < 128u)        return 1;
    if (value < 16384u)      return 2;
    if (value < 2097152u)    return 3;
    if (value < 268435456u)  return 4;
    return 0;
}

int mqtt_build_connect(uint8_t *buf, size_t buf_size,
                       const char *client_id,
                       const char *username,
                       const char *password)
{
    if (buf == NULL || client_id == NULL || client_id[0] == '\0') {
        return 0;
    }
    size_t cid_len = strlen(client_id);
    if (cid_len > 0xFFFFu) {
        return 0;
    }

    /* Resolve auth: password is only honoured when username is set
     * (per MQTT 3.1.1 spec MQTT-3.1.2-22). Empty-string username is
     * treated the same as NULL. */
    size_t un_len = 0, pw_len = 0;
    if (username != NULL && username[0] != '\0') {
        un_len = strlen(username);
        if (un_len > 0xFFFFu) return 0;
        if (password != NULL) {
            pw_len = strlen(password);
            if (pw_len > 0xFFFFu) return 0;
        }
    } else {
        username = NULL;
        password = NULL;
    }

    /* Body layout for MQTT 3.1.1 with no will:
     *   00 04 'M' 'Q' 'T' 'T'      protocol name
     *   04                          protocol level
     *   FL                          connect flags
     *   00 3C                       keepalive (60s)
     *   NN NN <client_id>           client id
     *   [NN NN <username>]          if username flag set
     *   [NN NN <password>]          if password flag set */
    uint8_t flags = MQTT_FLAG_CLEAN_SES;
    if (username != NULL) {
        flags |= 0x80;
        if (password != NULL) flags |= 0x40;
    }

    size_t body_len = 12u + cid_len
                    + (username ? 2u + un_len : 0u)
                    + (password ? 2u + pw_len : 0u);
    int    rl_size  = varint_size((uint32_t)body_len);
    if (rl_size == 0) return 0;

    size_t total = 1u + (size_t)rl_size + body_len;
    if (total > buf_size) return 0;

    size_t i = 0;
    bs_put_u8(buf, &i, 0x10);                                  /* CONNECT */
    i += (size_t)put_varint(&buf[i], (uint32_t)body_len);

    bs_put_str_u16_be(buf, &i, "MQTT", 4);                     /* proto name */
    bs_put_u8(buf, &i, 0x04);                                  /* level 4 */
    bs_put_u8(buf, &i, flags);
    bs_put_u16_be(buf, &i, MQTT_KEEPALIVE_S);                  /* keepalive */
    bs_put_str_u16_be(buf, &i, client_id, cid_len);

    if (username != NULL) bs_put_str_u16_be(buf, &i, username, un_len);
    if (password != NULL) bs_put_str_u16_be(buf, &i, password, pw_len);

    return (int)i;
}

int mqtt_build_publish(uint8_t *buf, size_t buf_size,
                       const char *topic,
                       const void *payload, size_t payload_len)
{
    if (buf == NULL || topic == NULL || topic[0] == '\0') {
        return 0;
    }
    if (payload == NULL && payload_len > 0) {
        return 0;
    }
    size_t topic_len = strlen(topic);
    if (topic_len > 0xFFFFu) {
        return 0;
    }

    /* Body: topic (length-prefixed) + payload (raw, no prefix). */
    size_t body_len = 2u + topic_len + payload_len;
    int    rl_size  = varint_size((uint32_t)body_len);
    if (rl_size == 0) return 0;

    size_t total = 1u + (size_t)rl_size + body_len;
    if (total > buf_size) return 0;

    size_t i = 0;
    bs_put_u8(buf, &i, 0x30);                                  /* PUBLISH q0 */
    i += (size_t)put_varint(&buf[i], (uint32_t)body_len);

    bs_put_str_u16_be(buf, &i, topic, topic_len);
    bs_put_bytes(buf, &i, payload, payload_len);

    return (int)i;
}

int mqtt_build_subscribe(uint8_t *buf, size_t buf_size,
                         uint16_t packet_id,
                         const char *topic)
{
    if (buf == NULL || topic == NULL || topic[0] == '\0') {
        return 0;
    }
    if (packet_id == 0) {
        return 0;
    }
    size_t topic_len = strlen(topic);
    if (topic_len > 0xFFFFu) {
        return 0;
    }

    /* Body:
     *   NN NN                packet id
     *   NN NN <topic>        topic filter (length-prefixed)
     *   00                   requested QoS (0) */
    size_t body_len = 2u + 2u + topic_len + 1u;
    int    rl_size  = varint_size((uint32_t)body_len);
    if (rl_size == 0) return 0;

    size_t total = 1u + (size_t)rl_size + body_len;
    if (total > buf_size) return 0;

    size_t i = 0;
    bs_put_u8(buf, &i, 0x82);                                  /* SUBSCRIBE */
    i += (size_t)put_varint(&buf[i], (uint32_t)body_len);

    bs_put_u16_be(buf, &i, packet_id);
    bs_put_str_u16_be(buf, &i, topic, topic_len);
    bs_put_u8(buf, &i, 0x00);                                  /* QoS 0 */

    return (int)i;
}

int mqtt_build_unsubscribe(uint8_t *buf, size_t buf_size,
                           uint16_t packet_id,
                           const char *topic)
{
    if (buf == NULL || topic == NULL || topic[0] == '\0') {
        return 0;
    }
    if (packet_id == 0) {
        return 0;
    }
    size_t topic_len = strlen(topic);
    if (topic_len > 0xFFFFu) {
        return 0;
    }

    /* Body: packet id + length-prefixed topic. */
    size_t body_len = 2u + 2u + topic_len;
    int    rl_size  = varint_size((uint32_t)body_len);
    if (rl_size == 0) return 0;

    size_t total = 1u + (size_t)rl_size + body_len;
    if (total > buf_size) return 0;

    size_t i = 0;
    bs_put_u8(buf, &i, 0xA2);                                  /* UNSUBSCRIBE */
    i += (size_t)put_varint(&buf[i], (uint32_t)body_len);

    bs_put_u16_be(buf, &i, packet_id);
    bs_put_str_u16_be(buf, &i, topic, topic_len);

    return (int)i;
}

int mqtt_build_pingreq(uint8_t *buf, size_t buf_size)
{
    if (buf == NULL || buf_size < 2u) return 0;
    buf[0] = 0xC0;
    buf[1] = 0x00;
    return 2;
}

int mqtt_build_disconnect(uint8_t *buf, size_t buf_size)
{
    if (buf == NULL || buf_size < 2u) return 0;
    buf[0] = 0xE0;
    buf[1] = 0x00;
    return 2;
}
