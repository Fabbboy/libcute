#pragma once

/** @file slab.h Simple slab allocator. */

#include "memory/allocator.h"
#include <stddef.h>

#define CU_SLAB_DEFAULT_SIZE 4096 /**< default slab size */

struct cu_SlabAllocator_Slab;

/** Runtime state for the slab allocator. */
typedef struct {
  cu_Allocator backingAllocator;         /**< allocator used for slabs */
  struct cu_SlabAllocator_Slab *slabs;   /**< list of allocated slabs */
  struct cu_SlabAllocator_Slab *current; /**< slab used for new allocations */
  size_t slabSize;                       /**< bytes per slab */
} cu_SlabAllocator;

/** Configuration for creating a slab allocator. */
typedef struct {
  size_t slabSize;                        /**< desired slab size */
  cu_Allocator_Optional backingAllocator; /**< custom backing allocator */
} cu_SlabAllocator_Config;

/** Create a slab allocator using the given configuration. */
cu_Allocator cu_Allocator_SlabAllocator(
    cu_SlabAllocator *alloc, cu_SlabAllocator_Config cfg);

/** Release all memory owned by the allocator. */
void cu_SlabAllocator_destroy(cu_SlabAllocator *alloc);
