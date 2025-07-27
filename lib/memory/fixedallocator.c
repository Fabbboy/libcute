#include "memory/fixedallocator.h"
#include "io/error.h"
#include "macro.h"
#include <nostd.h>

static void cu_fixed_free(void *self, cu_Slice mem);

static cu_Slice_Result cu_fixed_alloc(
    void *self, size_t size, size_t alignment) {
  cu_FixedAllocator *alloc = (cu_FixedAllocator *)self;
  if (size == 0) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }
  if (alignment == 0) {
    alignment = 1;
  }

  const size_t header_size = sizeof(struct cu_FixedAllocator_Header);
  size_t req_align = alignment > _Alignof(struct cu_FixedAllocator_Header)
                          ? alignment
                          : _Alignof(struct cu_FixedAllocator_Header);
  size_t start = CU_ALIGN_UP(alloc->used + header_size, req_align);
  if (start + size > alloc->buffer.length) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  struct cu_FixedAllocator_Header *hdr =
      (struct cu_FixedAllocator_Header *)((unsigned char *)alloc->buffer.ptr +
                                          start - header_size);
  hdr->prev_offset = alloc->used;
  alloc->used = start + size;
  return cu_Slice_result_ok(
      cu_Slice_create((unsigned char *)alloc->buffer.ptr + start, size));
}

static cu_Slice_Result cu_fixed_resize_internal(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  cu_FixedAllocator *alloc = (cu_FixedAllocator *)self;
  CU_IF_NULL(mem.ptr) { return cu_fixed_alloc(self, size, alignment); }
  if (size == 0) {
    cu_fixed_free(self, mem);
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  const size_t header_size = sizeof(struct cu_FixedAllocator_Header);
  struct cu_FixedAllocator_Header *hdr =
      (struct cu_FixedAllocator_Header *)((unsigned char *)mem.ptr -
                                          header_size);
  if ((unsigned char *)mem.ptr + mem.length ==
      (unsigned char *)alloc->buffer.ptr + alloc->used) {
    size_t avail = alloc->buffer.length - (alloc->used - mem.length);
    if (size <= mem.length + avail) {
      alloc->used = (alloc->used - mem.length) + size;
      return cu_Slice_result_ok(cu_Slice_create(mem.ptr, size));
    }
  }

  cu_Slice_Result new_mem = cu_fixed_alloc(self, size, alignment);
  if (!cu_Slice_result_is_ok(&new_mem)) {
    return new_mem;
  }
  cu_Memory_memcpy(new_mem.value.ptr,
      cu_Slice_create(mem.ptr, mem.length < size ? mem.length : size));
  // adjust used to release old memory
  alloc->used = hdr->prev_offset;
  return new_mem;
}

static cu_Slice_Result cu_fixed_grow(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  return cu_fixed_resize_internal(self, mem, size, alignment);
}

static cu_Slice_Result cu_fixed_shrink(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  return cu_fixed_resize_internal(self, mem, size, alignment);
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

  cu_Allocator a;
  a.self = alloc;
  a.allocFn = cu_fixed_alloc;
  a.growFn = cu_fixed_grow;
  a.shrinkFn = cu_fixed_shrink;
  a.freeFn = cu_fixed_free;
  return a;
}

void cu_FixedAllocator_reset(cu_FixedAllocator *alloc) { alloc->used = 0; }
