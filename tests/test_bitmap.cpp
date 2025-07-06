#include <gtest/gtest.h>
extern "C" {
#include "collection/bitmap.h"
#include "memory/allocator.h"
}

TEST(Bitmap, Basic) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  cu_Bitmap_Optional opt = cu_Bitmap_create(alloc, 128);
  ASSERT_TRUE(cu_Bitmap_is_some(&opt));
  cu_Bitmap map = opt.value;

  EXPECT_EQ(cu_Bitmap_size(&map), 128u);
  EXPECT_FALSE(cu_Bitmap_get(&map, 5));
  cu_Bitmap_set(&map, 5);
  EXPECT_TRUE(cu_Bitmap_get(&map, 5));
  cu_Bitmap_clear(&map, 5);
  EXPECT_FALSE(cu_Bitmap_get(&map, 5));

  cu_Bitmap_destroy(&map);
}
