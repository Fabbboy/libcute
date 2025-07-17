#include <gtest/gtest.h>
extern "C" {
#include "collection/vector.h"
#include "memory/arenaallocator.h"
}

TEST(ArenaAllocator, LifoVectors) {
  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {0};
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

  void *v2_ptr = v2.data.value.ptr;

  cu_Vector_destroy(&v2);

  cu_Slice_Optional tmp = cu_Allocator_Alloc(alloc, 16, 4);
  ASSERT_TRUE(cu_Slice_Optional_is_some(&tmp));
  EXPECT_EQ(tmp.value.ptr, v2_ptr);
  cu_Allocator_Free(alloc, tmp.value);

  cu_Vector_destroy(&v1);
  cu_ArenaAllocator_destroy(&arena);
}

TEST(ArenaAllocator, NonLifoAlloc) {
  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {0};
  cfg.chunkSize = 128;
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Slice_Optional a = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_Optional_is_some(&a));
  cu_Slice_Optional b = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_Optional_is_some(&b));
  void *first = a.value.ptr;

  cu_Allocator_Free(alloc, a.value);
  cu_Slice_Optional c = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_Optional_is_some(&c));
  EXPECT_NE(c.value.ptr, first);

  cu_Allocator_Free(alloc, b.value);
  cu_Allocator_Free(alloc, c.value);
  cu_ArenaAllocator_destroy(&arena);
}
TEST(ArenaAllocator, ChunkReuseStress) {
  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {0};
  cfg.chunkSize = 128;
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Slice_Optional first = cu_Allocator_Alloc(alloc, 32, 8);
  ASSERT_TRUE(cu_Slice_Optional_is_some(&first));
  void *ptr = first.value.ptr;
  cu_Allocator_Free(alloc, first.value);

  for (int i = 0; i < 1000; ++i) {
    cu_Slice_Optional tmp = cu_Allocator_Alloc(alloc, 32, 8);
    ASSERT_TRUE(cu_Slice_Optional_is_some(&tmp));
    EXPECT_EQ(tmp.value.ptr, ptr);
    cu_Allocator_Free(alloc, tmp.value);
    EXPECT_EQ(arena.current->prev, nullptr);
  }

  cu_ArenaAllocator_destroy(&arena);
}
