#include <gtest/gtest.h>
#include <string.h>
extern "C" {
#include "memory/fixedallocator.h"
#include "nostd.h"
}

TEST(FixedAllocator, Basic) {
  unsigned char buf[64];
  cu_FixedAllocator fa;
  cu_Slice buf_slice = cu_Slice_create(buf, sizeof(buf));
  cu_Allocator alloc = cu_Allocator_FixedAllocator(&fa, buf_slice);

  cu_Slice_Result a_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&a_res));
  cu_Slice a = a_res.value;
  memset(a.ptr, 0xAA, a.length);

  cu_Allocator_Free(alloc, a);
  cu_Slice_Result b_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&b_res));
  EXPECT_EQ(b_res.value.ptr, a.ptr);
}

TEST(FixedAllocator, Exhaustion) {
  unsigned char buf[32];
  cu_FixedAllocator fa;
  cu_Slice buf_slice = cu_Slice_create(buf, sizeof(buf));
  cu_Allocator alloc = cu_Allocator_FixedAllocator(&fa, buf_slice);

  cu_Slice_Result a_res = cu_Allocator_Alloc(alloc, 24, 8);
  ASSERT_TRUE(cu_Slice_result_is_ok(&a_res));
  cu_Slice_Result b_res = cu_Allocator_Alloc(alloc, 16, 8);
  ASSERT_FALSE(cu_Slice_result_is_ok(&b_res));
}
