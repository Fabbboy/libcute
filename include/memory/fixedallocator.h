#pragma once

/** @file fixedallocator.h Fixed-size buffer allocator. */

#include "memory/allocator.h"
#include "nostd.h"

struct cu_FixedAllocator_Header {
  size_t prev_offset;
};

/** Runtime state for the fixed allocator. */
typedef struct {
  cu_Slice buffer; /**< memory backing */
  size_t used;     /**< consumed bytes */
} cu_FixedAllocator;

/** Initialize a fixed allocator using the given buffer. */
cu_Allocator cu_Allocator_FixedAllocator(
    cu_FixedAllocator *alloc, cu_Slice buffer);

/** Reset the allocator to reuse the buffer. */
void cu_FixedAllocator_reset(cu_FixedAllocator *alloc);
