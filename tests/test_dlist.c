#if CU_FREESTANDING
#include "unity.h"
#include <unity_internals.h>
static void DList_Unsupported(void) {}
#else
#include "collection/dlist.h"
#include "memory/allocator.h"
#include "memory/fixedallocator.h"
#include "memory/gpallocator.h"
#include "memory/page.h"
#include "unity.h"
#include <unity_internals.h>

static cu_Allocator create_allocator(cu_GPAllocator *gpa) {
  (void)gpa;
  return cu_Allocator_CAllocator();
}

static void DList_PushPop(void) {
  cu_Allocator alloc = create_allocator(NULL);

  cu_DList_Result res = cu_DList_create(alloc, CU_LAYOUT(int));
  TEST_ASSERT_TRUE(cu_DList_Result_is_ok(&res));
  cu_DList list = cu_DList_Result_unwrap(&res);

  for (int i = 0; i < 3; ++i) {
    cu_DList_Error_Optional err = cu_DList_push_back(&list, &i);
    TEST_ASSERT_TRUE(cu_DList_Error_Optional_is_none(&err));
  }
  TEST_ASSERT_EQUAL(cu_DList_size(&list), 3u);

  int out = -1;
  cu_DList_Error_Optional err = cu_DList_pop_front(&list, &out);
  TEST_ASSERT_TRUE(cu_DList_Error_Optional_is_none(&err));
  TEST_ASSERT_EQUAL(out, 0);

  err = cu_DList_pop_back(&list, &out);
  TEST_ASSERT_TRUE(cu_DList_Error_Optional_is_none(&err));
  TEST_ASSERT_EQUAL(out, 2);

  cu_DList_destroy(&list);
}

static void DList_InsertIter(void) {
  cu_Allocator alloc = create_allocator(NULL);

  cu_DList_Result res = cu_DList_create(alloc, CU_LAYOUT(int));
  TEST_ASSERT_TRUE(cu_DList_Result_is_ok(&res));
  cu_DList list = cu_DList_Result_unwrap(&res);

  for (int i = 0; i < 2; ++i) {
    cu_DList_push_back(&list, &i); // 0,1
  }

  cu_DList_Node *node = list.head;
  int v = 42;
  cu_DList_insert_after(&list, node, &v); // 0,42,1

  v = 99;
  cu_DList_insert_before(&list, list.head, &v); // 99,0,42,1

  int expected[] = {99, 0, 42, 1};
  size_t idx = 0;
  cu_DList_Node *it = NULL;
  void *elem = NULL;
  while (cu_DList_iter(&list, &it, &elem)) {
    TEST_ASSERT_TRUE((idx) < (CU_ARRAY_LEN(expected)));
    TEST_ASSERT_EQUAL(*(int *)elem, expected[idx++]);
  }
  TEST_ASSERT_EQUAL(idx, CU_ARRAY_LEN(expected));

  cu_DList_destroy(&list);
}
#endif

int main(void) {
  UNITY_BEGIN();
#if CU_FREESTANDING
  RUN_TEST(DList_Unsupported);
#else
  RUN_TEST(DList_PushPop);
  RUN_TEST(DList_InsertIter);
#endif
  return UNITY_END();
}
