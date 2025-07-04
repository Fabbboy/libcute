#include "memory/allocator.h"
#include "macro.h"
#include "slice.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static inline void cu_Allocator_CFree(void *_, cu_Slice mem) { free(mem.ptr); }

static inline Slice_Optional cu_Allocator_CAlloc(
    void *_, size_t size, size_t alignment) {
  if (size == 0) {
    return Slice_none();
  }

  size_t alignedSize = CU_ALIGN_UP(size, alignment);
  void *ptr = malloc(alignedSize);

  CU_IF_NULL(ptr) { return Slice_none(); }

  cu_Slice slice = cu_Slice_create(ptr, size);
  return Slice_some(slice);
}

static inline Slice_Optional cu_Allocator_CResize(
    void *_, cu_Slice mem, size_t size, size_t alignment) {

  if (size == 0) {
    cu_Allocator_CFree(NULL, mem);
    return Slice_none();
  }

  if (mem.length == size) {
    return Slice_some(mem);
  }

  Slice_Optional newSlice = cu_Allocator_CAlloc(NULL, size, alignment);
  if (Slice_is_none(&newSlice)) {
    return Slice_none();
  }

  void *newPtr = newSlice.value.ptr;

  size_t copySize = mem.length < size ? mem.length : size;
  memmove(newPtr, mem.ptr, copySize);
  cu_Allocator_CFree(NULL, mem);

  return newSlice;
}

cu_Allocator cu_Allocator_CAllocator(void) {
  cu_Allocator allocator;
  allocator.self = NULL;
  allocator.allocFn = cu_Allocator_CAlloc;
  allocator.resizeFn = cu_Allocator_CResize;
  allocator.freeFn = cu_Allocator_CFree;
  return allocator;
}