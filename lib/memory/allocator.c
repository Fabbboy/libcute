#include "memory/allocator.h"
#include "macro.h"
#include <stddef.h>
#include <stdlib.h>

static inline Ptr_Optional cu_Allocator_CAlloc(
    void *_, size_t size, size_t alignment) {
  void *ptr = aligned_alloc(alignment, size);
  CU_IF_NULL(ptr) { return Ptr_none(); }
  return Ptr_some(ptr);
}

static inline Ptr_Optional cu_Allocator_CResize(
    void *_, void *ptr, size_t size, size_t alignment) {
  size_t aligned_size = size + (alignment - 1);
  void *new_ptr = realloc(ptr, aligned_size);
  CU_IF_NULL(new_ptr) { return Ptr_none(); }
  return Ptr_some(new_ptr);
}

static inline void cu_Allocator_CFree(void *_, void *ptr) { free(ptr); }

cu_Allocator cu_Allocator_Default(void) {
  cu_Allocator allocator;
  allocator.self = NULL;
  allocator.allocFn = cu_Allocator_CAlloc;
  allocator.resizeFn = cu_Allocator_CResize;
  allocator.freeFn = cu_Allocator_CFree;
  return allocator;
}