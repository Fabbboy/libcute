#include <gtest/gtest.h>
#include <string.h>
extern "C" {
#include "memory/slab.h"
}

TEST(SlabAllocator, Basic) {
  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cfg.backingAllocator = cu_Allocator_Optional_none();
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_Slice_Result a_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&a_res));
  cu_Slice a = a_res.value;
  cu_Slice_Result b_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&b_res));
  cu_Slice b = b_res.value;
  EXPECT_NE(a.ptr, b.ptr);

  cu_SlabAllocator_destroy(&slab);
}

TEST(SlabAllocator, BigAllocation) {
  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_Slice_Result big_res = cu_Allocator_Alloc(alloc, 256, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&big_res));

  cu_SlabAllocator_destroy(&slab);
}

TEST(SlabAllocator, Resize) {
  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_Slice_Result mem_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&mem_res));
  cu_Slice mem = mem_res.value;
  memset(mem.ptr, 0xAA, mem.length);

  cu_Slice_Result resized_res = cu_Allocator_Resize(alloc, mem, 128, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&resized_res));
  EXPECT_EQ(((unsigned char *)resized_res.value.ptr)[0], 0xAA);

  cu_SlabAllocator_destroy(&slab);
}

TEST(SlabAllocator, ManyAllocations) {
  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 32;
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  for (int i = 0; i < 1000; ++i) {
    cu_Slice_Result s_res = cu_Allocator_Alloc(alloc, 8, 4);
    ASSERT_TRUE(cu_Slice_result_is_ok(&s_res));
  }

  cu_SlabAllocator_destroy(&slab);
}

TEST(SlabAllocator, ReuseFreed) {
  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_Slice_Result a_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&a_res));
  cu_Slice a = a_res.value;
  void *ptr = a.ptr;
  cu_Allocator_Free(alloc, a);

  cu_Slice_Result b_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&b_res));
  EXPECT_EQ(b_res.value.ptr, ptr);

  cu_SlabAllocator_destroy(&slab);
}

TEST(SlabAllocator, Alignment) {
  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_Slice_Result res = cu_Allocator_Alloc(alloc, 8, 16);
  ASSERT_TRUE(cu_Slice_result_is_ok(&res));
  EXPECT_EQ((uintptr_t)res.value.ptr % 16, 0u);

  cu_SlabAllocator_destroy(&slab);
}
