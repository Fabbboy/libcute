#include "memory/fixedallocator.h"
#include "nostd.h"
#include "unity.h"
#include <nostd.h>
#include <unity_internals.h>

static void FixedAllocator_Basic(void) {
  unsigned char buf[64];
  cu_FixedAllocator fa;
  cu_Slice buf_slice = cu_Slice_create(buf, sizeof(buf));
  cu_Allocator alloc = cu_Allocator_FixedAllocator(&fa, buf_slice);

  cu_Slice_Result a_res = cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&a_res));
  cu_Slice a = a_res.value;
  cu_Memory_memset(a.ptr, 0xAA, a.length);

  cu_Allocator_Free(alloc, a);
  cu_Slice_Result b_res = cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&b_res));
  TEST_ASSERT_EQUAL(b_res.value.ptr, a.ptr);
}

static void FixedAllocator_Exhaustion(void) {
  unsigned char buf[32];
  cu_FixedAllocator fa;
  cu_Slice buf_slice = cu_Slice_create(buf, sizeof(buf));
  cu_Allocator alloc = cu_Allocator_FixedAllocator(&fa, buf_slice);

  cu_Slice_Result a_res = cu_Allocator_Alloc(alloc, cu_Layout_create(24, 8));
  TEST_ASSERT_TRUE(cu_Slice_Result_is_ok(&a_res));
  cu_Slice_Result b_res = cu_Allocator_Alloc(alloc, cu_Layout_create(16, 8));
  TEST_ASSERT_FALSE(cu_Slice_Result_is_ok(&b_res));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(FixedAllocator_Basic);
  RUN_TEST(FixedAllocator_Exhaustion);
  return UNITY_END();
}
