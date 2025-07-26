#include "collection/ring_buffer.h"
#include "utility.h"
#include <nostd.h>

CU_RESULT_IMPL(cu_RingBuffer, cu_RingBuffer, cu_RingBuffer_Error)
CU_OPTIONAL_IMPL(cu_RingBuffer_Error, cu_RingBuffer_Error)

cu_RingBuffer_Result cu_RingBuffer_create(
    cu_Allocator allocator, cu_Layout layout, size_t capacity) {
  CU_LAYOUT_CHECK(layout) {
    return cu_RingBuffer_result_error(CU_RINGBUFFER_ERROR_INVALID_LAYOUT);
  }

  cu_Slice_Optional data = cu_Slice_Optional_none();
  if (capacity > 0) {
    cu_Slice_Result r = cu_Allocator_Alloc(
        allocator, capacity * layout.elem_size, layout.alignment);
    if (!cu_Slice_result_is_ok(&r)) {
      return cu_RingBuffer_result_error(CU_RINGBUFFER_ERROR_OOM);
    }
    data = cu_Slice_Optional_some(r.value);
  }

  cu_RingBuffer rb;
  rb.data = data;
  rb.capacity = capacity;
  rb.head = 0;
  rb.length = 0;
  rb.layout = layout;
  rb.allocator = allocator;

  return cu_RingBuffer_result_ok(rb);
}

void cu_RingBuffer_destroy(cu_RingBuffer *rb) {
  if (!rb) {
    return;
  }
  if (cu_Slice_Optional_is_some(&rb->data)) {
    cu_Allocator_Free(rb->allocator, cu_Slice_create(rb->data.value.ptr,
                                         rb->capacity * rb->layout.elem_size));
    rb->data = cu_Slice_Optional_none();
  }
  rb->capacity = 0;
  rb->head = 0;
  rb->length = 0;
}

cu_RingBuffer_Error_Optional cu_RingBuffer_push(cu_RingBuffer *rb, void *elem) {
  CU_IF_NULL(rb) {
    return cu_RingBuffer_Error_Optional_some(CU_RINGBUFFER_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(rb->layout) {
    return cu_RingBuffer_Error_Optional_some(
        CU_RINGBUFFER_ERROR_INVALID_LAYOUT);
  }
  if (cu_RingBuffer_is_full(rb)) {
    return cu_RingBuffer_Error_Optional_some(CU_RINGBUFFER_ERROR_FULL);
  }
  size_t tail = (rb->head + rb->length) % rb->capacity;
  void *dest =
      (unsigned char *)rb->data.value.ptr + tail * rb->layout.elem_size;
  memcpy(dest, elem, rb->layout.elem_size);
  rb->length++;
  return cu_RingBuffer_Error_Optional_none();
}

cu_RingBuffer_Error_Optional cu_RingBuffer_pop(
    cu_RingBuffer *rb, void *out_elem) {
  CU_IF_NULL(rb) {
    return cu_RingBuffer_Error_Optional_some(CU_RINGBUFFER_ERROR_INVALID);
  }
  CU_LAYOUT_CHECK(rb->layout) {
    return cu_RingBuffer_Error_Optional_some(
        CU_RINGBUFFER_ERROR_INVALID_LAYOUT);
  }
  if (cu_RingBuffer_is_empty(rb)) {
    return cu_RingBuffer_Error_Optional_some(CU_RINGBUFFER_ERROR_EMPTY);
  }
  void *src =
      (unsigned char *)rb->data.value.ptr + rb->head * rb->layout.elem_size;
  memcpy(out_elem, src, rb->layout.elem_size);
  rb->head = (rb->head + 1) % rb->capacity;
  rb->length--;
  return cu_RingBuffer_Error_Optional_none();
}
