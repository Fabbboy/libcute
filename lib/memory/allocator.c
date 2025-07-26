#include "memory/allocator.h"
#include "macro.h"
#include "nostd.h"
#include <nostd.h>
#include <stddef.h>
#include <string.h>

static inline void cu_CAllocator_Free(void *self, cu_Slice mem) {
  CU_UNUSED(self);
  free(mem.ptr);
}

static cu_Slice_Result cu_CAllocator_Alloc(
    void *self, size_t size, size_t alignment) {
  CU_UNUSED(self);

  if (size == 0) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  size_t alignedSize = CU_ALIGN_UP(size, alignment);
  void *ptr = malloc(alignedSize);

  CU_IF_NULL(ptr) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  cu_Slice slice = cu_Slice_create(ptr, size);
  return cu_Slice_result_ok(slice);
}

static cu_Slice_Result cu_CAllocator_Resize(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  CU_UNUSED(self);

  if (size == 0) {
    cu_CAllocator_Free(NULL, mem);
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  if (mem.length == size) {
    return cu_Slice_result_ok(mem);
  }

  cu_Slice_Result newSlice = cu_CAllocator_Alloc(NULL, size, alignment);
  if (!cu_Slice_result_is_ok(&newSlice)) {
    return newSlice;
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

CU_OPTIONAL_IMPL(cu_Allocator, cu_Allocator)
