#if CU_FREESTANDING
#include "unity.h"
#include <unity_internals.h>
static void SkipList_Unsupported(void) {}
#else
#include "collection/skip_list.h"
#include "memory/allocator.h"
#include "memory/fixedallocator.h"
#include "memory/gpallocator.h"
#include "memory/page.h"
#include "unity.h"
#include <unity_internals.h>

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

static int int_cmp(const void *a, const void *b) {
  int ia = *(const int *)a;
  int ib = *(const int *)b;
  return (ia > ib) - (ia < ib);
}

static void SkipList_InsertFindRemove(void) {
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_SkipList_Result res = cu_SkipList_create(alloc, CU_LAYOUT(int),
      CU_LAYOUT(int), 8, cu_SkipList_CmpFn_Optional_some(int_cmp));
  TEST_ASSERT_TRUE(cu_SkipList_Result_is_ok(&res));
  cu_SkipList list = cu_SkipList_Result_unwrap(&res);

  for (int i = 0; i < 10; ++i) {
    cu_SkipList_Error_Optional err = cu_SkipList_insert(&list, &i, &i);
    TEST_ASSERT_TRUE(cu_SkipList_Error_Optional_is_none(&err));
  }

  for (int i = 0; i < 10; ++i) {
    Ptr_Optional opt = cu_SkipList_find(&list, &i);
    TEST_ASSERT_TRUE(Ptr_Optional_is_some(&opt));
    int *v = (int *)Ptr_Optional_unwrap(&opt);
    TEST_ASSERT_EQUAL(*v, i);
  }

  for (int i = 0; i < 10; ++i) {
    cu_SkipList_Error_Optional err = cu_SkipList_remove(&list, &i);
    TEST_ASSERT_TRUE(cu_SkipList_Error_Optional_is_none(&err));
  }
  int zero = 0;
  TEST_ASSERT_FALSE(cu_SkipList_find(&list, &zero).isSome);

  cu_SkipList_destroy(&list);
  cu_GPAllocator_destroy(&gpa);
}

static void SkipList_Iteration(void) {
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_SkipList_Result res = cu_SkipList_create(alloc, CU_LAYOUT(int),
      CU_LAYOUT(int), 6, cu_SkipList_CmpFn_Optional_some(int_cmp));
  TEST_ASSERT_TRUE(cu_SkipList_Result_is_ok(&res));
  cu_SkipList list = cu_SkipList_Result_unwrap(&res);

  int sum = 0;
  for (int i = 0; i < 5; ++i) {
    cu_SkipList_insert(&list, &i, &i);
    sum += i;
  }

  cu_SkipList_Node *n = NULL;
  void *k;
  void *v;
  int iter_sum = 0;
  while (cu_SkipList_iter(&list, &n, &k, &v)) {
    iter_sum += *(int *)v;
  }
  TEST_ASSERT_EQUAL(iter_sum, sum);

  cu_SkipList_destroy(&list);
  cu_GPAllocator_destroy(&gpa);
}
#endif

int main(void) {
  UNITY_BEGIN();
#if CU_FREESTANDING
  RUN_TEST(SkipList_Unsupported);
#else
  RUN_TEST(SkipList_InsertFindRemove);
  RUN_TEST(SkipList_Iteration);
#endif
  return UNITY_END();
}
