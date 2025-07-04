#include "memory/page.h"
#include "macro.h"
#include "slice.h"
#include <stddef.h>

#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
#else
#error "Unsupported platform for page allocator"
#endif

static Slice_Optional cu_PageAllocator_Alloc(
    void *self, size_t size, size_t alignment) {
  cu_PageAllocator *allocator = (cu_PageAllocator *)self;
  if (size == 0) {
    return Slice_none();
  }

  size_t alignedSize = CU_ALIGN_UP(size, allocator->pageSize);

  void *ptr = NULL;
#if defined(__linux__) || defined(__APPLE__)
  ptr = mmap(NULL, alignedSize, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#else
#error "Unsupported platform for page allocator"
#endif

  CU_IF_NULL(ptr) { return Slice_none(); }
}

cu_Allocator cu_Allocator_PageAllocator(cu_PageAllocator *allocator) {
  size_t pageSize = 0;
#if defined(__linux__) || defined(__APPLE__)
  pageSize = sysconf(_SC_PAGESIZE);
#else
#error "Unsupported platform for page allocator"
#endif

  allocator->pageSize = pageSize;

  cu_Allocator alloc;
  alloc.self = allocator;
  alloc.allocFn = NULL;
  alloc.resizeFn = NULL;
  alloc.freeFn = NULL;
  return alloc;
}