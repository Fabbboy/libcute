extern "C" {
#include "collection/ring_buffer.h"
#include "memory/allocator.h"
#include "memory/fixedallocator.h"
#include "memory/gpallocator.h"
}
#include <gtest/gtest.h>

static cu_Allocator create_allocator(cu_GPAllocator *gpa) {
#if CU_FREESTANDING
  static char buf[32 * 1024];
  static cu_FixedAllocator fa;
  cu_Allocator backing =
      cu_Allocator_FixedAllocator(&fa, cu_Slice_create(buf, sizeof(buf)));
#else
  static cu_PageAllocator page;
  cu_Allocator backing = cu_Allocator_PageAllocator(&page);
#endif
  cu_GPAllocator_Config cfg = {};
  cfg.bucketSize = CU_GPA_BUCKET_SIZE;
  cfg.backingAllocator = cu_Allocator_Optional_some(backing);
  return cu_Allocator_GPAllocator(gpa, cfg);
}

TEST(RingBuffer, PushPop) {
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_RingBuffer_Result res = cu_RingBuffer_create(alloc, CU_LAYOUT(int), 4);
  ASSERT_TRUE(cu_RingBuffer_result_is_ok(&res));
  cu_RingBuffer rb = cu_RingBuffer_result_unwrap(&res);

  for (int i = 0; i < 4; ++i) {
    int v = i;
    cu_RingBuffer_Error_Optional err = cu_RingBuffer_push(&rb, &v);
    ASSERT_TRUE(cu_RingBuffer_Error_Optional_is_none(&err));
  }
  ASSERT_TRUE(cu_RingBuffer_is_full(&rb));

  for (int i = 0; i < 4; ++i) {
    int out = 0;
    cu_RingBuffer_Error_Optional err = cu_RingBuffer_pop(&rb, &out);
    ASSERT_TRUE(cu_RingBuffer_Error_Optional_is_none(&err));
    EXPECT_EQ(out, i);
  }
  EXPECT_TRUE(cu_RingBuffer_is_empty(&rb));

  cu_RingBuffer_destroy(&rb);
  cu_GPAllocator_destroy(&gpa);
}
