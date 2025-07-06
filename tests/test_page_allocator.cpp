#include <gtest/gtest.h>
extern "C" {
#include "memory/page.h"
#include "memory/allocator.h"
}

TEST(PageAllocator, Basic) {
  cu_PageAllocator palloc;
  cu_Allocator a = cu_Allocator_PageAllocator(&palloc);
  cu_Slice_Optional mem = cu_Allocator_Alloc(a, 4096, 4096);
  ASSERT_TRUE(cu_Slice_is_some(&mem));
  memset(mem.value.ptr, 0, mem.value.length);
  cu_Allocator_Free(a, mem.value);
}
