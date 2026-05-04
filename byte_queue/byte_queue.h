/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 MaIII Themd
 *
 * byte_queue.h -- caller-buffer-only byte ring queue (FIFO).
 *
 * The queue does no allocation: the caller supplies a backing
 * byte buffer and a capacity at init time. Storage cost is one
 * st_byte_queue (4 size_t fields) plus the user's buffer. No I/O,
 * no globals, no third-party deps.
 *
 * Goroutine / interrupt safety:
 *   The queue is NOT internally locked. If you put from a thread
 *   and get from an interrupt, wrap calls in your own critical
 *   section.
 */

#ifndef BYTE_QUEUE_H
#define BYTE_QUEUE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *data;
    size_t   capacity;
    size_t   head;     /* next write slot */
    size_t   tail;     /* next read slot  */
    size_t   count;    /* items currently in the queue */
} st_byte_queue;

/* Initialise a queue around a caller-supplied buffer of `capacity`
 * bytes. The buffer is owned by the caller; do not free it while
 * the queue is in use. Re-using an existing st_byte_queue with
 * different storage is allowed -- just call bq_init again. */
void   bq_init      (st_byte_queue *q, uint8_t *buffer, size_t capacity);

/* Drop every queued byte (count -> 0). The backing buffer is left
 * untouched. */
void   bq_clear     (st_byte_queue *q);

/* Number of bytes currently in the queue. */
size_t bq_count     (const st_byte_queue *q);

/* Maximum number of bytes the queue can hold. */
size_t bq_capacity  (const st_byte_queue *q);

/* Convenience predicates. */
int    bq_is_empty  (const st_byte_queue *q);
int    bq_is_full   (const st_byte_queue *q);

/* Push one byte. Returns 1 on success, 0 if the queue is full. */
int    bq_put       (st_byte_queue *q, uint8_t v);

/* Pop one byte into *out. Returns 1 on success, 0 if empty. */
int    bq_get       (st_byte_queue *q, uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif /* BYTE_QUEUE_H */
