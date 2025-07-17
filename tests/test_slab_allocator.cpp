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

  cu_Slice_Optional a = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_Optional_is_some(&a));
  cu_Slice_Optional b = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_Optional_is_some(&b));
  EXPECT_NE(a.value.ptr, b.value.ptr);

  cu_SlabAllocator_destroy(&slab);
}

TEST(SlabAllocator, BigAllocation) {
  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_Slice_Optional big = cu_Allocator_Alloc(alloc, 256, 8);
  ASSERT_TRUE(cu_Slice_Optional_is_some(&big));

  cu_SlabAllocator_destroy(&slab);
}

TEST(SlabAllocator, Resize) {
  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_Slice_Optional mem = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_Optional_is_some(&mem));
  memset(mem.value.ptr, 0xAA, mem.value.length);

  cu_Slice_Optional resized = cu_Allocator_Resize(alloc, mem.value, 128, 8);
  ASSERT_TRUE(cu_Slice_Optional_is_some(&resized));
  EXPECT_EQ(((unsigned char *)resized.value.ptr)[0], 0xAA);

  cu_SlabAllocator_destroy(&slab);
}

TEST(SlabAllocator, ManyAllocations) {
  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 32;
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  for (int i = 0; i < 1000; ++i) {
    cu_Slice_Optional s = cu_Allocator_Alloc(alloc, 8, 4);
    ASSERT_TRUE(cu_Slice_Optional_is_some(&s));
  }

  cu_SlabAllocator_destroy(&slab);
}
