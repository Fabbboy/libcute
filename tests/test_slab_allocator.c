#include "memory/fixedallocator.h"
#include "memory/slab.h"
#include "unity.h"
#include <nostd.h>
#include <unity_internals.h>

static unsigned char backing[512 * 1024];

static void SlabAllocator_Basic(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_IoSlice_Result a_res = cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&a_res));
  cu_Slice a = a_res.value;
  cu_IoSlice_Result b_res = cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&b_res));
  cu_Slice b = b_res.value;
  TEST_ASSERT_TRUE((a.ptr) != (b.ptr));

  cu_SlabAllocator_destroy(&slab);
}

static void SlabAllocator_BigAllocation(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_IoSlice_Result big_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(256, 8));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&big_res));

  cu_SlabAllocator_destroy(&slab);
}

static void SlabAllocator_GrowInPlace(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_IoSlice_Result mem_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&mem_res));
  cu_Slice mem = mem_res.value;
  cu_Memory_memset(mem.ptr, 0xAA, mem.length);

  cu_IoSlice_Result resized_res =
      cu_Allocator_Grow(alloc, mem, cu_Layout_create(32, 8));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&resized_res));
  TEST_ASSERT_EQUAL(resized_res.value.ptr, mem.ptr);

  cu_SlabAllocator_destroy(&slab);
}

static void SlabAllocator_GrowFallback(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 32;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_IoSlice_Result mem_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&mem_res));
  cu_Slice mem = mem_res.value;
  cu_IoSlice_Result resized =
      cu_Allocator_Grow(alloc, mem, cu_Layout_create(128, 8));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&resized));
  TEST_ASSERT_NOT_EQUAL(resized.value.ptr, mem.ptr);

  cu_SlabAllocator_destroy(&slab);
}

static void SlabAllocator_ReuseFreed(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_IoSlice_Result a_res = cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&a_res));
  cu_Slice a = a_res.value;
  void *ptr = a.ptr;
  cu_Allocator_Free(alloc, a);

  cu_IoSlice_Result b_res = cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&b_res));
  TEST_ASSERT_EQUAL(b_res.value.ptr, ptr);

  cu_SlabAllocator_destroy(&slab);
}

static void SlabAllocator_Alignment(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_SlabAllocator slab;
  cu_SlabAllocator_Config cfg = {0};
  cfg.slabSize = 64;
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_SlabAllocator(&slab, cfg);

  cu_IoSlice_Result res = cu_Allocator_Alloc(alloc, cu_Layout_create(8, 16));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&res));
  TEST_ASSERT_EQUAL((uintptr_t)res.value.ptr % 16, 0u);

  cu_SlabAllocator_destroy(&slab);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(SlabAllocator_Basic);
  RUN_TEST(SlabAllocator_BigAllocation);
  RUN_TEST(SlabAllocator_GrowInPlace);
  RUN_TEST(SlabAllocator_GrowFallback);
  RUN_TEST(SlabAllocator_ReuseFreed);
  RUN_TEST(SlabAllocator_Alignment);
  return UNITY_END();
}
