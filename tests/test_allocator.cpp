#include <gtest/gtest.h>
extern "C" {
#include "memory/gpallocator.h"
#include "memory/allocator.h"
}

TEST(Allocator, GPABasic) {
  cu_GPAllocator gpa;
  cu_GPAllocator_Config cfg = {0};
  cfg.backingAllocator = cu_Allocator_none();
  cfg.bucketSize = 64;
  cu_Allocator alloc = cu_Allocator_GPAllocator(&gpa, cfg);

  cu_Slice_Optional mem = cu_Allocator_Alloc(alloc, 32, 8);
  ASSERT_TRUE(cu_Slice_is_some(&mem));
  memset(mem.value.ptr, 0xAA, mem.value.length);
  cu_Allocator_Free(alloc, mem.value);

  cu_GPAllocator_destroy(&gpa);
}
