#if CU_FREESTANDING
#include "unity.h"
#include <unity_internals.h>
static void HashMap_Unsupported(void) {}
#else
#include "collection/hashmap.h"
#include "memory/allocator.h"
#include "test_common.h"
#include "unity.h"
#include <stdlib.h>
#include <unity_internals.h>

static void HashMap_BasicInsertGet(void) {
  cu_Allocator alloc = test_allocator;

  cu_HashMap_Result res = cu_HashMap_create(alloc, CU_LAYOUT(int),
      CU_LAYOUT(int), Size_Optional_some(8), cu_HashMap_HashFn_Optional_none(),
      cu_HashMap_EqualsFn_Optional_none());
  TEST_ASSERT_TRUE(cu_HashMap_Result_is_ok(&res));
  cu_HashMap map = cu_HashMap_Result_unwrap(&res);

  for (int i = 0; i < 10; ++i) {
    cu_HashMap_insert(&map, &i, &i);
  }

  for (int i = 0; i < 10; ++i) {
    Ptr_Optional opt = cu_HashMap_get(&map, &i);
    TEST_ASSERT_TRUE(Ptr_Optional_is_some(&opt));
    int *val = (int *)Ptr_Optional_unwrap(&opt);
    TEST_ASSERT_EQUAL(*val, i);
  }

  cu_HashMap_destroy(&map);
}

static uint64_t int_hash(const void *key, size_t unused) {
  return (uint64_t)*(const int *)key;
}

static bool int_eq(const void *a, const void *b, size_t unused) {
  return *(const int *)a == *(const int *)b;
}

static void HashMap_CustomHashIter(void) {
  cu_Allocator alloc = test_allocator;

  cu_HashMap_Result res =
      cu_HashMap_create(alloc, CU_LAYOUT(int), CU_LAYOUT(int),
          Size_Optional_some(4), cu_HashMap_HashFn_Optional_some(int_hash),
          cu_HashMap_EqualsFn_Optional_some(int_eq));
  TEST_ASSERT_TRUE(cu_HashMap_Result_is_ok(&res));
  cu_HashMap map = cu_HashMap_Result_unwrap(&res);

  for (int i = 0; i < 5; ++i) {
    cu_HashMap_insert(&map, &i, &i);
  }

  size_t idx = 0;
  void *k;
  void *v;
  int sum = 0;
  while (cu_HashMap_iter(&map, &idx, &k, &v)) {
    TEST_ASSERT_EQUAL(*(int *)k, *(int *)v);
    sum += *(int *)k;
  }
  TEST_ASSERT_EQUAL(sum, 10);

  cu_HashMap_destroy(&map);
}

static void HashMap_StressRandomAccess(void) {
  cu_Allocator alloc = test_allocator;

  cu_HashMap_Result res = cu_HashMap_create(alloc, CU_LAYOUT(int),
      CU_LAYOUT(int), Size_Optional_some(128),
      cu_HashMap_HashFn_Optional_none(), cu_HashMap_EqualsFn_Optional_none());
  TEST_ASSERT_TRUE(cu_HashMap_Result_is_ok(&res));
  cu_HashMap map = cu_HashMap_Result_unwrap(&res);

  const int count = 250;
  for (int i = 0; i < count; ++i) {
    cu_HashMap_Error_Optional err = cu_HashMap_insert(&map, &i, &i);
    TEST_ASSERT_TRUE(cu_HashMap_Error_Optional_is_none(&err));
  }

  int *keys = malloc(sizeof(int) * count);
  for (int i = 0; i < count; ++i) {
    keys[i] = i;
  }
  srand(1234);
  for (int i = count - 1; i > 0; --i) {
    int j = rand() % (i + 1);
    int tmp = keys[i];
    keys[i] = keys[j];
    keys[j] = tmp;
  }

  for (int i = 0; i < count; ++i) {
    int k = keys[i];
    Ptr_Optional opt = cu_HashMap_get(&map, &k);
    TEST_ASSERT_TRUE(Ptr_Optional_is_some(&opt));
    int *val = (int *)Ptr_Optional_unwrap(&opt);
    TEST_ASSERT_EQUAL(*val, k);
  }
  free(keys);

  cu_HashMap_destroy(&map);
}
#endif

int main(void) {
  UNITY_BEGIN();
#if CU_FREESTANDING
  RUN_TEST(HashMap_Unsupported);
#else
  RUN_TEST(HashMap_BasicInsertGet);
  RUN_TEST(HashMap_CustomHashIter);
  RUN_TEST(HashMap_StressRandomAccess);
#endif
  return UNITY_END();
}
