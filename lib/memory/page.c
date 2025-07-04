#include "memory/page.h"

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#else
#error "Unsupported platform for page allocator"
#endif

cu_Allocator cu_Allocator_PageAllocator(cu_PageAllocator *allocator) {
  cu_Allocator alloc;
  alloc.self = allocator;
  alloc.allocFn = NULL;
  alloc.resizeFn = NULL;
  alloc.freeFn = NULL;
  return alloc;
}