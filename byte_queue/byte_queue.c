/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd */

#include "byte_queue.h"

void bq_init(st_byte_queue *q, uint8_t *buffer, size_t capacity)
{
    if (q == NULL) return;
    q->data     = buffer;
    q->capacity = capacity;
    q->head     = 0;
    q->tail     = 0;
    q->count    = 0;
}

void bq_clear(st_byte_queue *q)
{
    if (q == NULL) return;
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
}

size_t bq_count(const st_byte_queue *q)
{
    return q != NULL ? q->count : 0;
}

size_t bq_capacity(const st_byte_queue *q)
{
    return q != NULL ? q->capacity : 0;
}

int bq_is_empty(const st_byte_queue *q)
{
    return q == NULL || q->count == 0;
}

int bq_is_full(const st_byte_queue *q)
{
    return q != NULL && q->count >= q->capacity;
}

int bq_put(st_byte_queue *q, uint8_t v)
{
    if (q == NULL || q->data == NULL || q->capacity == 0) {
        return 0;
    }
    if (q->count >= q->capacity) {
        return 0;
    }
    q->data[q->head] = v;
    q->head = (q->head + 1u) % q->capacity;
    q->count++;
    return 1;
}

int bq_get(st_byte_queue *q, uint8_t *out)
{
    if (q == NULL || q->data == NULL || q->count == 0) {
        return 0;
    }
    if (out != NULL) {
        *out = q->data[q->tail];
    }
    q->tail = (q->tail + 1u) % q->capacity;
    q->count--;
    return 1;
}
