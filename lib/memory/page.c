#if CU_PLAT_LINUX
#define _GNU_SOURCE
#endif
#include "memory/page.h"
#include "io/error.h"
#include "macro.h"
#include <nostd.h>
#include <stddef.h>
#if !CU_FREESTANDING && !CU_PLAT_WASM
#if CU_PLAT_WINDOWS
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif
#endif

#if !CU_FREESTANDING && !CU_PLAT_WASM

static void cu_PageAllocator_Free(void *self, cu_Slice mem) {
  CU_UNUSED(self);
  CU_IF_NULL(mem.ptr) { return; }
#if CU_PLAT_WINDOWS
  VirtualFree(mem.ptr, 0, MEM_RELEASE);
#else
  munmap(mem.ptr, mem.length);
#endif
}

static cu_IoSlice_Result cu_PageAllocator_Alloc(void *self, cu_Layout layout) {
  cu_PageAllocator *allocator = (cu_PageAllocator *)self;
  if (layout.elem_size == 0) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }
  size_t size = layout.elem_size;
  size_t aligned_size = CU_ALIGN_UP(size, allocator->pageSize);
#if CU_PLAT_WINDOWS
  void *ptr = VirtualAlloc(
      NULL, aligned_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!ptr) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }
#else
  void *ptr = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANON, -1, 0);
  if (ptr == MAP_FAILED) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_IoSlice_Result_error(err);
  }
#endif
  return cu_IoSlice_Result_ok(cu_Slice_create(ptr, aligned_size));
}

static cu_IoSlice_Result cu_PageAllocator_Grow(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  cu_PageAllocator *allocator = (cu_PageAllocator *)self;
  size_t aligned_size = CU_ALIGN_UP(new_layout.elem_size, allocator->pageSize);
  cu_IoSlice_Result res = cu_PageAllocator_Alloc(
      self, cu_Layout_create(aligned_size, allocator->pageSize));
  if (!cu_IoSlice_Result_is_ok(&res)) {
    return res;
  }
  cu_Memory_smemcpy(res.value, old_mem);
  cu_PageAllocator_Free(self, old_mem);
  return res;
}

static cu_IoSlice_Result cu_PageAllocator_Shrink(
    void *self, cu_Slice old_mem, cu_Layout new_layout) {
  cu_PageAllocator *allocator = (cu_PageAllocator *)self;
  size_t aligned_size = CU_ALIGN_UP(new_layout.elem_size, allocator->pageSize);
  cu_IoSlice_Result res = cu_PageAllocator_Alloc(
      self, cu_Layout_create(aligned_size, allocator->pageSize));
  if (!cu_IoSlice_Result_is_ok(&res)) {
    return res;
  }
  size_t copy = CU_MIN(aligned_size, old_mem.length);
  cu_Memory_smemcpy(cu_Slice_create(res.value.ptr, copy),
      cu_Slice_create(old_mem.ptr, copy));
  cu_PageAllocator_Free(self, old_mem);
  return res;
}

cu_Allocator cu_Allocator_PageAllocator(cu_PageAllocator *allocator) {
#if CU_PLAT_WINDOWS
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  allocator->pageSize = (size_t)info.dwPageSize;
#else
  long ps = sysconf(_SC_PAGESIZE);
  allocator->pageSize = ps > 0 ? (size_t)ps : 4096;
#endif
  cu_Allocator alloc = {0};
  alloc.self = allocator;
  alloc.allocFn = cu_PageAllocator_Alloc;
  alloc.growFn = cu_PageAllocator_Grow;
  alloc.shrinkFn = cu_PageAllocator_Shrink;
  alloc.freeFn = cu_PageAllocator_Free;
  return alloc;
}

#endif
