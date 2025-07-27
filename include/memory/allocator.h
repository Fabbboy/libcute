#pragma once

/** @file allocator.h Generic allocator interface. */

#include <stddef.h>

#include "io/error.h"
#include "nostd.h"

/** Allocation function signature. */
typedef cu_Slice_Result (*cu_Allocator_AllocFunc)(
    void *self, size_t size, size_t alignment);
/** Grow function signature. */
typedef cu_Slice_Result (*cu_Allocator_GrowFunc)(
    void *self, cu_Slice mem, size_t size, size_t alignment);
/** Shrink function signature. */
typedef cu_Slice_Result (*cu_Allocator_ShrinkFunc)(
    void *self, cu_Slice mem, size_t size, size_t alignment);
/** Free function signature. */
typedef void (*cu_Allocator_FreeFunc)(void *self, cu_Slice mem);

/** Generic allocator with user-provided callbacks. */
typedef struct {
  void *self;                       /**< implementation specific data */
  cu_Allocator_AllocFunc allocFn;   /**< allocate memory */
  cu_Allocator_GrowFunc growFn;     /**< grow previously allocated memory */
  cu_Allocator_ShrinkFunc shrinkFn; /**< shrink previously allocated memory */
  cu_Allocator_FreeFunc freeFn;     /**< free memory */
} cu_Allocator;
CU_OPTIONAL_DECL(cu_Allocator, cu_Allocator)

/** Allocate memory using the allocator. */
static inline cu_Slice_Result cu_Allocator_Alloc(
    cu_Allocator allocator, size_t size, size_t alignment) {
  return allocator.allocFn(allocator.self, size, alignment);
}

/** Grow a previously allocated block. */
static inline cu_Slice_Result cu_Allocator_Grow(
    cu_Allocator allocator, cu_Slice mem, size_t size, size_t alignment) {
  return allocator.growFn(allocator.self, mem, size, alignment);
}

/** Shrink a previously allocated block. */
static inline cu_Slice_Result cu_Allocator_Shrink(
    cu_Allocator allocator, cu_Slice mem, size_t size, size_t alignment) {
  return allocator.shrinkFn(allocator.self, mem, size, alignment);
}

/** Free memory obtained from this allocator. */
static inline void cu_Allocator_Free(cu_Allocator allocator, cu_Slice mem) {
  allocator.freeFn(allocator.self, mem);
}

/** System allocator backed by libc malloc/free. */
#if !CU_FREESTANDING
cu_Allocator cu_Allocator_CAllocator(void);
#endif

/** Allocator that always fails with out-of-memory errors. */
cu_Allocator cu_Allocator_NullAllocator(void);
