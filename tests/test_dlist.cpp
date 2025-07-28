#if CU_FREESTANDING
#include <gtest/gtest.h>
TEST(DList, Unsupported) { SUCCEED(); }
#else
extern "C" {
#include "collection/dlist.h"
#include "memory/allocator.h"
#include "memory/fixedallocator.h"
#include "memory/gpallocator.h"
#include "memory/page.h"
}
#include <gtest/gtest.h>

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
  cu_GPAllocator_Config cfg = {};
  cfg.bucketSize = CU_GPA_BUCKET_SIZE;
  cfg.backingAllocator = cu_Allocator_Optional_some(backing);
  return cu_Allocator_GPAllocator(gpa, cfg);
}

TEST(DList, PushPop) {
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_DList_Result res = cu_DList_create(alloc, CU_LAYOUT(int));
  ASSERT_TRUE(cu_DList_Result_is_ok(&res));
  cu_DList list = cu_DList_Result_unwrap(&res);

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
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_DList_Result res = cu_DList_create(alloc, CU_LAYOUT(int));
  ASSERT_TRUE(cu_DList_Result_is_ok(&res));
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
    ASSERT_LT(idx, CU_ARRAY_LEN(expected));
    EXPECT_EQ(*(int *)elem, expected[idx++]);
  }
  EXPECT_EQ(idx, CU_ARRAY_LEN(expected));

  cu_DList_destroy(&list);
  cu_GPAllocator_destroy(&gpa);
}
#endif
