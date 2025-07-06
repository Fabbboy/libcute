extern "C" {
#include "collection/vector.h"
#include "memory/allocator.h"
#include "memory/gpallocator.h"
#include "memory/page.h"
}
#include <gtest/gtest.h>

TEST(Vector, CreateAndResize) {
  cu_PageAllocator page_allocator;
  cu_Allocator page_alloc = cu_Allocator_PageAllocator(&page_allocator);
  cu_GPAllocator gpa;
  cu_GPAllocator_Config gpa_config = {};
  gpa_config.bucketSize = CU_GPA_BUCKET_SIZE;
  gpa_config.backingAllocator = cu_Allocator_some(page_alloc);
  cu_Allocator gpa_alloc = cu_Allocator_GPAllocator(&gpa, gpa_config);

  cu_Layout layout = CU_LAYOUT(int);
  cu_Vector_Result result = cu_Vector_create(gpa_alloc, layout, Size_some(10));
  ASSERT_TRUE(cu_Vector_result_is_ok(&result));

  cu_Vector vector = cu_Vector_result_unwrap(&result);
  ASSERT_EQ(vector.length, 0);
  ASSERT_EQ(vector.capacity, 10);

  cu_Vector_Error_Optional resize_result = cu_Vector_resize(&vector, 20);
  ASSERT_TRUE(cu_Vector_Error_is_none(&resize_result));
  ASSERT_EQ(cu_Vector_capacity(&vector), 0);
  ASSERT_EQ(cu_Vector_size(&vector), 0);

  resize_result = cu_Vector_resize(&vector, 5);
  ASSERT_TRUE(cu_Vector_Error_is_none(&resize_result));
  ASSERT_EQ(cu_Vector_capacity(&vector), 0);
  ASSERT_EQ(cu_Vector_size(&vector), 0);

  cu_Vector_destroy(&vector);
}
