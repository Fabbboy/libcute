#pragma once

/** Round up to the next power of two. */
#include <stddef.h>
static inline size_t cu_next_pow2(size_t x) {
  if (x <= 1) {
    return 1;
  }
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
#if SIZE_MAX > UINT32_MAX
  x |= x >> 32;
#endif
  return x + 1;
}

typedef struct {
  size_t elem_size;
  size_t alignment;
} cu_Layout;

static inline cu_Layout cu_Layout_create(size_t elem_size, size_t alignment) {
  return (cu_Layout){elem_size, alignment};
}

#define CU_LAYOUT(T) cu_Layout_create(sizeof(T), alignof(T))
#define CU_LAYOUT_CHECK(layout)                                                \
  if ((layout).elem_size == 0 || (layout).alignment == 0)
