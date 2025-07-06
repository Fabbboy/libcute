extern "C" {
#include "collection/vector.h"
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

TEST(Vector, Create) {
  cu_PageAllocator page;
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa, &page);

  cu_Layout layout = CU_LAYOUT(int);
  cu_Vector_Result res = cu_Vector_create(alloc, layout, Size_Optional_some(10));
  ASSERT_TRUE(cu_Vector_result_is_ok(&res));
  cu_Vector vec = cu_Vector_result_unwrap(&res);
  EXPECT_EQ(vec.length, 0u);
  EXPECT_EQ(vec.capacity, 10u);

  cu_Vector_destroy(&vec);
  cu_GPAllocator_destroy(&gpa);
}

TEST(Vector, Resize) {
  cu_PageAllocator page;
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa, &page);

  cu_Layout layout = CU_LAYOUT(int);
  cu_Vector_Result res = cu_Vector_create(alloc, layout, Size_Optional_some(10));
  ASSERT_TRUE(cu_Vector_result_is_ok(&res));
  cu_Vector vector = cu_Vector_result_unwrap(&res);

  cu_Vector_Error_Optional err = cu_Vector_resize(&vector, 20);
  ASSERT_TRUE(cu_Vector_Error_Optional_is_none(&err));
  EXPECT_EQ(cu_Vector_size(&vector), 20u);
  EXPECT_GE(cu_Vector_capacity(&vector), 20u);

  err = cu_Vector_resize(&vector, 5);
  ASSERT_TRUE(cu_Vector_Error_Optional_is_none(&err));
  EXPECT_EQ(cu_Vector_size(&vector), 5u);
  EXPECT_GE(cu_Vector_capacity(&vector), 20u);

  cu_Vector_destroy(&vector);
  cu_GPAllocator_destroy(&gpa);
}

TEST(Vector, PushBack) {
  cu_PageAllocator page;
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa, &page);

  cu_Vector_Result res = cu_Vector_create(alloc, CU_LAYOUT(int), Size_Optional_some(0));
  ASSERT_TRUE(cu_Vector_result_is_ok(&res));
  cu_Vector vector = cu_Vector_result_unwrap(&res);

  int v = 42;
  cu_Vector_Error_Optional err = cu_Vector_push_back(&vector, &v);
  ASSERT_TRUE(cu_Vector_Error_Optional_is_none(&err));
  EXPECT_EQ(cu_Vector_size(&vector), 1u);
  EXPECT_EQ(*(int *)((unsigned char *)vector.data.value.ptr), 42);

  cu_Vector_destroy(&vector);
  cu_GPAllocator_destroy(&gpa);
}

TEST(Vector, PopBack) {
  cu_PageAllocator page;
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa, &page);

  cu_Vector_Result res = cu_Vector_create(alloc, CU_LAYOUT(int), Size_Optional_some(0));
  ASSERT_TRUE(cu_Vector_result_is_ok(&res));
  cu_Vector vector = cu_Vector_result_unwrap(&res);

  int a = 1, b = 2, out = 0;
  cu_Vector_push_back(&vector, &a);
  cu_Vector_push_back(&vector, &b);

  cu_Vector_Error_Optional err = cu_Vector_pop_back(&vector, &out);
  ASSERT_TRUE(cu_Vector_Error_Optional_is_none(&err));
  EXPECT_EQ(out, 2);
  EXPECT_EQ(cu_Vector_size(&vector), 1u);

  err = cu_Vector_pop_back(&vector, &out);
  ASSERT_TRUE(cu_Vector_Error_Optional_is_none(&err));
  EXPECT_EQ(out, 1);
  EXPECT_EQ(cu_Vector_size(&vector), 0u);
  EXPECT_EQ(cu_Vector_capacity(&vector), 0u);

  cu_Vector_destroy(&vector);
  cu_GPAllocator_destroy(&gpa);
}

TEST(Vector, Copy) {
  cu_PageAllocator page;
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa, &page);

  cu_Vector_Result res = cu_Vector_create(alloc, CU_LAYOUT(int), Size_Optional_some(5));
  ASSERT_TRUE(cu_Vector_result_is_ok(&res));

  cu_Vector vector = cu_Vector_result_unwrap(&res);

  for (int i = 0; i < 5; ++i) {
    cu_Vector_push_back(&vector, &i);
  }
  ASSERT_EQ(cu_Vector_size(&vector), 5u);
  ASSERT_EQ(cu_Vector_capacity(&vector), 5u);

  cu_Vector_Result copy_res = cu_Vector_copy(&vector);
  ASSERT_TRUE(cu_Vector_result_is_ok(&copy_res));
  cu_Vector copy = cu_Vector_result_unwrap(&copy_res);

  ASSERT_EQ(cu_Vector_size(&copy), 5u);
  ASSERT_EQ(cu_Vector_capacity(&copy), 5u);
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(
        *(int *)((unsigned char *)copy.data.value.ptr + i * sizeof(int)), i);
  }

  cu_Vector_destroy(&copy);
  cu_Vector_destroy(&vector);
  cu_GPAllocator_destroy(&gpa);
}

TEST(Vector, ReserveClearAt) {
  cu_PageAllocator page;
  cu_GPAllocator gpa;
  cu_Allocator alloc = create_allocator(&gpa, &page);

  cu_Vector_Result res = cu_Vector_create(alloc, CU_LAYOUT(int), Size_Optional_some(0));
  ASSERT_TRUE(cu_Vector_result_is_ok(&res));
  cu_Vector vector = cu_Vector_result_unwrap(&res);

  cu_Vector_Error_Optional err = cu_Vector_reserve(&vector, 10);
  ASSERT_TRUE(cu_Vector_Error_Optional_is_none(&err));
  EXPECT_GE(cu_Vector_capacity(&vector), 10u);

  for (int i = 0; i < 5; ++i) {
    cu_Vector_push_back(&vector, &i);
  }

  int *val = CU_VECTOR_AT_AS(&vector, int, 2);
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(*val, 2);

  cu_Vector_clear(&vector);
  EXPECT_EQ(cu_Vector_size(&vector), 0u);
  EXPECT_EQ(cu_Vector_capacity(&vector), 0u);

  cu_Vector_destroy(&vector);
  cu_GPAllocator_destroy(&gpa);
}
