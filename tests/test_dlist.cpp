extern "C" {
#include "collection/dlist.h"
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

TEST(DList, PushPop) {
  cu_PageAllocator page;
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa, &page);

  cu_DList_Result res = cu_DList_create(alloc, CU_LAYOUT(int));
  ASSERT_TRUE(cu_DList_result_is_ok(&res));
  cu_DList list = cu_DList_result_unwrap(&res);

  for (int i = 0; i < 3; ++i) {
    cu_DList_Error_Optional err = cu_DList_push_back(&list, &i);
    ASSERT_TRUE(cu_DList_Error_Optional_is_none(&err));
  }
  EXPECT_EQ(cu_DList_size(&list), 3u);

  int out = -1;
  cu_DList_Error_Optional err = cu_DList_pop_front(&list, &out);
  ASSERT_TRUE(cu_DList_Error_Optional_is_none(&err));
  EXPECT_EQ(out, 0);

  err = cu_DList_pop_back(&list, &out);
  ASSERT_TRUE(cu_DList_Error_Optional_is_none(&err));
  EXPECT_EQ(out, 2);

  cu_DList_destroy(&list);
  cu_GPAllocator_destroy(&gpa);
}

TEST(DList, InsertIter) {
  cu_PageAllocator page;
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa, &page);

  cu_DList_Result res = cu_DList_create(alloc, CU_LAYOUT(int));
  ASSERT_TRUE(cu_DList_result_is_ok(&res));
  cu_DList list = cu_DList_result_unwrap(&res);

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
    ASSERT_LT(idx, CU_ARRAY_LEN(expected));
    EXPECT_EQ(*(int *)elem, expected[idx++]);
  }
  EXPECT_EQ(idx, CU_ARRAY_LEN(expected));

  cu_DList_destroy(&list);
  cu_GPAllocator_destroy(&gpa);
}
