#include <gtest/gtest.h>
extern "C" {
#include "collection/vector.h"
#include "memory/arenaallocator.h"
}

TEST(ArenaAllocator, LifoVectors) {
  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {};
  cfg.chunkSize = 256;
  cfg.backingAllocator = cu_Allocator_Optional_none();
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Vector_Result r1 =
      cu_Vector_create(alloc, CU_LAYOUT(int), Size_Optional_some(4));
  ASSERT_TRUE(cu_Vector_result_is_ok(&r1));
  cu_Vector v1 = cu_Vector_result_unwrap(&r1);

  cu_Vector_Result r2 =
      cu_Vector_create(alloc, CU_LAYOUT(float), Size_Optional_some(4));
  ASSERT_TRUE(cu_Vector_result_is_ok(&r2));
  cu_Vector v2 = cu_Vector_result_unwrap(&r2);

  int i = 42;
  float f = 3.14f;
  cu_Vector_push_back(&v1, &i);
  cu_Vector_push_back(&v2, &f);

  cu_Slice_Optional data = cu_Vector_data(&v2);
  ASSERT_TRUE(cu_Slice_Optional_is_some(&data));
  void *v2_ptr = data.value.ptr;

  cu_Vector_destroy(&v2);

  cu_Slice_Result reallocation_slice = cu_Allocator_Alloc(alloc, 16, 4);
  ASSERT_TRUE(cu_Slice_result_is_ok(&reallocation_slice));
  EXPECT_EQ(reallocation_slice.value.ptr, v2_ptr);
  cu_Allocator_Free(alloc, reallocation_slice.value);

  cu_Vector_destroy(&v1);
  cu_ArenaAllocator_destroy(&arena);
}

TEST(ArenaAllocator, NonLifoAlloc) {
  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {};
  cfg.chunkSize = 128;
  cfg.backingAllocator = cu_Allocator_Optional_none();
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Slice_Result a_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&a_res));
  cu_Slice a = a_res.value;
  cu_Slice_Result b_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&b_res));
  cu_Slice b = b_res.value;
  void *first = a.ptr;

  cu_Allocator_Free(alloc, a);
  cu_Slice_Result c_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&c_res));
  cu_Slice c = c_res.value;
  EXPECT_NE(c.ptr, first);

  cu_Allocator_Free(alloc, b);
  cu_Allocator_Free(alloc, c);
  cu_ArenaAllocator_destroy(&arena);
}
TEST(ArenaAllocator, ChunkReuseStress) {
  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {};
  cfg.chunkSize = 128;
  cfg.backingAllocator = cu_Allocator_Optional_none();
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Slice_Result first_res = cu_Allocator_Alloc(alloc, 32, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&first_res));
  cu_Slice first = first_res.value;
  void *ptr = first.ptr;
  cu_Allocator_Free(alloc, first);

  for (int i = 0; i < 1000; ++i) {
    cu_Slice_Result slice_res = cu_Allocator_Alloc(alloc, 32, 8);
    ASSERT_TRUE(cu_Slice_result_is_ok(&slice_res));
    EXPECT_EQ(slice_res.value.ptr, ptr);
    cu_Allocator_Free(alloc, slice_res.value);
    EXPECT_EQ(arena.current->prev, nullptr);
  }

  cu_ArenaAllocator_destroy(&arena);
}

TEST(ArenaAllocator, ResizeGrowInPlace) {
  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {};
  cfg.chunkSize = 128;
  cfg.backingAllocator = cu_Allocator_Optional_none();
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Slice_Result block_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&block_res));
  cu_Slice block = block_res.value;
  void *ptr = block.ptr;

  cu_Slice_Result resized =
      cu_Allocator_Resize(alloc, block, 32, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&resized));
  EXPECT_EQ(resized.value.ptr, ptr);

  cu_Allocator_Free(alloc, resized.value);
  cu_ArenaAllocator_destroy(&arena);
}

TEST(ArenaAllocator, ResizeShrinkInPlace) {
  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {};
  cfg.chunkSize = 128;
  cfg.backingAllocator = cu_Allocator_Optional_none();
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Slice_Result block_res = cu_Allocator_Alloc(alloc, 32, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&block_res));
  cu_Slice block = block_res.value;
  void *ptr = block.ptr;

  cu_Slice_Result resized =
      cu_Allocator_Resize(alloc, block, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&resized));
  EXPECT_EQ(resized.value.ptr, ptr);

  cu_Allocator_Free(alloc, resized.value);
  cu_ArenaAllocator_destroy(&arena);
}

TEST(ArenaAllocator, ResizeAllocNewBlock) {
  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {};
  cfg.chunkSize = 128;
  cfg.backingAllocator = cu_Allocator_Optional_none();
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Slice_Result a_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&a_res));
  cu_Slice a = a_res.value;
  cu_Slice_Result b_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&b_res));
  cu_Slice b = b_res.value;
  void *old_ptr = a.ptr;

  cu_Slice_Result resized =
      cu_Allocator_Resize(alloc, a, 64, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&resized));
  EXPECT_NE(resized.value.ptr, old_ptr);

  cu_Allocator_Free(alloc, b);
  cu_Allocator_Free(alloc, resized.value);
  cu_ArenaAllocator_destroy(&arena);
}
