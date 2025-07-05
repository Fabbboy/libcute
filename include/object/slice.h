#pragma once

#include "object/optional.h"
#include <stddef.h>

typedef struct {
  void *ptr;
  size_t length;
} cu_Slice;

cu_Slice cu_Slice_create(void *ptr, size_t length);

CU_OPTIONAL_DECL(Slice, cu_Slice)
