#include "object/optional.h"
#if CU_FREESTANDING
#include "unity.h"
#include <unity_internals.h>
static void Vector_Unsupported(void) {}
#else
#include "collection/vector.h"
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

static void Vector_Create(void) {
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_Layout layout = CU_LAYOUT(int);
  cu_Vector_Result res =
      cu_Vector_create(alloc, layout, Size_Optional_some(10));
  TEST_ASSERT_TRUE(cu_Vector_Result_is_ok(&res));
  cu_Vector vec = cu_Vector_Result_unwrap(&res);
  TEST_ASSERT_EQUAL(vec.length, 0u);
  TEST_ASSERT_EQUAL(vec.capacity, 10u);

  cu_Vector_destroy(&vec);
  cu_GPAllocator_destroy(&gpa);
}

static void Vector_Resize(void) {
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_Layout layout = CU_LAYOUT(int);
  cu_Vector_Result res =
      cu_Vector_create(alloc, layout, Size_Optional_some(10));
  TEST_ASSERT_TRUE(cu_Vector_Result_is_ok(&res));
  cu_Vector vector = cu_Vector_Result_unwrap(&res);

  cu_Vector_Error_Optional err = cu_Vector_resize(&vector, 20);
  TEST_ASSERT_TRUE(cu_Vector_Error_Optional_is_none(&err));
  TEST_ASSERT_EQUAL(cu_Vector_capacity(&vector), 20u);

  err = cu_Vector_resize(&vector, 5);
  TEST_ASSERT_TRUE(cu_Vector_Error_Optional_is_none(&err));
  TEST_ASSERT_EQUAL(cu_Vector_size(&vector), 5u);
  TEST_ASSERT_TRUE((cu_Vector_capacity(&vector)) >= (20u));

  cu_Vector_destroy(&vector);
  cu_GPAllocator_destroy(&gpa);
}

static void Vector_PushBack(void) {
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_Vector_Result res =
      cu_Vector_create(alloc, CU_LAYOUT(int), Size_Optional_some(0));
  TEST_ASSERT_TRUE(cu_Vector_Result_is_ok(&res));
  cu_Vector vector = cu_Vector_Result_unwrap(&res);

  int v = 42;
  cu_Vector_Error_Optional err = cu_Vector_push_back(&vector, &v);
  TEST_ASSERT_TRUE(cu_Vector_Error_Optional_is_none(&err));
  TEST_ASSERT_EQUAL(cu_Vector_size(&vector), 1u);
  TEST_ASSERT_EQUAL(*(int *)((unsigned char *)vector.data.value.ptr), 42);

  cu_Vector_destroy(&vector);
  cu_GPAllocator_destroy(&gpa);
}

static void Vector_PopBack(void) {
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_Vector_Result res =
      cu_Vector_create(alloc, CU_LAYOUT(int), Size_Optional_some(0));
  TEST_ASSERT_TRUE(cu_Vector_Result_is_ok(&res));
  cu_Vector vector = cu_Vector_Result_unwrap(&res);

  int a = 1, b = 2, out = 0;
  cu_Vector_push_back(&vector, &a);
  cu_Vector_push_back(&vector, &b);

  cu_Vector_Error_Optional err = cu_Vector_pop_back(&vector, &out);
  TEST_ASSERT_TRUE(cu_Vector_Error_Optional_is_none(&err));
  TEST_ASSERT_EQUAL(out, 2);
  TEST_ASSERT_EQUAL(cu_Vector_size(&vector), 1u);

  err = cu_Vector_pop_back(&vector, &out);
  TEST_ASSERT_TRUE(cu_Vector_Error_Optional_is_none(&err));
  TEST_ASSERT_EQUAL(out, 1);
  TEST_ASSERT_EQUAL(cu_Vector_size(&vector), 0u);
  TEST_ASSERT_EQUAL(cu_Vector_capacity(&vector), 0u);

  cu_Vector_destroy(&vector);
  cu_GPAllocator_destroy(&gpa);
}

static void Vector_Copy(void) {
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_Vector_Result res =
      cu_Vector_create(alloc, CU_LAYOUT(int), Size_Optional_some(5));
  TEST_ASSERT_TRUE(cu_Vector_Result_is_ok(&res));

  cu_Vector vector = cu_Vector_Result_unwrap(&res);

  for (int i = 0; i < 5; ++i) {
    cu_Vector_push_back(&vector, &i);
  }
  TEST_ASSERT_EQUAL(cu_Vector_size(&vector), 5u);
  TEST_ASSERT_EQUAL(cu_Vector_capacity(&vector), 5u);

  cu_Vector_Result copy_res = cu_Vector_copy(&vector);
  TEST_ASSERT_TRUE(cu_Vector_Result_is_ok(&copy_res));
  cu_Vector copy = cu_Vector_Result_unwrap(&copy_res);

  TEST_ASSERT_EQUAL(cu_Vector_size(&copy), 5u);
  TEST_ASSERT_EQUAL(cu_Vector_capacity(&copy), 5u);
  for (int i = 0; i < 5; ++i) {
    Ptr_Optional ptr = cu_Vector_at(&copy, i);
    TEST_ASSERT_TRUE(Ptr_Optional_is_some(&ptr));
    int *val = CU_AS(Ptr_Optional_unwrap(&ptr), int *);
    TEST_ASSERT_EQUAL(*val, i);
  }

  cu_Vector_destroy(&copy);
  cu_Vector_destroy(&vector);
  cu_GPAllocator_destroy(&gpa);
}

static void Vector_ReserveClearAt(void) {
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa);

  cu_Vector_Result res =
      cu_Vector_create(alloc, CU_LAYOUT(int), Size_Optional_some(0));
  TEST_ASSERT_TRUE(cu_Vector_Result_is_ok(&res));
  cu_Vector vector = cu_Vector_Result_unwrap(&res);

  cu_Vector_Error_Optional err = cu_Vector_reserve(&vector, 10);
  TEST_ASSERT_TRUE(cu_Vector_Error_Optional_is_none(&err));
  TEST_ASSERT_TRUE((cu_Vector_capacity(&vector)) >= (10u));

  for (int i = 0; i < 5; ++i) {
    cu_Vector_push_back(&vector, &i);
  }

  // int *val = CU_VECTOR_AT_AS(&vector, int, 2);
  Ptr_Optional ptr = cu_Vector_at(&vector, 2);
  TEST_ASSERT_TRUE(Ptr_Optional_is_some(&ptr));
  int *val = CU_AS(Ptr_Optional_unwrap(&ptr), int *);
  TEST_ASSERT_TRUE((val) != (NULL));
  TEST_ASSERT_EQUAL(*val, 2);

  cu_Vector_clear(&vector);
  TEST_ASSERT_EQUAL(cu_Vector_size(&vector), 0u);
  TEST_ASSERT_EQUAL(cu_Vector_capacity(&vector), 0u);

  cu_Vector_destroy(&vector);
  cu_GPAllocator_destroy(&gpa);
}
#endif

int main(void) {
  UNITY_BEGIN();
#if CU_FREESTANDING
  RUN_TEST(Vector_Unsupported);
#else
  RUN_TEST(Vector_Create);
  RUN_TEST(Vector_Resize);
  RUN_TEST(Vector_PushBack);
  RUN_TEST(Vector_PopBack);
  RUN_TEST(Vector_Copy);
  RUN_TEST(Vector_ReserveClearAt);
#endif
  return UNITY_END();
}
