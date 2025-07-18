#include <gtest/gtest.h>
extern "C" {
#include "memory/allocator.h"
#include "memory/gpallocator.h"
}

TEST(Allocator, GPABasic) {
  cu_GPAllocator gpa;
  cu_GPAllocator_Config cfg = {0};
  cfg.backingAllocator = cu_Allocator_Optional_none();
  cfg.bucketSize = 64;
  cu_Allocator alloc = cu_Allocator_GPAllocator(&gpa, cfg);

  cu_Slice_Result mem_res = cu_Allocator_Alloc(alloc, 32, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&mem_res));
  cu_Slice mem = mem_res.value;
  memset(mem.ptr, 0xAA, mem.length);
  cu_Allocator_Free(alloc, mem);

  cu_GPAllocator_destroy(&gpa);
}
