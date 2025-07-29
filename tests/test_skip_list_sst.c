#if CU_FREESTANDING
#include "unity.h"
#include <unity_internals.h>
static void SkipListSST_Unsupported(void) {}
#else
#include "collection/skip_list.h"
#include "memory/allocator.h"
#include "memory/fixedallocator.h"
#include "memory/gpallocator.h"
#include "memory/page.h"
#include "nostd.h"
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

static int cstring_cmp(const void *a, const void *b) {
  const char *sa = *(const char *const *)a;
  const char *sb = *(const char *const *)b;
  return cu_CString_cmp(sa, sb);
}

static void SkipListSST_SortedStrings(void) {
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_SkipList_Result res = cu_SkipList_create(alloc, CU_LAYOUT(const char *),
      CU_LAYOUT(const char *), 6, cu_SkipList_CmpFn_Optional_some(cstring_cmp));
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

  cu_SkipList_Node *n = NULL;
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
  cu_GPAllocator_destroy(&gpa);
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
