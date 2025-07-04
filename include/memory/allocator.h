#pragma once

#include <stddef.h>

#include "optional.h"

typedef Ptr_Optional (*cu_Allocator_AllocFunc)(
    void *self, size_t size, size_t alignment);
typedef Ptr_Optional (*cu_Allocator_ReszizeFunc)(
    void *self, void *ptr, size_t size, size_t alignment);
typedef void (*cu_Allocator_FreeFunc)(void *self, void *ptr);

typedef struct {
  void *self;
  cu_Allocator_AllocFunc allocFn;
  cu_Allocator_ReszizeFunc resizeFn;
  cu_Allocator_FreeFunc freeFn;
} cu_Allocator;

static inline Ptr_Optional cu_Allocator_Alloc(
    cu_Allocator *allocator, size_t size, size_t alignment) {
  return allocator->allocFn(allocator->self, size, alignment);
}

static inline Ptr_Optional cu_Allocator_Resize(
    cu_Allocator *allocator, void *ptr, size_t size, size_t alignment) {
  return allocator->resizeFn(allocator->self, ptr, size, alignment);
}

static inline void cu_Allocator_Free(cu_Allocator *allocator, void *ptr) {
  allocator->freeFn(allocator->self, ptr);
}

cu_Allocator cu_Allocator_Default(void);