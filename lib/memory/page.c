#include "memory/page.h"
#include "macro.h"
#include "slice.h"
#include <stddef.h>
#include <string.h>

#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#error "Unsupported platform for page allocator"
#endif

static void cu_PageAllocator_Free(void *self, cu_Slice mem);

static Slice_Optional cu_PageAllocator_Alloc(
    void *self, size_t size, size_t alignment) {
  CU_UNUSED(alignment);

  cu_PageAllocator *allocator = (cu_PageAllocator *)self;
  if (size == 0) {
    return Slice_none();
  }

  size_t alignedSize = CU_ALIGN_UP(size, allocator->pageSize);

  void *ptr = NULL;
#if defined(__linux__) || defined(__APPLE__)
  ptr = mmap(NULL, alignedSize, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED) {
    ptr = NULL;
  }
#elif defined(_WIN32)
  ptr =
      VirtualAlloc(NULL, alignedSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
#error "Unsupported platform for page allocator"
#endif

  CU_IF_NULL(ptr) { return Slice_none(); }

  cu_Slice slice = cu_Slice_create(ptr, alignedSize);
  return Slice_some(slice);
}

static Slice_Optional cu_PageAllocator_Resize(
    void *self, cu_Slice mem, size_t size, size_t alignment) {
  CU_UNUSED(alignment);

  if (size == 0) {
    cu_PageAllocator_Free(self, mem);
    return Slice_none();
  }

  if (mem.length == size) {
    return Slice_some(mem);
  }

  Slice_Optional newMem = cu_PageAllocator_Alloc(self, size, alignment);
  if (Slice_is_none(&newMem)) {
    return Slice_none();
  }

  size_t copySize = mem.length < size ? mem.length : size;
  memmove(newMem.value.ptr, mem.ptr, copySize);
  cu_PageAllocator_Free(self, mem);
  return newMem;
}

static void cu_PageAllocator_Free(void *self, cu_Slice mem) {
  if (mem.ptr == NULL) {
    return;
  }
#if defined(__linux__) || defined(__APPLE__)
  cu_PageAllocator *allocator = (cu_PageAllocator *)self;
  size_t alignedSize = CU_ALIGN_UP(mem.length, allocator->pageSize);
  munmap(mem.ptr, alignedSize);
#elif defined(_WIN32)
  VirtualFree(mem.ptr, 0, MEM_RELEASE);
#else
#error "Unsupported platform for page allocator"
#endif
}

cu_Allocator cu_Allocator_PageAllocator(cu_PageAllocator *allocator) {
  size_t pageSize = 0;
#if defined(__linux__) || defined(__APPLE__)
  pageSize = sysconf(_SC_PAGESIZE);
#elif defined(_WIN32)
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  pageSize = info.dwPageSize;
#else
#error "Unsupported platform for page allocator"
#endif

  allocator->pageSize = pageSize;

  cu_Allocator alloc;
  alloc.self = allocator;
  alloc.allocFn = cu_PageAllocator_Alloc;
  alloc.resizeFn = cu_PageAllocator_Resize;
  alloc.freeFn = cu_PageAllocator_Free;
  return alloc;
}