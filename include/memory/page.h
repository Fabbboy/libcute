#pragma once

#include "memory/allocator.h"
#include <stddef.h>

typedef struct {
  size_t pageSize;
} cu_PageAllocator;

cu_Allocator cu_Allocator_PageAllocator(cu_PageAllocator *allocator);
