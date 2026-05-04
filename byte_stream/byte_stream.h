/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 *
 * byte_stream.h -- header-only helpers for sequential read/write of
 * primitive types into/out of a byte buffer with a moving offset.
 *
 * Big-endian (MQTT and most network protocols) and little-endian
 * (most on-disk formats) variants are both provided. Signed types
 * are thin casts over the unsigned ones, so all width/order pairs
 * are uniformly available.
 *
 * Drop this single header into any C project that needs the same.
 *
 * Bounds:
 *   None of these helpers check (off + N) against a buffer size --
 *   that's the caller's job, exactly the way `memcpy` works. Pair
 *   them with whatever `if (off + N > sz) return 0;` style guard
 *   makes sense for your code.
 */

#ifndef BYTE_STREAM_H
#define BYTE_STREAM_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------- */
/* 8-bit                                                             */
/* ---------------------------------------------------------------- */

static inline void bs_put_u8(uint8_t *buf, size_t *off, uint8_t v)
{
    buf[(*off)++] = v;
}

static inline uint8_t bs_get_u8(const uint8_t *buf, size_t *off)
{
    return buf[(*off)++];
}

static inline void bs_put_i8(uint8_t *buf, size_t *off, int8_t v)
{
    bs_put_u8(buf, off, (uint8_t)v);
}

static inline int8_t bs_get_i8(const uint8_t *buf, size_t *off)
{
    return (int8_t)bs_get_u8(buf, off);
}

/* ---------------------------------------------------------------- */
/* 16-bit  big-endian (network byte order)                            */
/* ---------------------------------------------------------------- */

static inline void bs_put_u16_be(uint8_t *buf, size_t *off, uint16_t v)
{
    buf[(*off)++] = (uint8_t)((v >> 8) & 0xFFu);
    buf[(*off)++] = (uint8_t)( v       & 0xFFu);
}

static inline uint16_t bs_get_u16_be(const uint8_t *buf, size_t *off)
{
    uint16_t v = (uint16_t)buf[(*off)++] << 8;
    v |= (uint16_t)buf[(*off)++];
    return v;
}

static inline void bs_put_i16_be(uint8_t *buf, size_t *off, int16_t v)
{
    bs_put_u16_be(buf, off, (uint16_t)v);
}

static inline int16_t bs_get_i16_be(const uint8_t *buf, size_t *off)
{
    return (int16_t)bs_get_u16_be(buf, off);
}

/* ---------------------------------------------------------------- */
/* 16-bit  little-endian                                              */
/* ---------------------------------------------------------------- */

static inline void bs_put_u16_le(uint8_t *buf, size_t *off, uint16_t v)
{
    buf[(*off)++] = (uint8_t)( v       & 0xFFu);
    buf[(*off)++] = (uint8_t)((v >> 8) & 0xFFu);
}

static inline uint16_t bs_get_u16_le(const uint8_t *buf, size_t *off)
{
    uint16_t v = (uint16_t)buf[(*off)++];
    v |= (uint16_t)buf[(*off)++] << 8;
    return v;
}

static inline void bs_put_i16_le(uint8_t *buf, size_t *off, int16_t v)
{
    bs_put_u16_le(buf, off, (uint16_t)v);
}

static inline int16_t bs_get_i16_le(const uint8_t *buf, size_t *off)
{
    return (int16_t)bs_get_u16_le(buf, off);
}

/* ---------------------------------------------------------------- */
/* 32-bit  big-endian                                                 */
/* ---------------------------------------------------------------- */

static inline void bs_put_u32_be(uint8_t *buf, size_t *off, uint32_t v)
{
    buf[(*off)++] = (uint8_t)((v >> 24) & 0xFFu);
    buf[(*off)++] = (uint8_t)((v >> 16) & 0xFFu);
    buf[(*off)++] = (uint8_t)((v >>  8) & 0xFFu);
    buf[(*off)++] = (uint8_t)( v        & 0xFFu);
}

static inline uint32_t bs_get_u32_be(const uint8_t *buf, size_t *off)
{
    uint32_t v = (uint32_t)buf[(*off)++] << 24;
    v |= (uint32_t)buf[(*off)++] << 16;
    v |= (uint32_t)buf[(*off)++] <<  8;
    v |= (uint32_t)buf[(*off)++];
    return v;
}

static inline void bs_put_i32_be(uint8_t *buf, size_t *off, int32_t v)
{
    bs_put_u32_be(buf, off, (uint32_t)v);
}

static inline int32_t bs_get_i32_be(const uint8_t *buf, size_t *off)
{
    return (int32_t)bs_get_u32_be(buf, off);
}

/* ---------------------------------------------------------------- */
/* 32-bit  little-endian                                              */
/* ---------------------------------------------------------------- */

static inline void bs_put_u32_le(uint8_t *buf, size_t *off, uint32_t v)
{
    buf[(*off)++] = (uint8_t)( v        & 0xFFu);
    buf[(*off)++] = (uint8_t)((v >>  8) & 0xFFu);
    buf[(*off)++] = (uint8_t)((v >> 16) & 0xFFu);
    buf[(*off)++] = (uint8_t)((v >> 24) & 0xFFu);
}

static inline uint32_t bs_get_u32_le(const uint8_t *buf, size_t *off)
{
    uint32_t v = (uint32_t)buf[(*off)++];
    v |= (uint32_t)buf[(*off)++] <<  8;
    v |= (uint32_t)buf[(*off)++] << 16;
    v |= (uint32_t)buf[(*off)++] << 24;
    return v;
}

static inline void bs_put_i32_le(uint8_t *buf, size_t *off, int32_t v)
{
    bs_put_u32_le(buf, off, (uint32_t)v);
}

static inline int32_t bs_get_i32_le(const uint8_t *buf, size_t *off)
{
    return (int32_t)bs_get_u32_le(buf, off);
}

/* ---------------------------------------------------------------- */
/* Raw bytes / strings                                                */
/* ---------------------------------------------------------------- */

/* Append `len` bytes from `src` starting at *off; advance *off. */
static inline void bs_put_bytes(uint8_t *buf, size_t *off,
                                const void *src, size_t len)
{
    if (len > 0) memcpy(&buf[*off], src, len);
    *off += len;
}

/* Read `len` bytes from buf+*off into dst; advance *off. */
static inline void bs_get_bytes(void *dst, const uint8_t *buf,
                                size_t *off, size_t len)
{
    if (len > 0) memcpy(dst, &buf[*off], len);
    *off += len;
}

/* Length-prefixed string (1-byte length + bytes), useful for short
 * tag/value formats. `len` is silently truncated to 255. */
static inline void bs_put_str_u8(uint8_t *buf, size_t *off,
                                 const char *s, size_t len)
{
    bs_put_u8(buf, off, (uint8_t)(len & 0xFFu));
    bs_put_bytes(buf, off, s, len);
}

/* Length-prefixed string (2-byte big-endian length + bytes). This is
 * the form MQTT, NATS protocol fields, and most network text use. */
static inline void bs_put_str_u16_be(uint8_t *buf, size_t *off,
                                     const char *s, size_t len)
{
    bs_put_u16_be(buf, off, (uint16_t)(len & 0xFFFFu));
    bs_put_bytes(buf, off, s, len);
}

#ifdef __cplusplus
}
#endif

#endif /* BYTE_STREAM_H */
