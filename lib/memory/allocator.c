#include "memory/allocator.h"
#include "io/error.h"
#include "macro.h"
#include <nostd.h>
#include <stddef.h>
#include <stdint.h>
#if !CU_FREESTANDING
#include <stdlib.h>
#endif

#if !CU_FREESTANDING

static void cu_CAllocator_init(void) {}

static inline void cu_CAllocator_Free(void *self, cu_Slice mem) {
  CU_UNUSED(self);
  CU_IF_NULL(mem.ptr) { return; }
  void *raw = *((void **)mem.ptr - 1);
  free(raw);
}

static cu_Slice_Result cu_CAllocator_Alloc(
    void *self, size_t size, size_t alignment) {
  CU_UNUSED(self);
  if (size == 0) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  if (alignment < sizeof(void *))
    alignment = sizeof(void *);

  size_t total = size + alignment - 1 + sizeof(void *);
  void *raw = malloc(total);
  if (raw == NULL) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  uintptr_t aligned_addr = ((uintptr_t)raw + sizeof(void *) + alignment - 1) &
                           ~(uintptr_t)(alignment - 1);
  void **store = (void **)aligned_addr - 1;
  *store = raw;

  return cu_Slice_result_ok(cu_Slice_create((void *)aligned_addr, size));
}

static cu_Slice_Result cu_CAllocator_Resize(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  CU_UNUSED(self);
  CU_IF_NULL(mem.ptr) { return cu_CAllocator_Alloc(self, size, alignment); }
  if (size == 0) {
    cu_CAllocator_Free(NULL, mem);
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  cu_Slice_Result new_mem = cu_CAllocator_Alloc(self, size, alignment);
  if (!cu_Slice_result_is_ok(&new_mem)) {
    return new_mem;
  }
  size_t copy = mem.length < size ? mem.length : size;
  cu_Memory_memmove(new_mem.value.ptr, cu_Slice_create(mem.ptr, copy));
  cu_CAllocator_Free(NULL, mem);
  return new_mem;
}

cu_Allocator cu_Allocator_CAllocator(void) {
  cu_CAllocator_init();
  cu_Allocator allocator = {0};
  allocator.self = NULL;
  allocator.allocFn = cu_CAllocator_Alloc;
  allocator.resizeFn = cu_CAllocator_Resize;
  allocator.freeFn = cu_CAllocator_Free;
  return allocator;
}

#endif // !CU_FREESTANDING

CU_OPTIONAL_IMPL(cu_Allocator, cu_Allocator)

static cu_Slice_Result cu_null_alloc(
    void *self, size_t size, size_t alignment) {
  CU_UNUSED(self);
  CU_UNUSED(size);
  CU_UNUSED(alignment);
  cu_Io_Error err = {
      .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
  return cu_Slice_result_error(err);
}

static cu_Slice_Result cu_null_resize(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  CU_UNUSED(self);
  CU_UNUSED(mem);
  CU_UNUSED(size);
  CU_UNUSED(alignment);
  cu_Io_Error err = {
      .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
  return cu_Slice_result_error(err);
}

static void cu_null_free(void *self, cu_Slice mem) {
  CU_UNUSED(self);
  CU_UNUSED(mem);
}

cu_Allocator cu_Allocator_NullAllocator(void) {
  cu_Allocator allocator = {0};
  allocator.self = NULL;
  allocator.allocFn = cu_null_alloc;
  allocator.resizeFn = cu_null_resize;
  allocator.freeFn = cu_null_free;
  return allocator;
}
