/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 *
 * intmath.h -- header-only integer math helpers.
 *
 * The five functions here cover the everyday cases that come up in
 * sensor / control code: integer power, clamp-to-range, min, max,
 * and absolute value. All are 32-bit signed, all are static inline,
 * no .c file to link.
 *
 * Overflow note:
 *   int_pow_i32 multiplies repeatedly into an int32_t; results
 *   above INT32_MAX wrap modulo 2^32. The function does NOT detect
 *   overflow -- if you need exact arithmetic for big bases, use a
 *   wider type or handle the bound externally.
 */

#ifndef INTMATH_H
#define INTMATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* int_pow_i32 returns base^exponent. exponent == 0 returns 1.
 * exponent == 1 returns base. Result wraps on overflow. */
static inline int32_t int_pow_i32(int32_t base, uint8_t exponent)
{
    int32_t res = 1;
    for (uint8_t i = 0; i < exponent; i++) {
        res *= base;
    }
    return res;
}

/* int_clamp_i32 returns v constrained to [lo, hi]. If lo > hi the
 * result is undefined; the caller should ensure lo <= hi. */
static inline int32_t int_clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline int32_t int_min_i32(int32_t a, int32_t b)
{
    return a < b ? a : b;
}

static inline int32_t int_max_i32(int32_t a, int32_t b)
{
    return a > b ? a : b;
}

/* int_abs_i32 returns |v|. INT32_MIN is unrepresentable as a
 * positive int32_t; this implementation returns INT32_MIN in that
 * case (matches the typical hardware behaviour of a NEG instruction). */
static inline int32_t int_abs_i32(int32_t v)
{
    return v < 0 ? -v : v;
}

#ifdef __cplusplus
}
#endif

#endif /* INTMATH_H */
