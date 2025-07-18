#include <gtest/gtest.h>
extern "C" {
#include "memory/allocator.h"
#include "memory/page.h"
}

TEST(PageAllocator, Basic) {
  cu_PageAllocator palloc;
  cu_Allocator a = cu_Allocator_PageAllocator(&palloc);
  cu_Slice_Result mem_res = cu_Allocator_Alloc(a, 4096, 4096);
  ASSERT_TRUE(cu_Slice_result_is_ok(&mem_res));
  cu_Slice mem = mem_res.value;
  memset(mem.ptr, 0, mem.length);
  cu_Allocator_Free(a, mem);
}
