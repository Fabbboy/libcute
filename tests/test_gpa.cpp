#include <gtest/gtest.h>
#include <vector>
extern "C" {
#include "io/error.h"
#include "memory/allocator.h"
#include "memory/fixedallocator.h"
#include "memory/gpallocator.h"
}
#include "utility.h"

static unsigned char backing[512 * 1024 * 1024];

TEST(Allocator, GPALargeAllocFree) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_GPAllocator gpa;
  cu_GPAllocator_Config cfg = {0};
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
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

typedef struct {
  int x;
  int y;
} cu_Point;

TEST(Allocator, NormalAllocAndFree) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_GPAllocator gpa;
  cu_GPAllocator_Config cfg = {0};
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_GPAllocator(&gpa, cfg);

  cu_Slice_Result res =
      cu_Allocator_Alloc(alloc, sizeof(cu_Point), alignof(cu_Point));
  ASSERT_TRUE(cu_Slice_result_is_ok(&res));

  cu_Slice mem = res.value;
  ASSERT_TRUE(mem.ptr != NULL);
  ASSERT_EQ(mem.length, sizeof(cu_Point));

  cu_Point *point = (cu_Point *)mem.ptr;
  point->x = 42;
  point->y = 84;

  EXPECT_EQ(point->x, 42);
  EXPECT_EQ(point->y, 84);

  cu_Allocator_Free(alloc, mem);
  cu_GPAllocator_destroy(&gpa);
}

TEST(Allocator, DoubleFree) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_GPAllocator gpa;
  cu_GPAllocator_Config cfg = {0};
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_GPAllocator(&gpa, cfg);

  cu_Slice_Result res =
      cu_Allocator_Alloc(alloc, sizeof(cu_Point), alignof(cu_Point));
  ASSERT_TRUE(cu_Slice_result_is_ok(&res));

  cu_Slice mem = res.value;
  ASSERT_TRUE(mem.ptr != NULL);

  // Free the memory
  cu_Allocator_Free(alloc, mem);

  // Attempt to free it again (should not crash)
  cu_Allocator_Free(alloc, mem);

  cu_GPAllocator_destroy(&gpa);
}

TEST(Allocator, Exhaustion) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_GPAllocator gpa;
  cu_GPAllocator_Config cfg = {0};
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_GPAllocator(&gpa, cfg);

  // Allocate a large block
  size_t large_size = 512 * 1024 * 1024; // 100 MiB
  cu_Slice_Result res =
      cu_Allocator_Alloc(alloc, large_size, alignof(cu_Point));

  ASSERT_FALSE(cu_Slice_result_is_ok(&res));
  EXPECT_EQ(
      cu_Slice_result_unwrap_error(&res).kind, CU_IO_ERROR_KIND_OUT_OF_MEMORY);
}