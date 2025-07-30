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

static cu_IoSlice_Result cu_CAllocator_Alloc(void *self, cu_Layout layout) {
  CU_UNUSED(self);
  if (layout.elem_size == 0) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }

  size_t size = layout.elem_size;
  size_t alignment = layout.alignment;
  if (alignment < sizeof(void *))
    alignment = sizeof(void *);

  size_t total = size + alignment - 1 + sizeof(void *);
  void *raw = malloc(total);
  if (raw == NULL) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }

  uintptr_t aligned_addr = ((uintptr_t)raw + sizeof(void *) + alignment - 1) &
                           ~(uintptr_t)(alignment - 1);
  void **store = (void **)aligned_addr - 1;
  *store = raw;

  return cu_IoSlice_Result_ok(cu_Slice_create((void *)aligned_addr, size));
}

static cu_IoSlice_Result cu_CAllocator_Grow(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  CU_UNUSED(self);
  if (old_mem.ptr == NULL) {
    return cu_CAllocator_Alloc(self, new_layout);
  }
  size_t alignment = new_layout.alignment;
  if (alignment < sizeof(void *)) {
    alignment = sizeof(void *);
  }
  void *raw = *((void **)old_mem.ptr - 1);
  size_t total = new_layout.elem_size + alignment - 1 + sizeof(void *);
  void *new_raw = realloc(raw, total);
  if (!new_raw && total != 0) {
    cu_IoSlice_Result alloc_res = cu_CAllocator_Alloc(self, new_layout);
    if (!cu_IoSlice_Result_is_ok(&alloc_res)) {
      return alloc_res;
    }
    cu_Memory_smemcpy(alloc_res.value, old_mem);
    cu_CAllocator_Free(NULL, old_mem);
    return alloc_res;
  }

  uintptr_t aligned_addr =
      ((uintptr_t)new_raw + sizeof(void *) + alignment - 1) &
      ~(uintptr_t)(alignment - 1);
  void **store = (void **)aligned_addr - 1;
  *store = new_raw;
  if ((void *)aligned_addr != old_mem.ptr) {
    size_t copy = CU_MIN(old_mem.length, new_layout.elem_size);
    cu_Memory_smemmove(cu_Slice_create((void *)aligned_addr, copy),
        cu_Slice_create(old_mem.ptr, copy));
  }
  return cu_IoSlice_Result_ok(
      cu_Slice_create((void *)aligned_addr, new_layout.elem_size));
}

static cu_IoSlice_Result cu_CAllocator_Shrink(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  CU_UNUSED(self);
  if (old_mem.ptr == NULL) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }
  size_t alignment = new_layout.alignment;
  if (alignment < sizeof(void *)) {
    alignment = sizeof(void *);
  }
  void *raw = *((void **)old_mem.ptr - 1);
  size_t total = new_layout.elem_size + alignment - 1 + sizeof(void *);
  void *new_raw = realloc(raw, total);
  if (!new_raw && total != 0) {
    cu_IoSlice_Result alloc_res = cu_CAllocator_Alloc(self, new_layout);
    if (!cu_IoSlice_Result_is_ok(&alloc_res)) {
      return alloc_res;
    }
    if (new_layout.elem_size > 0 && old_mem.length > 0) {
      cu_Memory_smemcpy(alloc_res.value,
          cu_Slice_create(old_mem.ptr, old_mem.length));
    }
    cu_CAllocator_Free(NULL, old_mem);
    return alloc_res;
  }

  uintptr_t aligned_addr =
      ((uintptr_t)new_raw + sizeof(void *) + alignment - 1) &
      ~(uintptr_t)(alignment - 1);
  void **store = (void **)aligned_addr - 1;
  *store = new_raw;
  if ((void *)aligned_addr != old_mem.ptr) {
    size_t copy = CU_MIN(new_layout.elem_size, old_mem.length);
    cu_Memory_smemcpy(cu_Slice_create((void *)aligned_addr, copy),
        cu_Slice_create(old_mem.ptr, copy));
  }
  return cu_IoSlice_Result_ok(
      cu_Slice_create((void *)aligned_addr, new_layout.elem_size));
}

cu_Allocator cu_Allocator_CAllocator(void) {
  cu_CAllocator_init();
  cu_Allocator allocator = {0};
  allocator.self = NULL;
  allocator.allocFn = cu_CAllocator_Alloc;
  allocator.growFn = cu_CAllocator_Grow;
  allocator.shrinkFn = cu_CAllocator_Shrink;
  allocator.freeFn = cu_CAllocator_Free;
  return allocator;
}

#endif // !CU_FREESTANDING

CU_OPTIONAL_IMPL(cu_Allocator, cu_Allocator)

static cu_IoSlice_Result cu_null_alloc(void *self, cu_Layout layout) {
  CU_UNUSED(self);
  CU_UNUSED(layout);
  cu_Io_Error err = {
      .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
  return cu_IoSlice_Result_error(err);
}

static cu_IoSlice_Result cu_null_grow(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  CU_UNUSED(self);
  CU_UNUSED(old_mem);
  CU_UNUSED(new_layout);
  cu_Io_Error err = {
      .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
  return cu_IoSlice_Result_error(err);
}

static cu_IoSlice_Result cu_null_shrink(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  CU_UNUSED(self);
  CU_UNUSED(old_mem);
  CU_UNUSED(new_layout);
  cu_Io_Error err = {
      .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
  return cu_IoSlice_Result_error(err);
}

static void cu_null_free(void *self, cu_Slice mem) {
  CU_UNUSED(self);
  CU_UNUSED(mem);
}

cu_Allocator cu_Allocator_NullAllocator(void) {
  cu_Allocator allocator = {0};
  allocator.self = NULL;
  allocator.allocFn = cu_null_alloc;
  allocator.growFn = cu_null_grow;
  allocator.shrinkFn = cu_null_shrink;
  allocator.freeFn = cu_null_free;
  return allocator;
}
