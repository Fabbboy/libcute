#pragma once

#include <stddef.h>

#include "slice.h"

typedef Slice_Optional (*cu_Allocator_AllocFunc)(
    void *self, size_t size, size_t alignment);
typedef Slice_Optional (*cu_Allocator_ReszizeFunc)(
    void *self, cu_Slice mem, size_t size, size_t alignment);
typedef void (*cu_Allocator_FreeFunc)(void *self, cu_Slice mem);

typedef struct {
  void *self;
  cu_Allocator_AllocFunc allocFn;
  cu_Allocator_ReszizeFunc resizeFn;
  cu_Allocator_FreeFunc freeFn;
} cu_Allocator;

static inline Slice_Optional cu_Allocator_Alloc(
    cu_Allocator allocator, size_t size, size_t alignment) {
  return allocator.allocFn(allocator.self, size, alignment);
}

static inline Slice_Optional cu_Allocator_Resize(
    cu_Allocator allocator, cu_Slice mem, size_t size, size_t alignment) {
  return allocator.resizeFn(allocator.self, mem, size, alignment);
}

static inline void cu_Allocator_Free(cu_Allocator allocator, cu_Slice mem) {
  allocator.freeFn(allocator.self, mem);
}

cu_Allocator cu_Allocator_CAllocator(void);

