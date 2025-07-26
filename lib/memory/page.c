#include "memory/page.h"
#include "io/error.h"
#include "macro.h"
#include "nostd.h"
#include <nostd.h>
#include <stddef.h>
#include <stdint.h>

#ifndef CU_NO_STD

#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#error "Unsupported platform for page allocator"
#endif

static void cu_PageAllocator_Free(void *self, cu_Slice mem);

static cu_Slice_Result cu_PageAllocator_Alloc(
    void *self, size_t size, size_t alignment) {
  cu_PageAllocator *allocator = (cu_PageAllocator *)self;
  if (size == 0) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  const size_t page_size = allocator->pageSize;
  const size_t aligned_len = CU_ALIGN_UP(size, page_size);
  const size_t max_drop_len =
      alignment - (alignment < page_size ? alignment : page_size);
  const size_t overalloc_len =
      (max_drop_len <= aligned_len - size)
          ? aligned_len
          : CU_ALIGN_UP(aligned_len + max_drop_len, page_size);

  void *base_ptr = NULL;
#if defined(__linux__) || defined(__APPLE__)
  base_ptr = mmap(NULL, overalloc_len, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (base_ptr == MAP_FAILED) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }
#elif defined(_WIN32)
  base_ptr = VirtualAlloc(
      NULL, overalloc_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (base_ptr == NULL) {
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_OUT_OF_MEMORY, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }
#endif

  uintptr_t base_addr = (uintptr_t)base_ptr;
  uintptr_t aligned_addr = CU_ALIGN_UP(base_addr, alignment);
  size_t prefix_size = aligned_addr - base_addr;
  size_t suffix_size = overalloc_len - prefix_size - aligned_len;

#if defined(__linux__) || defined(__APPLE__)
  if (prefix_size > 0) {
    munmap((void *)base_addr, prefix_size);
  }
  if (suffix_size > 0) {
    munmap((void *)(aligned_addr + aligned_len), suffix_size);
  }
#elif defined(_WIN32)
  if (prefix_size > 0) {
    VirtualFree((void *)base_addr, prefix_size, MEM_DECOMMIT);
  }
  if (suffix_size > 0) {
    VirtualFree(
        (void *)(aligned_addr + aligned_len), suffix_size, MEM_DECOMMIT);
  }
#endif

  return cu_Slice_result_ok(cu_Slice_create((void *)aligned_addr, aligned_len));
}

static cu_Slice_Result cu_PageAllocator_Resize(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  if (size == 0) {
    cu_PageAllocator_Free(self, mem);
    cu_Io_Error err = {
        .kind = CU_IO_ERROR_KIND_INVALID_INPUT, .errnum = Size_Optional_none()};
    return cu_Slice_result_error(err);
  }

  if (mem.length == size) {
    return cu_Slice_result_ok(mem);
  }

  cu_Slice_Result new_mem = cu_PageAllocator_Alloc(self, size, alignment);
  if (!cu_Slice_result_is_ok(&new_mem)) {
    return new_mem;
  }

  size_t copy_size = (mem.length < size) ? mem.length : size;
  memmove(new_mem.value.ptr, mem.ptr, copy_size);
  cu_PageAllocator_Free(self, mem);
  return new_mem;
}

static void cu_PageAllocator_Free(void *self, cu_Slice mem) {
  if (mem.ptr == NULL) {
    return;
  }
#if defined(__linux__) || defined(__APPLE__)
  cu_PageAllocator *allocator = (cu_PageAllocator *)self;
  size_t aligned_size = CU_ALIGN_UP(mem.length, allocator->pageSize);
  munmap(mem.ptr, aligned_size);
#elif defined(_WIN32)
  VirtualFree(mem.ptr, 0, MEM_RELEASE);
#endif
}

cu_Allocator cu_Allocator_PageAllocator(cu_PageAllocator *allocator) {
  size_t page_size = 0;
#if defined(__linux__) || defined(__APPLE__)
  page_size = sysconf(_SC_PAGESIZE);
#elif defined(_WIN32)
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  page_size = info.dwPageSize;
#endif

  allocator->pageSize = page_size;

  cu_Allocator alloc;
  alloc.self = allocator;
  alloc.allocFn = cu_PageAllocator_Alloc;
  alloc.resizeFn = cu_PageAllocator_Resize;
  alloc.freeFn = cu_PageAllocator_Free;
  return alloc;
}

#endif