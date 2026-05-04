/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 *
 * Self-test: exercises FIFO order, full / empty boundaries,
 * wrap-around, and the failure modes (put on full, get on empty). */

#include <stdint.h>
#include <stdio.h>

#include "../byte_queue.h"

#define CHECK(expr) do { \
    if (!(expr)) { \
        printf("  ** FAIL: %s  (%s:%d)\n", #expr, __FILE__, __LINE__); \
        fails++; \
    } \
} while (0)

int main(void)
{
    int     fails = 0;
    uint8_t buf[4];
    st_byte_queue q;

    bq_init(&q, buf, sizeof buf);

    /* Fresh queue is empty, not full. */
    CHECK(bq_count(&q) == 0);
    CHECK(bq_capacity(&q) == 4);
    CHECK(bq_is_empty(&q));
    CHECK(!bq_is_full(&q));

    /* Get on empty fails cleanly. */
    uint8_t v = 0xAA;
    CHECK(bq_get(&q, &v) == 0);
    CHECK(v == 0xAA);                 /* untouched on failure */

    /* Fill it. */
    CHECK(bq_put(&q, 1));
    CHECK(bq_put(&q, 2));
    CHECK(bq_put(&q, 3));
    CHECK(bq_put(&q, 4));
    CHECK(bq_count(&q) == 4);
    CHECK(bq_is_full(&q));

    /* Put on full fails cleanly. */
    CHECK(bq_put(&q, 99) == 0);
    CHECK(bq_count(&q) == 4);

    /* Drain in FIFO order. */
    CHECK(bq_get(&q, &v) && v == 1);
    CHECK(bq_get(&q, &v) && v == 2);
    CHECK(bq_count(&q) == 2);

    /* Refill triggers wrap-around (head jumps past the end of the
     * backing array and resumes at index 0). */
    CHECK(bq_put(&q, 5));
    CHECK(bq_put(&q, 6));
    CHECK(bq_count(&q) == 4);
    CHECK(bq_is_full(&q));

    /* Drain everything; verify FIFO order across the wrap. */
    CHECK(bq_get(&q, &v) && v == 3);
    CHECK(bq_get(&q, &v) && v == 4);
    CHECK(bq_get(&q, &v) && v == 5);
    CHECK(bq_get(&q, &v) && v == 6);
    CHECK(bq_is_empty(&q));

    /* Clear should leave us with an empty queue regardless of state. */
    CHECK(bq_put(&q, 7));
    CHECK(bq_put(&q, 8));
    bq_clear(&q);
    CHECK(bq_is_empty(&q));
    CHECK(bq_count(&q) == 0);

    if (fails == 0) {
        printf("byte_queue: All OK.\n");
        return 0;
    }
    printf("byte_queue: %d FAILURE(S)\n", fails);
    return 1;
}
