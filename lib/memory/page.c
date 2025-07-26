#include "memory/page.h"
#include "io/error.h"
#include "macro.h"
#include <nostd.h>
#include <stddef.h>
#if !CU_FREESTANDING && !CU_PLAT_WASM
#include <stdlib.h>
#endif

#if !CU_FREESTANDING && !CU_PLAT_WASM

static void cu_PageAllocator_Free(void *self, cu_Slice mem) {
  CU_UNUSED(self);
  CU_IF_NOT_NULL(mem.ptr) { free(mem.ptr); }
}

static cu_Slice_Result cu_PageAllocator_Alloc(
    void *self, size_t size, size_t alignment) {
  CU_UNUSED(alignment);
  cu_PageAllocator *allocator = (cu_PageAllocator *)self;
  if (size == 0) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  size_t aligned_size = CU_ALIGN_UP(size, allocator->pageSize);
  void *ptr = malloc(aligned_size);
  if (ptr == NULL) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }
  return cu_Slice_result_ok(cu_Slice_create(ptr, aligned_size));
}

static cu_Slice_Result cu_PageAllocator_Resize(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  CU_UNUSED(alignment);
  if (size == 0) {
    cu_PageAllocator_Free(self, mem);
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  cu_PageAllocator *allocator = (cu_PageAllocator *)self;
  size_t aligned_size = CU_ALIGN_UP(size, allocator->pageSize);
  void *new_ptr = realloc(mem.ptr, aligned_size);
  if (new_ptr == NULL && aligned_size != 0) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  return cu_Slice_result_ok(cu_Slice_create(new_ptr, aligned_size));
}

cu_Allocator cu_Allocator_PageAllocator(cu_PageAllocator *allocator) {
  allocator->pageSize = 4096;
  cu_Allocator alloc;
  alloc.self = allocator;
  alloc.allocFn = cu_PageAllocator_Alloc;
  alloc.resizeFn = cu_PageAllocator_Resize;
  alloc.freeFn = cu_PageAllocator_Free;
  return alloc;
}

#endif
