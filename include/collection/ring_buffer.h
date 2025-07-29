#pragma once

/** @file ring_buffer.h Circular buffer container. */

#include "macro.h"
#include "memory/allocator.h"
#include "nostd.h"
#include "object/optional.h"
#include "object/result.h"
#include "object/destructor.h"
#include "utility.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Ring buffer storing elements in FIFO order.
 */
typedef struct {
  cu_Slice_Optional data; /**< element storage */
  size_t capacity;        /**< maximum elements */
  size_t head;            /**< index of first element */
  size_t length;          /**< number of stored elements */
  cu_Layout layout;       /**< element layout */
  cu_Allocator allocator; /**< backing allocator */
  cu_Destructor_Optional destructor; /**< optional element destructor */
} cu_RingBuffer;

/** Error codes returned by ring buffer operations. */
typedef enum {
  CU_RINGBUFFER_ERROR_NONE = 0,
  CU_RINGBUFFER_ERROR_OOM,
  CU_RINGBUFFER_ERROR_INVALID_LAYOUT,
  CU_RINGBUFFER_ERROR_INVALID,
  CU_RINGBUFFER_ERROR_FULL,
  CU_RINGBUFFER_ERROR_EMPTY,
} cu_RingBuffer_Error;

CU_RESULT_DECL(cu_RingBuffer, cu_RingBuffer, cu_RingBuffer_Error)
CU_OPTIONAL_DECL(cu_RingBuffer_Error, cu_RingBuffer_Error)

cu_RingBuffer_Result cu_RingBuffer_create(
    cu_Allocator allocator, cu_Layout layout, size_t capacity,
    cu_Destructor_Optional destructor);

void cu_RingBuffer_destroy(cu_RingBuffer *rb);

static inline size_t cu_RingBuffer_size(const cu_RingBuffer *rb) {
  CU_IF_NULL(rb) { return 0; }
  return rb->length;
}

static inline size_t cu_RingBuffer_capacity(const cu_RingBuffer *rb) {
  CU_IF_NULL(rb) { return 0; }
  return rb->capacity;
}

static inline bool cu_RingBuffer_is_empty(const cu_RingBuffer *rb) {
  CU_IF_NULL(rb) { return true; }
  return rb->length == 0;
}

static inline bool cu_RingBuffer_is_full(const cu_RingBuffer *rb) {
  CU_IF_NULL(rb) { return false; }
  return rb->length == rb->capacity && rb->capacity > 0;
}

cu_RingBuffer_Error_Optional cu_RingBuffer_push(cu_RingBuffer *rb, void *elem);

cu_RingBuffer_Error_Optional cu_RingBuffer_pop(
    cu_RingBuffer *rb, void *out_elem);
