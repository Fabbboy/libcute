#pragma once

/** Round up to the next power of two. */
#include <stddef.h>
#include "macro.h"
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
  cu_Layout layout = {0, 0};
  if (elem_size == 0 || alignment == 0) {
    return layout;
  }

  layout.elem_size = elem_size;
  layout.alignment = alignment;
  return layout;
}

#define CU_LAYOUT(T) cu_Layout_create(sizeof(T), _Alignof(T))
#define CU_LAYOUT_CHECK(layout)                                                \
  if ((layout).elem_size == 0 || (layout).alignment == 0)

#define CU_TIME_NS_PER_SEC 1000000000ULL
#if CU_PLAT_WINDOWS
#define CU_TIME_WINDOWS_TICKS_PER_SEC 10000000ULL
#define CU_TIME_WINDOWS_EPOCH_DIFF 116444736000000000ULL
#define CU_TIME_WINDOWS_TICK_NS 100ULL
#include <windows.h>
long long cu_Time_filetime_to_unix(FILETIME ft);
FILETIME cu_Time_unix_to_filetime(long long ns);
#endif

