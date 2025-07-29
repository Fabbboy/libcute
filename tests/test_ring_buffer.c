#include "collection/ring_buffer.h"
#include "memory/allocator.h"
#include "test_common.h"
#include "unity.h"
#include <unity_internals.h>

static void RingBuffer_PushPop(void) {
  cu_Allocator alloc = test_allocator;

  cu_RingBuffer_Result res =
      cu_RingBuffer_create(alloc, CU_LAYOUT(int), 4,
          cu_Destructor_Optional_none());
  TEST_ASSERT_TRUE(cu_RingBuffer_Result_is_ok(&res));
  cu_RingBuffer rb = cu_RingBuffer_Result_unwrap(&res);

  for (int i = 0; i < 4; ++i) {
    int v = i;
    cu_RingBuffer_Error_Optional err = cu_RingBuffer_push(&rb, &v);
    TEST_ASSERT_TRUE(cu_RingBuffer_Error_Optional_is_none(&err));
  }
  TEST_ASSERT_TRUE(cu_RingBuffer_is_full(&rb));

  for (int i = 0; i < 4; ++i) {
    int out = 0;
    cu_RingBuffer_Error_Optional err = cu_RingBuffer_pop(&rb, &out);
    TEST_ASSERT_TRUE(cu_RingBuffer_Error_Optional_is_none(&err));
    TEST_ASSERT_EQUAL(out, i);
  }
  TEST_ASSERT_TRUE(cu_RingBuffer_is_empty(&rb));

  cu_RingBuffer_destroy(&rb);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(RingBuffer_PushPop);
  return UNITY_END();
}
