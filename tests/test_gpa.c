#include "test_common.h"
#include <stdlib.h>
#include "io/error.h"
#include "memory/allocator.h"
#include "memory/fixedallocator.h"
#include "memory/gpallocator.h"
#include "utility.h"

static unsigned char backing[512 * 1024 * 1024];

static void Allocator_GPALargeAllocFree(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_GPAllocator gpa;
  cu_GPAllocator_Config cfg = {0};
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_GPAllocator(&gpa, cfg);

  const size_t big = 100 * 1024 * 1024; // 100 MiB
  cu_Slice_Result mem_res =
      cu_Allocator_Alloc(alloc, cu_Layout_create(big, 16));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&mem_res));
  cu_Slice mem = mem_res.value;
  cu_Memory_memset(mem.ptr, 0xCD, mem.length);
  cu_Allocator_Free(alloc, mem);

  const size_t small = 4096; // 4 KiB blocks
  const size_t total = 100 * 1024 * 1024;
  const size_t count = total / small;
  cu_Slice *blocks = malloc(sizeof(cu_Slice) * count);
  for (size_t i = 0; i < count; ++i) {
    cu_Slice_Result s_res = cu_Allocator_Alloc(alloc, cu_Layout_create(small, 16));
    TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&s_res));
    blocks[i] = s_res.value;
    cu_Memory_memset(blocks[i].ptr, 0xEF, blocks[i].length);
  }
  for (size_t i = 0; i < count; ++i) {
    cu_Allocator_Free(alloc, blocks[i]);
  }
  free(blocks);

  cu_GPAllocator_destroy(&gpa);
}

typedef struct {
  int x;
  int y;
} cu_Point;

static void Allocator_NormalAllocAndFree(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_GPAllocator gpa;
  cu_GPAllocator_Config cfg = {0};
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_GPAllocator(&gpa, cfg);

  cu_Slice_Result res =
      cu_Allocator_Alloc(alloc,
          cu_Layout_create(sizeof(cu_Point), alignof(cu_Point)));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&res));

  cu_Slice mem = res.value;
  TEST_ASSERT_TRUE(mem.ptr != NULL);
  TEST_ASSERT_EQUAL(mem.length, sizeof(cu_Point));

  cu_Point *point = (cu_Point *)mem.ptr;
  point->x = 42;
  point->y = 84;

  TEST_ASSERT_EQUAL(point->x, 42);
  TEST_ASSERT_EQUAL(point->y, 84);

  cu_Allocator_Free(alloc, mem);
  cu_GPAllocator_destroy(&gpa);
}

static void Allocator_DoubleFree(void) {
  cu_FixedAllocator fa;
  cu_Allocator fa_alloc = cu_Allocator_FixedAllocator(
      &fa, cu_Slice_create(backing, sizeof(backing)));

  cu_GPAllocator gpa;
  cu_GPAllocator_Config cfg = {0};
  cfg.backingAllocator = cu_Allocator_Optional_some(fa_alloc);
  cu_Allocator alloc = cu_Allocator_GPAllocator(&gpa, cfg);

  cu_Slice_Result res =
      cu_Allocator_Alloc(alloc,
          cu_Layout_create(sizeof(cu_Point), alignof(cu_Point)));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&res));

  cu_Slice mem = res.value;
  TEST_ASSERT_TRUE(mem.ptr != NULL);

  // Free the memory
  cu_Allocator_Free(alloc, mem);

  // Attempt to free it again (should not crash)
  cu_Allocator_Free(alloc, mem);

  cu_GPAllocator_destroy(&gpa);
}

static void Allocator_Exhaustion(void) {
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
      cu_Allocator_Alloc(alloc,
          cu_Layout_create(large_size, alignof(cu_Point)));

  TEST_ASSERT_FALSE(cu_Slice_Result_is_ok(&res));
  TEST_ASSERT_EQUAL(
      cu_Slice_Result_unwrap_error(&res).kind, CU_IO_ERROR_KIND_OUT_OF_MEMORY);
}
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(Allocator_GPALargeAllocFree);
    RUN_TEST(Allocator_NormalAllocAndFree);
    RUN_TEST(Allocator_DoubleFree);
    RUN_TEST(Allocator_Exhaustion);
    return UNITY_END();
}
