extern "C" {
#include "collection/hashmap.h"
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

TEST(HashMap, BasicInsertGet) {
  cu_PageAllocator page;
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa, &page);

  cu_HashMap_Result res = cu_HashMap_create(alloc, CU_LAYOUT(int),
      CU_LAYOUT(int), Size_Optional_some(8), cu_HashMap_HashFn_Optional_none(),
      cu_HashMap_EqualsFn_Optional_none());
  ASSERT_TRUE(cu_HashMap_result_is_ok(&res));
  cu_HashMap map = cu_HashMap_result_unwrap(&res);

  for (int i = 0; i < 10; ++i) {
    cu_HashMap_insert(&map, &i, &i);
  }

  for (int i = 0; i < 10; ++i) {
    Ptr_Optional opt = cu_HashMap_get(&map, &i);
    ASSERT_TRUE(Ptr_Optional_is_some(&opt));
    int *val = (int *)Ptr_Optional_unwrap(&opt);
    EXPECT_EQ(*val, i);
  }

  cu_HashMap_destroy(&map);
  cu_GPAllocator_destroy(&gpa);
}

static uint64_t int_hash(const void *key, size_t) {
  return (uint64_t)*(const int *)key;
}

static bool int_eq(const void *a, const void *b, size_t) {
  return *(const int *)a == *(const int *)b;
}

TEST(HashMap, CustomHashIter) {
  cu_PageAllocator page;
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa, &page);

  cu_HashMap_Result res =
      cu_HashMap_create(alloc, CU_LAYOUT(int), CU_LAYOUT(int),
          Size_Optional_some(4), cu_HashMap_HashFn_Optional_some(int_hash),
          cu_HashMap_EqualsFn_Optional_some(int_eq));
  ASSERT_TRUE(cu_HashMap_result_is_ok(&res));
  cu_HashMap map = cu_HashMap_result_unwrap(&res);

  for (int i = 0; i < 5; ++i) {
    cu_HashMap_insert(&map, &i, &i);
  }

  size_t idx = 0;
  void *k;
  void *v;
  int sum = 0;
  while (cu_HashMap_iter(&map, &idx, &k, &v)) {
    EXPECT_EQ(*(int *)k, *(int *)v);
    sum += *(int *)k;
  }
  EXPECT_EQ(sum, 10);

  cu_HashMap_destroy(&map);
  cu_GPAllocator_destroy(&gpa);
}
