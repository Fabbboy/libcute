#include <gtest/gtest.h>
#include <vector>
extern "C" {
#include "memory/allocator.h"
#include "memory/gpallocator.h"
}

TEST(Allocator, GPALargeAllocFree) {
  cu_GPAllocator gpa;
  cu_GPAllocator_Config cfg = {0};
  cfg.backingAllocator = cu_Allocator_Optional_none();
  cu_Allocator alloc = cu_Allocator_GPAllocator(&gpa, cfg);

  const size_t big = 100 * 1024 * 1024; // 100 MiB
  cu_Slice_Result mem_res = cu_Allocator_Alloc(alloc, big, 16);
  ASSERT_TRUE(cu_Slice_result_is_ok(&mem_res));
  cu_Slice mem = mem_res.value;
  cu_Memory_memset(mem.ptr, 0xCD, mem.length);
  cu_Allocator_Free(alloc, mem);

  const size_t small = 4096; // 4 KiB blocks
  const size_t total = 100 * 1024 * 1024;
  const size_t count = total / small;
  std::vector<cu_Slice> blocks;
  blocks.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    cu_Slice_Result s_res = cu_Allocator_Alloc(alloc, small, 16);
    ASSERT_TRUE(cu_Slice_result_is_ok(&s_res));
    cu_Slice s = s_res.value;
    cu_Memory_memset(s.ptr, 0xEF, s.length);
    blocks.push_back(s);
  }
  for (cu_Slice s : blocks) {
    cu_Allocator_Free(alloc, s);
  }

  cu_GPAllocator_destroy(&gpa);
}
