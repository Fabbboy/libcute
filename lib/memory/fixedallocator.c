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
  size_t req_align = alignment > _Alignof(struct cu_FixedAllocator_Header)
                         ? alignment
                         : _Alignof(struct cu_FixedAllocator_Header);
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

static cu_IoSlice_Result cu_fixed_resize(
    void *self, cu_Slice mem, cu_Layout layout) {
  cu_FixedAllocator *alloc = (cu_FixedAllocator *)self;
  CU_IF_NULL(mem.ptr) { return cu_fixed_alloc(self, layout); }
  if (layout.elem_size == 0) {
    cu_fixed_free(self, mem);
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }

  size_t size = layout.elem_size;
  size_t alignment = layout.alignment;

  const size_t header_size = sizeof(struct cu_FixedAllocator_Header);
  struct cu_FixedAllocator_Header *hdr =
      (struct cu_FixedAllocator_Header *)((unsigned char *)mem.ptr -
                                          header_size);
  if ((unsigned char *)mem.ptr + mem.length ==
      (unsigned char *)alloc->buffer.ptr + alloc->used) {
    size_t avail = alloc->buffer.length - (alloc->used - mem.length);
    if (size <= mem.length + avail) {
      alloc->used = (alloc->used - mem.length) + size;
      return cu_IoSlice_Result_ok(cu_Slice_create(mem.ptr, size));
    }
  }

  cu_IoSlice_Result new_mem =
      cu_fixed_alloc(self, cu_Layout_create(size, alignment));
  if (!cu_IoSlice_Result_is_ok(&new_mem)) {
    return new_mem;
  }
  cu_Memory_memcpy(new_mem.value.ptr,
      cu_Slice_create(mem.ptr, mem.length < size ? mem.length : size));
  // adjust used to release old memory
  alloc->used = hdr->prev_offset;
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
  a.resizeFn = cu_fixed_resize;
  a.freeFn = cu_fixed_free;
  return a;
}

void cu_FixedAllocator_reset(cu_FixedAllocator *alloc) { alloc->used = 0; }
