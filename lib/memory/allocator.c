#include "memory/allocator.h"
#include "macro.h"
#include "object/slice.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static inline void cu_CAllocator_Free(void *self, cu_Slice mem) {
  CU_UNUSED(self);
  free(mem.ptr);
}

static cu_Slice_Optional cu_CAllocator_Alloc(
    void *self, size_t size, size_t alignment) {
  CU_UNUSED(self);

  if (size == 0) {
    return cu_Slice_none();
  }

  size_t alignedSize = CU_ALIGN_UP(size, alignment);
  void *ptr = malloc(alignedSize);

  CU_IF_NULL(ptr) { return cu_Slice_none(); }

  cu_Slice slice = cu_Slice_create(ptr, size);
  return cu_Slice_some(slice);
}

static cu_Slice_Optional cu_CAllocator_Resize(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  CU_UNUSED(self);

  if (size == 0) {
    cu_CAllocator_Free(NULL, mem);
    return cu_Slice_none();
  }

  if (mem.length == size) {
    return cu_Slice_some(mem);
  }

  cu_Slice_Optional newSlice = cu_CAllocator_Alloc(NULL, size, alignment);
  if (cu_Slice_is_none(&newSlice)) {
    return cu_Slice_none();
  }

  void *newPtr = newSlice.value.ptr;

  size_t copySize = mem.length < size ? mem.length : size;
  memmove(newPtr, mem.ptr, copySize);
  cu_CAllocator_Free(NULL, mem);

  return newSlice;
}

cu_Allocator cu_Allocator_CAllocator(void) {
  cu_Allocator allocator;
  allocator.self = NULL;
  allocator.allocFn = cu_CAllocator_Alloc;
  allocator.resizeFn = cu_CAllocator_Resize;
  allocator.freeFn = cu_CAllocator_Free;
  return allocator;
}
