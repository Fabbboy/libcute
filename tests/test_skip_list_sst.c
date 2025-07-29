#if CU_FREESTANDING
#include "unity.h"
#include <unity_internals.h>
static void SkipListSST_Unsupported(void) {}
#else
#include "collection/skip_list.h"
#include "memory/allocator.h"
#include "test_common.h"
#include "nostd.h"
#include "unity.h"
#include <unity_internals.h>


static int cstring_cmp(const void *a, const void *b) {
  const char *sa = *(const char *const *)a;
  const char *sb = *(const char *const *)b;
  return cu_CString_cmp(sa, sb);
}

static void SkipListSST_SortedStrings(void) {
  cu_Allocator alloc = test_allocator;
  cu_RandomState rng;
  cu_State st = cu_RandomState_init(&rng, 1);

  cu_SkipList_Result res = cu_SkipList_create(alloc, CU_LAYOUT(const char *),
      CU_LAYOUT(const char *), 6, cu_SkipList_CmpFn_Optional_some(cstring_cmp),
      cu_Destructor_Optional_none(), cu_Destructor_Optional_none(), st);
  TEST_ASSERT_TRUE(cu_SkipList_Result_is_ok(&res));
  cu_SkipList list = cu_SkipList_Result_unwrap(&res);

  const char *keys[] = {"cherry", "apple", "date", "banana", "elderberry"};
  const char *values[] = {"C", "A", "D", "B", "E"};
  const char *sorted[] = {"apple", "banana", "cherry", "date", "elderberry"};

  for (int i = 0; i < 5; ++i) {
    cu_SkipList_Error_Optional err =
        cu_SkipList_insert(&list, &keys[i], &values[i]);
    TEST_ASSERT_TRUE(cu_SkipList_Error_Optional_is_none(&err));
  }

  struct cu_SkipList_Node *n = NULL;
  void *k;
  void *v;
  int idx = 0;
  while (cu_SkipList_iter(&list, &n, &k, &v)) {
    const char *kk = *(const char **)k;
    const char *vv = *(const char **)v;
    TEST_ASSERT_TRUE((idx) < (5));
    TEST_ASSERT_EQUAL_STRING(kk, sorted[idx]);
    if (cu_CString_cmp(kk, "apple") == 0) {
      TEST_ASSERT_EQUAL_STRING(vv, "A");
    }
    if (cu_CString_cmp(kk, "banana") == 0) {
      TEST_ASSERT_EQUAL_STRING(vv, "B");
    }
    if (cu_CString_cmp(kk, "cherry") == 0) {
      TEST_ASSERT_EQUAL_STRING(vv, "C");
    }
    if (cu_CString_cmp(kk, "date") == 0) {
      TEST_ASSERT_EQUAL_STRING(vv, "D");
    }
    if (cu_CString_cmp(kk, "elderberry") == 0) {
      TEST_ASSERT_EQUAL_STRING(vv, "E");
    }
    ++idx;
  }
  TEST_ASSERT_EQUAL(idx, 5);

  cu_SkipList_destroy(&list);
}
#endif

int main(void) {
  UNITY_BEGIN();
#if CU_FREESTANDING
  RUN_TEST(SkipListSST_Unsupported);
#else
  RUN_TEST(SkipListSST_SortedStrings);
#endif
  return UNITY_END();
}
