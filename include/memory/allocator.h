#pragma once

/** @file allocator.h Generic allocator interface. */

#include <stddef.h>

#include "nostd.h"

/** Allocation function signature. */
typedef cu_Slice_Result (*cu_Allocator_AllocFunc)(
    void *self, size_t size, size_t alignment);
/** Resize function signature. */
typedef cu_Slice_Result (*cu_Allocator_ReszizeFunc)(
    void *self, cu_Slice mem, size_t size, size_t alignment);
/** Free function signature. */
typedef void (*cu_Allocator_FreeFunc)(void *self, cu_Slice mem);

/** Generic allocator with user-provided callbacks. */
typedef struct {
  void *self;                        /**< implementation specific data */
  cu_Allocator_AllocFunc allocFn;    /**< allocate memory */
  cu_Allocator_ReszizeFunc resizeFn; /**< resize previously allocated memory */
  cu_Allocator_FreeFunc freeFn;      /**< free memory */
} cu_Allocator;

/** Allocate memory using the allocator. */
static inline cu_Slice_Result cu_Allocator_Alloc(
    cu_Allocator allocator, size_t size, size_t alignment) {
  return allocator.allocFn(allocator.self, size, alignment);
}

/** Resize a previously allocated block. */
static inline cu_Slice_Result cu_Allocator_Resize(
    cu_Allocator allocator, cu_Slice mem, size_t size, size_t alignment) {
  return allocator.resizeFn(allocator.self, mem, size, alignment);
}

/** Free memory obtained from this allocator. */
static inline void cu_Allocator_Free(cu_Allocator allocator, cu_Slice mem) {
  allocator.freeFn(allocator.self, mem);
}

/** Standard allocator backed by malloc and free. */
cu_Allocator cu_Allocator_CAllocator(void);

CU_OPTIONAL_DECL(cu_Allocator, cu_Allocator)
