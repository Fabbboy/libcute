#if CU_FREESTANDING
#include "test_common.h"
static void List_Unsupported(void) { }
#else
#include "collection/list.h"
#include "memory/allocator.h"
#include "memory/fixedallocator.h"
#include "memory/gpallocator.h"
#include "memory/page.h"
#include "test_common.h"

static cu_Allocator create_allocator(cu_GPAllocator *gpa) {
#if CU_FREESTANDING
  static char buf[32 * 1024];
  cu_FixedAllocator fa;
  cu_Allocator backing =
      cu_Allocator_FixedAllocator(&fa, cu_Slice_create(buf, sizeof(buf)));
#else
  cu_PageAllocator page;
  cu_Allocator backing = cu_Allocator_PageAllocator(&page);
#endif
  cu_GPAllocator_Config cfg = {0};
  cfg.bucketSize = CU_GPA_BUCKET_SIZE;
  cfg.backingAllocator = cu_Allocator_Optional_some(backing);
  return cu_Allocator_GPAllocator(gpa, cfg);
}

static void List_PushPop(void) {
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_List_Result res = cu_List_create(alloc, CU_LAYOUT(int));
  TEST_ASSERT_TRUE(cu_List_Result_is_ok(&res));
  cu_List list = cu_List_Result_unwrap(&res);

  for (int i = 0; i < 4; ++i) {
    cu_List_Error_Optional err = cu_List_push_front(&list, &i);
    TEST_ASSERT_TRUE(cu_List_Error_Optional_is_none(&err));
  }
  TEST_ASSERT_EQUAL(cu_List_size(&list), 4u);

  for (int i = 3; i >= 0; --i) {
    int out = -1;
    cu_List_Error_Optional err = cu_List_pop_front(&list, &out);
    TEST_ASSERT_TRUE(cu_List_Error_Optional_is_none(&err));
    TEST_ASSERT_EQUAL(out, i);
  }
  TEST_ASSERT_TRUE(cu_List_is_empty(&list));

  cu_List_destroy(&list);
  cu_GPAllocator_destroy(&gpa);
}

static void List_InsertIter(void) {
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_List_Result r = cu_List_create(alloc, CU_LAYOUT(int));
  TEST_ASSERT_TRUE(cu_List_Result_is_ok(&r));
  cu_List list = cu_List_Result_unwrap(&r);

  for (int i = 0; i < 3; ++i) {
    cu_List_push_front(&list, &i); // 2,1,0
  }

  cu_List_Node *node = list.head;
  int val = 42;
  cu_List_Error_Optional err = cu_List_insert_after(&list, node, &val);
  TEST_ASSERT_TRUE(cu_List_Error_Optional_is_none(&err)); // list:2,42,1,0

  node = list.head; // head 2
  while (node->next->next) node = node->next; // node at 1
  val = 99;
  err = cu_List_insert_before(&list, node->next, &val); // before 0
  TEST_ASSERT_TRUE(cu_List_Error_Optional_is_none(&err));

  int expected[] = {2, 42, 1, 99, 0};
  size_t idx = 0;
  cu_List_Node *it = NULL;
  void *elem = NULL;
  while (cu_List_iter(&list, &it, &elem)) {
    TEST_ASSERT_TRUE((idx) < (CU_ARRAY_LEN(expected)));
    TEST_ASSERT_EQUAL(*(int *)elem, expected[idx++]);
  }
  TEST_ASSERT_EQUAL(idx, CU_ARRAY_LEN(expected));

  cu_List_destroy(&list);
  cu_GPAllocator_destroy(&gpa);
}
#endif

int main(void) {
    UNITY_BEGIN();
#if CU_FREESTANDING
    RUN_TEST(List_Unsupported);
#else
    RUN_TEST(List_PushPop);
    RUN_TEST(List_InsertIter);
#endif
    return UNITY_END();
}
