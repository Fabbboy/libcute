extern "C" {
#include "collection/ring_buffer.h"
#include "memory/allocator.h"
#include "memory/gpallocator.h"
#include "memory/page.h"
}
#include <gtest/gtest.h>

static cu_Allocator create_allocator(
    cu_GPAllocator *gpa, cu_PageAllocator *page) {
  cu_Allocator page_alloc = cu_Allocator_PageAllocator(page);
  cu_GPAllocator_Config cfg = {};
  cfg.bucketSize = CU_GPA_BUCKET_SIZE;
  cfg.backingAllocator = cu_Allocator_Optional_some(page_alloc);
  return cu_Allocator_GPAllocator(gpa, cfg);
}

TEST(RingBuffer, PushPop) {
  cu_PageAllocator page;
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa, &page);

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
