#pragma once

/** @file allocator.h Generic allocator interface. */

#include <stddef.h>

#include "io/error.h"
#include "nostd.h"
#include "utility.h"

/** Allocation function signature. */
typedef cu_Slice_Result (*cu_Allocator_AllocFunc)(void *self, cu_Layout layout);
/** Resize function signature. */
typedef cu_Slice_Result (*cu_Allocator_ResizeFunc)(
    void *self, cu_Slice mem, cu_Layout layout);
/** Free function signature. */
typedef void (*cu_Allocator_FreeFunc)(void *self, cu_Slice mem);

/** Generic allocator with user-provided callbacks. */
typedef struct {
  void *self;                       /**< implementation specific data */
  cu_Allocator_AllocFunc allocFn;   /**< allocate memory */
  cu_Allocator_ResizeFunc resizeFn; /**< resize previously allocated memory */
  cu_Allocator_FreeFunc freeFn;     /**< free memory */
} cu_Allocator;
CU_OPTIONAL_DECL(cu_Allocator, cu_Allocator)

/** Allocate memory using the allocator. */
static inline cu_Slice_Result cu_Allocator_Alloc(
    cu_Allocator allocator, cu_Layout layout) {
  return allocator.allocFn(allocator.self, layout);
}

/** Resize a previously allocated block. */
static inline cu_Slice_Result cu_Allocator_Resize(
    cu_Allocator allocator, cu_Slice mem, cu_Layout layout) {
  return allocator.resizeFn(allocator.self, mem, layout);
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
