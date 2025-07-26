#pragma once

/** @file page.h OS backed page allocator. */
#include "macro.h"
#include "memory/allocator.h"
#include <stddef.h>

#if !CU_PLAT_WASM && !CU_FREESTANDING

/** Storage for a page allocator instance. */
typedef struct {
  size_t pageSize; /**< system page size in bytes */
} cu_PageAllocator;

/** Create an allocator that allocates memory using OS pages. */
cu_Allocator cu_Allocator_PageAllocator(cu_PageAllocator *allocator);

#endif
