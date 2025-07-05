#pragma once

#include "object/optional.h"
#include <stddef.h>

/** Non-owning memory view */
typedef struct {
  void *ptr;
  size_t length;
} cu_Slice;

/** Create a slice from a pointer and length without allocating. */
cu_Slice cu_Slice_create(void *ptr, size_t length);

CU_OPTIONAL_DECL(cu_Slice, cu_Slice)
