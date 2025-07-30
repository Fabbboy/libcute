#pragma once

/** @file allocator.h Generic allocator interface. */

#include <stddef.h>

#include "io/error.h"
#include "nostd.h"
#include "utility.h"

/** Allocation function signature. */
typedef cu_IoSlice_Result (*cu_Allocator_AllocFunc)(
    void *self, cu_Layout layout);
/** Grow function signature. */
typedef cu_IoSlice_Result (*cu_Allocator_GrowFunc)(
    void *self, cu_Slice old_mem, cu_Layout new_layout);
/** Shrink function signature. */
typedef cu_IoSlice_Result (*cu_Allocator_ShrinkFunc)(
    void *self, cu_Slice old_mem, cu_Layout new_layout);
/** Free function signature. */
typedef void (*cu_Allocator_FreeFunc)(void *self, cu_Slice mem);

/** Generic allocator with user-provided callbacks. */
typedef struct {
  void *self;                       /**< implementation specific data */
  cu_Allocator_AllocFunc allocFn;    /**< allocate memory */
  cu_Allocator_GrowFunc growFn;      /**< grow previously allocated memory */
  cu_Allocator_ShrinkFunc shrinkFn;  /**< shrink previously allocated memory */
  cu_Allocator_FreeFunc freeFn;     /**< free memory */
} cu_Allocator;
CU_OPTIONAL_DECL(cu_Allocator, cu_Allocator)

/** Allocate memory using the allocator. */
static inline cu_IoSlice_Result cu_Allocator_Alloc(
    cu_Allocator allocator, cu_Layout layout) {
  return allocator.allocFn(allocator.self, layout);
}

/** Grow a previously allocated block. */
static inline cu_IoSlice_Result cu_Allocator_Grow(
    cu_Allocator allocator, cu_Slice old_mem, cu_Layout new_layout) {
  return allocator.growFn(allocator.self, old_mem, new_layout);
}

/** Shrink a previously allocated block. */
static inline cu_IoSlice_Result cu_Allocator_Shrink(
    cu_Allocator allocator, cu_Slice old_mem, cu_Layout new_layout) {
  return allocator.shrinkFn(allocator.self, old_mem, new_layout);
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
