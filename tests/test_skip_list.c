#if CU_FREESTANDING
#include "unity.h"
#include <unity_internals.h>
static void SkipList_Unsupported(void) {}
#else
#include "collection/skip_list.h"
#include "memory/allocator.h"
#include "test_common.h"
#include "unity.h"
#include <unity_internals.h>


static int int_cmp(const void *a, const void *b) {
  int ia = *(const int *)a;
  int ib = *(const int *)b;
  return (ia > ib) - (ia < ib);
}

static void SkipList_InsertFindRemove(void) {
  cu_Allocator alloc = test_allocator;

  cu_SkipList_Result res = cu_SkipList_create(alloc, CU_LAYOUT(int),
      CU_LAYOUT(int), 8, cu_SkipList_CmpFn_Optional_some(int_cmp),
      cu_Destructor_Optional_none(), cu_Destructor_Optional_none());
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
}

static void SkipList_Iteration(void) {
  cu_Allocator alloc = test_allocator;

  cu_SkipList_Result res = cu_SkipList_create(alloc, CU_LAYOUT(int),
      CU_LAYOUT(int), 6, cu_SkipList_CmpFn_Optional_some(int_cmp),
      cu_Destructor_Optional_none(), cu_Destructor_Optional_none());
  TEST_ASSERT_TRUE(cu_SkipList_Result_is_ok(&res));
  cu_SkipList list = cu_SkipList_Result_unwrap(&res);

  int sum = 0;
  for (int i = 0; i < 5; ++i) {
    cu_SkipList_insert(&list, &i, &i);
    sum += i;
  }

  struct cu_SkipList_Node *n = NULL;
  void *k;
  void *v;
  int iter_sum = 0;
  while (cu_SkipList_iter(&list, &n, &k, &v)) {
    iter_sum += *(int *)v;
  }
  TEST_ASSERT_EQUAL(iter_sum, sum);

  cu_SkipList_destroy(&list);
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
