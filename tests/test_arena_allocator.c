#include "collection/vector.h"
#include "memory/arenaallocator.h"
#include "memory/fixedallocator.h"
#include "unity.h"
#include <unity_internals.h>

static unsigned char backing[64 * 1024];

static void ArenaAllocator_LifoVectors(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {0};
  cfg.chunkSize = 256;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Vector_Result r1 =
      cu_Vector_create(alloc, CU_LAYOUT(int), Size_Optional_some(4));
  TEST_ASSERT_TRUE(cu_Vector_Result_is_ok(&r1));
  cu_Vector v1 = cu_Vector_Result_unwrap(&r1);

  cu_Vector_Result r2 =
      cu_Vector_create(alloc, CU_LAYOUT(float), Size_Optional_some(4));
  TEST_ASSERT_TRUE(cu_Vector_Result_is_ok(&r2));
  cu_Vector v2 = cu_Vector_Result_unwrap(&r2);

  int i = 42;
  float f = 3.14f;
  cu_Vector_push_back(&v1, &i);
  cu_Vector_push_back(&v2, &f);

  cu_Slice_Optional data = cu_Vector_data(&v2);
  TEST_ASSERT_TRUE(cu_Slice_Optional_is_some(&data));
  void *v2_ptr = data.value.ptr;

  cu_Vector_destroy(&v2);

  cu_Slice_Result reallocation_slice =
      cu_Allocator_Alloc(alloc, cu_Layout_create(16, 4));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&reallocation_slice));
  TEST_ASSERT_EQUAL(reallocation_slice.value.ptr, v2_ptr);
  cu_Allocator_Free(alloc, reallocation_slice.value);

  cu_Vector_destroy(&v1);
  cu_ArenaAllocator_destroy(&arena);
}

static void ArenaAllocator_NonLifoAlloc(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {0};
  cfg.chunkSize = 128;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Slice_Result a_res = cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&a_res));
  cu_Slice a = a_res.value;
  cu_Slice_Result b_res = cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&b_res));
  cu_Slice b = b_res.value;
  void *first = a.ptr;

  cu_Allocator_Free(alloc, a);
  cu_Slice_Result c_res = cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&c_res));
  cu_Slice c = c_res.value;
  TEST_ASSERT_TRUE((c.ptr) != (first));

  cu_Allocator_Free(alloc, b);
  cu_Allocator_Free(alloc, c);
  cu_ArenaAllocator_destroy(&arena);
}
static void ArenaAllocator_ChunkReuseStress(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {0};
  cfg.chunkSize = 128;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Slice_Result first_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(32, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&first_res));
  cu_Slice first = first_res.value;
  void *ptr = first.ptr;
  cu_Allocator_Free(alloc, first);

  for (int i = 0; i < 1000; ++i) {
    cu_Slice_Result slice_res =
        cu_Allocator_Alloc(alloc, cu_Layout_create(32, 8));
    TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&slice_res));
    cu_Slice slice = slice_res.value;
    TEST_ASSERT_EQUAL(slice.ptr, ptr);
    cu_Allocator_Free(alloc, slice);
    TEST_ASSERT_EQUAL(arena.current->prev, NULL);
  }

  cu_ArenaAllocator_destroy(&arena);
}

static void ArenaAllocator_ReuseOldChunk(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {0};
  cfg.chunkSize = 128;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);
  cu_Slice_Result first_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(32, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&first_res));
  cu_Slice first = first_res.value;
  void *ptr = first.ptr;

  cu_Slice_Result blocks_res[20];
  cu_Slice blocks[20];
  for (int i = 0; i < 20; ++i) {
    blocks_res[i] = cu_Allocator_Alloc(alloc, cu_Layout_create(112, 8));
    TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&blocks_res[i]));
    blocks[i] = blocks_res[i].value;
  }

  cu_Allocator_Free(alloc, first);

  cu_Slice_Result again_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(32, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&again_res));
  cu_Slice again = again_res.value;
  TEST_ASSERT_EQUAL(again.ptr, ptr);

  for (int i = 0; i < 20; ++i) {
    cu_Allocator_Free(alloc, blocks[i]);
  }
  cu_Allocator_Free(alloc, again);
  cu_ArenaAllocator_destroy(&arena);
}

static void ArenaAllocator_ResizeGrowInPlace(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {0};
  cfg.chunkSize = 128;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Slice_Result block_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&block_res));
  cu_Slice block = block_res.value;
  void *ptr = block.ptr;

  cu_Slice_Result resized =
      cu_Allocator_Resize(alloc, block, cu_Layout_create(32, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&resized));
  TEST_ASSERT_EQUAL(resized.value.ptr, ptr);

  cu_Allocator_Free(alloc, resized.value);
  cu_ArenaAllocator_destroy(&arena);
}

static void ArenaAllocator_ResizeShrinkInPlace(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {0};
  cfg.chunkSize = 128;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Slice_Result block_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(32, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&block_res));
  cu_Slice block = block_res.value;
  void *ptr = block.ptr;

  cu_Slice_Result resized =
      cu_Allocator_Resize(alloc, block, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&resized));
  TEST_ASSERT_EQUAL(resized.value.ptr, ptr);

  cu_Allocator_Free(alloc, resized.value);
  cu_ArenaAllocator_destroy(&arena);
}

static void ArenaAllocator_ResizeAllocNewBlock(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_ArenaAllocator arena;
  cu_ArenaAllocator_Config cfg = {0};
  cfg.chunkSize = 128;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_ArenaAllocator(&arena, cfg);

  cu_Slice_Result a_res = cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&a_res));
  cu_Slice a = a_res.value;
  cu_Slice_Result b_res = cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&b_res));
  cu_Slice b = b_res.value;
  void *old_ptr = a.ptr;

  cu_Slice_Result resized =
      cu_Allocator_Resize(alloc, a, cu_Layout_create(64, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&resized));
  TEST_ASSERT_TRUE((resized.value.ptr) != (old_ptr));

  cu_Allocator_Free(alloc, b);
  cu_Allocator_Free(alloc, resized.value);
  cu_ArenaAllocator_destroy(&arena);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(ArenaAllocator_LifoVectors);
  RUN_TEST(ArenaAllocator_NonLifoAlloc);
  RUN_TEST(ArenaAllocator_ChunkReuseStress);
  RUN_TEST(ArenaAllocator_ReuseOldChunk);
  RUN_TEST(ArenaAllocator_ResizeGrowInPlace);
  RUN_TEST(ArenaAllocator_ResizeShrinkInPlace);
  RUN_TEST(ArenaAllocator_ResizeAllocNewBlock);
  return UNITY_END();
}
