/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 *
 * Demo + self-test. Builds each packet type, prints a hex dump,
 * and compares against a hand-computed reference so this example
 * doubles as `make test`.
 */

#include <stdio.h>
#include <string.h>

#include "../mqtt.h"

static void hexdump(const char *label, const uint8_t *buf, int len)
{
    printf("%-13s (%2d bytes)  ", label, len);
    for (int i = 0; i < len; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\n");
}

static int check(const char *label,
                 const uint8_t *got, int got_len,
                 const uint8_t *want, int want_len)
{
    if (got_len != want_len || memcmp(got, want, (size_t)got_len) != 0) {
        printf("  ** %s MISMATCH (expected %d bytes)\n", label, want_len);
        return 1;
    }
    return 0;
}

int main(void)
{
    uint8_t buf[256];
    int     n;
    int     fails = 0;

    /* CONNECT(client_id="test") -- no auth */
    static const uint8_t want_connect[] = {
        0x10, 0x10,
        0x00, 0x04, 'M', 'Q', 'T', 'T',
        0x04, 0x02, 0x00, 0x3C,
        0x00, 0x04, 't', 'e', 's', 't',
    };
    n = mqtt_build_connect(buf, sizeof buf, "test", NULL, NULL);
    hexdump("CONNECT", buf, n);
    fails += check("CONNECT", buf, n, want_connect, (int)sizeof want_connect);

    /* CONNECT(client_id="test", username="alice", password="hunter2") */
    static const uint8_t want_connect_auth[] = {
        0x10, 0x20,                           /* CONNECT, RL=32 */
        0x00, 0x04, 'M', 'Q', 'T', 'T',
        0x04, 0xC2,                           /* level 4, flags = clean+user+pass */
        0x00, 0x3C,                           /* keepalive 60 */
        0x00, 0x04, 't', 'e', 's', 't',       /* client id "test" */
        0x00, 0x05, 'a', 'l', 'i', 'c', 'e',  /* username "alice" */
        0x00, 0x07, 'h', 'u', 'n', 't', 'e', 'r', '2',  /* password */
    };
    n = mqtt_build_connect(buf, sizeof buf, "test", "alice", "hunter2");
    hexdump("CONNECT auth", buf, n);
    fails += check("CONNECT auth", buf, n,
                   want_connect_auth, (int)sizeof want_connect_auth);

    /* PUBLISH(topic="topic", payload="msg") */
    static const uint8_t want_publish[] = {
        0x30, 0x0A,
        0x00, 0x05, 't', 'o', 'p', 'i', 'c',
        'm', 's', 'g',
    };
    n = mqtt_build_publish(buf, sizeof buf, "topic", "msg", 3);
    hexdump("PUBLISH", buf, n);
    fails += check("PUBLISH", buf, n, want_publish, (int)sizeof want_publish);

    /* SUBSCRIBE(packet_id=1, topic="topic") -- QoS 0 */
    static const uint8_t want_subscribe[] = {
        0x82, 0x0A,
        0x00, 0x01,
        0x00, 0x05, 't', 'o', 'p', 'i', 'c',
        0x00,
    };
    n = mqtt_build_subscribe(buf, sizeof buf, 1, "topic");
    hexdump("SUBSCRIBE", buf, n);
    fails += check("SUBSCRIBE", buf, n, want_subscribe, (int)sizeof want_subscribe);

    /* UNSUBSCRIBE(packet_id=2, topic="topic") */
    static const uint8_t want_unsubscribe[] = {
        0xA2, 0x09,
        0x00, 0x02,
        0x00, 0x05, 't', 'o', 'p', 'i', 'c',
    };
    n = mqtt_build_unsubscribe(buf, sizeof buf, 2, "topic");
    hexdump("UNSUBSCRIBE", buf, n);
    fails += check("UNSUBSCRIBE", buf, n,
                   want_unsubscribe, (int)sizeof want_unsubscribe);

    /* PINGREQ */
    static const uint8_t want_pingreq[] = { 0xC0, 0x00 };
    n = mqtt_build_pingreq(buf, sizeof buf);
    hexdump("PINGREQ", buf, n);
    fails += check("PINGREQ", buf, n, want_pingreq, (int)sizeof want_pingreq);

    /* DISCONNECT */
    static const uint8_t want_disconnect[] = { 0xE0, 0x00 };
    n = mqtt_build_disconnect(buf, sizeof buf);
    hexdump("DISCONNECT", buf, n);
    fails += check("DISCONNECT", buf, n,
                   want_disconnect, (int)sizeof want_disconnect);

    if (fails == 0) {
        printf("All OK.\n");
        return 0;
    }
    printf("%d FAILURE(S)\n", fails);
    return 1;
}
