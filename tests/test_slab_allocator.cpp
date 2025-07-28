#include <gtest/gtest.h>
#include <nostd.h>
extern "C" {
#include "memory/fixedallocator.h"
#include "memory/slab.h"
}

static unsigned char backing[512 * 1024];

TEST(SlabAllocator, Basic) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_Slice_Result a_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  ASSERT_TRUE(cu_Slice_Result_is_ok(&a_res));
  cu_Slice a = a_res.value;
  cu_Slice_Result b_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  ASSERT_TRUE(cu_Slice_Result_is_ok(&b_res));
  cu_Slice b = b_res.value;
  EXPECT_NE(a.ptr, b.ptr);

  cu_SlabAllocator_destroy(&slab);
}

TEST(SlabAllocator, BigAllocation) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_Slice_Result big_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(256, 8));
  ASSERT_TRUE(cu_Slice_Result_is_ok(&big_res));

  cu_SlabAllocator_destroy(&slab);
}

TEST(SlabAllocator, Resize) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_Slice_Result mem_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  ASSERT_TRUE(cu_Slice_Result_is_ok(&mem_res));
  cu_Slice mem = mem_res.value;
  cu_Memory_memset(mem.ptr, 0xAA, mem.length);

  cu_Slice_Result resized_res =
      cu_Allocator_Resize(alloc, mem, cu_Layout_create(128, 8));
  ASSERT_TRUE(cu_Slice_Result_is_ok(&resized_res));
  EXPECT_EQ(((unsigned char *)resized_res.value.ptr)[0], 0xAA);

  cu_SlabAllocator_destroy(&slab);
}

TEST(SlabAllocator, ManyAllocations) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 32;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  for (int i = 0; i < 1000; ++i) {
    cu_Slice_Result s_res =
        cu_Allocator_Alloc(alloc, cu_Layout_create(8, 4));
    ASSERT_TRUE(cu_Slice_Result_is_ok(&s_res));
  }

  cu_SlabAllocator_destroy(&slab);
}

TEST(SlabAllocator, ReuseFreed) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_Slice_Result a_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  ASSERT_TRUE(cu_Slice_Result_is_ok(&a_res));
  cu_Slice a = a_res.value;
  void *ptr = a.ptr;
  cu_Allocator_Free(alloc, a);

  cu_Slice_Result b_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  ASSERT_TRUE(cu_Slice_Result_is_ok(&b_res));
  EXPECT_EQ(b_res.value.ptr, ptr);

  cu_SlabAllocator_destroy(&slab);
}

TEST(SlabAllocator, Alignment) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_Slice_Result res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(8, 16));
  ASSERT_TRUE(cu_Slice_Result_is_ok(&res));
  EXPECT_EQ((uintptr_t)res.value.ptr % 16, 0u);

  cu_SlabAllocator_destroy(&slab);
}
