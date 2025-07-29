#include "collection/bitmap.h"
#include "memory/allocator.h"
#include "test_common.h"
#include "unity.h"
#include <unity_internals.h>

static void Bitmap_Basic(void) {
  cu_Allocator alloc = test_allocator;
  cu_Bitmap_Optional opt = cu_Bitmap_create(alloc, 128);
  TEST_ASSERT_TRUE(cu_Bitmap_Optional_is_some(&opt));
  cu_Bitmap map = opt.value;

  TEST_ASSERT_EQUAL(cu_Bitmap_size(&map), 128u);
  TEST_ASSERT_FALSE(cu_Bitmap_get(&map, 5));
  cu_Bitmap_set(&map, 5);
  TEST_ASSERT_TRUE(cu_Bitmap_get(&map, 5));
  cu_Bitmap_clear(&map, 5);
  TEST_ASSERT_FALSE(cu_Bitmap_get(&map, 5));

  cu_Bitmap_destroy(&map);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(Bitmap_Basic);
  return UNITY_END();
}
