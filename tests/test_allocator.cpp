#include <gtest/gtest.h>
extern "C" {
#include "memory/allocator.h"
#include "memory/fixedallocator.h"
#include "memory/gpallocator.h"
}

static char buffer[1024 * 1024];

TEST(Allocator, GPABasic) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc =
      cu_Allocator_FixedAllocator(&fa, cu_Slice_create(buffer, sizeof(buffer)));

  cu_GPAllocator gpa;
  cu_GPAllocator_Config cfg = {0};
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cfg.bucketSize = 64;
  cu_Allocator alloc = cu_Allocator_GPAllocator(&gpa, cfg);

  cu_Slice_Result mem_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(32, 8));
  ASSERT_TRUE(cu_Slice_Result_is_ok(&mem_res));
  cu_Slice mem = mem_res.value;
  cu_Memory_memset(mem.ptr, 0xAA, mem.length);
  cu_Allocator_Free(alloc, mem);

  cu_GPAllocator_destroy(&gpa);
}
