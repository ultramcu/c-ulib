/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 *
 * Sanity-check the intmath helpers against hand-computed values. */

#include <stdio.h>
#include <stdint.h>

#include "../intmath.h"

#define CHECK(expr) do { \
    if (!(expr)) { \
        printf("  ** FAIL: %s  (%s:%d)\n", #expr, __FILE__, __LINE__); \
        fails++; \
    } \
} while (0)

int main(void)
{
    int fails = 0;

    /* int_pow_i32 */
    CHECK(int_pow_i32(2, 0)  == 1);
    CHECK(int_pow_i32(2, 1)  == 2);
    CHECK(int_pow_i32(2, 10) == 1024);
    CHECK(int_pow_i32(10, 5) == 100000);
    CHECK(int_pow_i32(-3, 4) == 81);
    CHECK(int_pow_i32(-3, 3) == -27);
    CHECK(int_pow_i32(7, 0)  == 1);

    /* int_clamp_i32 */
    CHECK(int_clamp_i32( 50, 0, 100) ==  50);
    CHECK(int_clamp_i32(-10, 0, 100) ==   0);
    CHECK(int_clamp_i32(150, 0, 100) == 100);
    CHECK(int_clamp_i32(  0, 0, 100) ==   0);
    CHECK(int_clamp_i32(100, 0, 100) == 100);

    /* int_min / int_max */
    CHECK(int_min_i32(3, 5) == 3);
    CHECK(int_min_i32(5, 3) == 3);
    CHECK(int_min_i32(-1, -2) == -2);
    CHECK(int_max_i32(3, 5) == 5);
    CHECK(int_max_i32(5, 3) == 5);
    CHECK(int_max_i32(-1, -2) == -1);

    /* int_abs */
    CHECK(int_abs_i32( 7) == 7);
    CHECK(int_abs_i32(-7) == 7);
    CHECK(int_abs_i32( 0) == 0);

    if (fails == 0) {
        printf("intmath: All OK.\n");
        return 0;
    }
    printf("intmath: %d FAILURE(S)\n", fails);
    return 1;
}
