#include "memory/fixedallocator.h"
#include "io/error.h"
#include "macro.h"
#include <nostd.h>

static void cu_fixed_free(void *self, cu_Slice mem);

static cu_IoSlice_Result cu_fixed_alloc(void *self, cu_Layout layout) {
  cu_FixedAllocator *alloc = (cu_FixedAllocator *)self;
  if (layout.elem_size == 0) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }
  size_t size = layout.elem_size;
  size_t alignment = layout.alignment;
  if (alignment == 0) {
    alignment = 1;
  }

  const size_t header_size = sizeof(struct cu_FixedAllocator_Header);
  size_t req_align = CU_MAX(alignment, _Alignof(struct cu_FixedAllocator_Header));
  size_t start = CU_ALIGN_UP(alloc->used + header_size, req_align);
  if (start + size > alloc->buffer.length) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }

  struct cu_FixedAllocator_Header *hdr =
      (struct cu_FixedAllocator_Header *)((unsigned char *)alloc->buffer.ptr +
                                          start - header_size);
  hdr->prev_offset = alloc->used;
  alloc->used = start + size;
  return cu_IoSlice_Result_ok(
      cu_Slice_create((unsigned char *)alloc->buffer.ptr + start, size));
}

static cu_IoSlice_Result cu_fixed_grow(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  cu_FixedAllocator *alloc = (cu_FixedAllocator *)self;
  CU_IF_NULL(old_mem.ptr) {
    return cu_fixed_alloc(self, new_layout);
  }

  const size_t header_size = sizeof(struct cu_FixedAllocator_Header);
  struct cu_FixedAllocator_Header *hdr =
      (struct cu_FixedAllocator_Header *)((unsigned char *)old_mem.ptr -
                                          header_size);
  if ((unsigned char *)old_mem.ptr + old_mem.length ==
      (unsigned char *)alloc->buffer.ptr + alloc->used) {
    size_t avail = alloc->buffer.length - alloc->used;
    if (new_layout.elem_size <= old_mem.length + avail) {
      alloc->used = (alloc->used - old_mem.length) + new_layout.elem_size;
      return cu_IoSlice_Result_ok(
          cu_Slice_create(old_mem.ptr, new_layout.elem_size));
    }
  }

  cu_IoSlice_Result new_mem = cu_fixed_alloc(self, new_layout);
  if (!cu_IoSlice_Result_is_ok(&new_mem)) {
    return new_mem;
  }
  if (old_mem.length > 0) {
    cu_Memory_smemcpy(new_mem.value, old_mem);
  }
  if ((unsigned char *)old_mem.ptr + old_mem.length ==
      (unsigned char *)alloc->buffer.ptr + alloc->used) {
    alloc->used = hdr->prev_offset;
  }
  return new_mem;
}

static cu_IoSlice_Result cu_fixed_shrink(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  cu_FixedAllocator *alloc = (cu_FixedAllocator *)self;
  CU_IF_NULL(old_mem.ptr) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }

  const size_t header_size = sizeof(struct cu_FixedAllocator_Header);
  struct cu_FixedAllocator_Header *hdr =
      (struct cu_FixedAllocator_Header *)((unsigned char *)old_mem.ptr -
                                          header_size);
  if ((unsigned char *)old_mem.ptr + old_mem.length ==
      (unsigned char *)alloc->buffer.ptr + alloc->used) {
    alloc->used = (alloc->used - old_mem.length) + new_layout.elem_size;
    return cu_IoSlice_Result_ok(
        cu_Slice_create(old_mem.ptr, new_layout.elem_size));
  }
  CU_UNUSED(hdr);

  cu_IoSlice_Result new_mem = cu_fixed_alloc(self, new_layout);
  if (!cu_IoSlice_Result_is_ok(&new_mem)) {
    return new_mem;
  }
  if (new_layout.elem_size > 0) {
    size_t copy = CU_MIN(new_layout.elem_size, old_mem.length);
    cu_Memory_smemcpy(cu_Slice_create(new_mem.value.ptr, copy),
        cu_Slice_create(old_mem.ptr, copy));
  }
  return new_mem;
}

static void cu_fixed_free(void *self, cu_Slice mem) {
  cu_FixedAllocator *alloc = (cu_FixedAllocator *)self;
  CU_IF_NULL(mem.ptr) { return; }
  const size_t header_size = sizeof(struct cu_FixedAllocator_Header);
  struct cu_FixedAllocator_Header *hdr =
      (struct cu_FixedAllocator_Header *)((unsigned char *)mem.ptr -
                                          header_size);
  if ((unsigned char *)mem.ptr + mem.length !=
      (unsigned char *)alloc->buffer.ptr + alloc->used) {
    return;
  }
  alloc->used = hdr->prev_offset;
}

cu_Allocator cu_Allocator_FixedAllocator(
    cu_FixedAllocator *alloc, cu_Slice buffer) {
  alloc->buffer = buffer;
  alloc->used = 0;

  cu_Allocator a = {0};
  a.self = alloc;
  a.allocFn = cu_fixed_alloc;
  a.growFn = cu_fixed_grow;
  a.shrinkFn = cu_fixed_shrink;
  a.freeFn = cu_fixed_free;
  return a;
}

void cu_FixedAllocator_reset(cu_FixedAllocator *alloc) { alloc->used = 0; }
