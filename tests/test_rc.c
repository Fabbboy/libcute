#include "unity.h"
#include <unity_internals.h>

#include "cute.h"

CU_RC_DECL(Int, int)
CU_RC_IMPL(Int, int)

static int drop_count = 0;

static void Rc_Basic(void) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  drop_count = 0;

  Int_Rc_Result result =
      Int_Rc_create(alloc, 42, Int_Rc_Destructor_Optional_none());
  TEST_ASSERT_TRUE(Int_Rc_Result_is_ok(&result));

  Int_Rc rc = Int_Rc_Result_unwrap(&result);
  TEST_ASSERT_EQUAL(*Int_Rc_get(&rc), 42);

  Int_Rc_Optional clone_opt = Int_Rc_clone(&rc);
  TEST_ASSERT_TRUE(Int_Rc_Optional_is_some(&clone_opt));
  Int_Rc clone = Int_Rc_Optional_unwrap(&clone_opt);
  TEST_ASSERT_EQUAL(*Int_Rc_get(&clone), 42);

  Int_Rc_destroy(&rc);
  TEST_ASSERT_EQUAL(*Int_Rc_get(&clone), 42);

  Int_Rc_destroy(&clone);
  TEST_ASSERT_EQUAL(drop_count, 0);
}

typedef struct {
  int x;
  int *y;
  cu_Allocator alloc;
} cu_Point;

CU_RC_DECL(Point, cu_Point)
CU_RC_IMPL(Point, cu_Point)

void destruct_point(cu_Point *point) { // NOLINT
  drop_count++;
  if (point->y) {
    cu_Allocator_Free(point->alloc, cu_Slice_create(point->y, sizeof(int)));
    point->y = NULL;
  }
}

static void Rc_Point(void) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  drop_count = 0;

  cu_IoSlice_Result int_container = cu_Allocator_Alloc(alloc, CU_LAYOUT(int));
  TEST_ASSERT_TRUE(cu_IoSlice_Result_is_ok(&int_container));
  int *y = (int *)cu_IoSlice_Result_unwrap(&int_container).ptr;
  *y = 100;

  cu_Point point = {42, y, alloc};
  Point_Rc_Result result = Point_Rc_create(
      alloc, point, Point_Rc_Destructor_Optional_some(destruct_point));
  TEST_ASSERT_TRUE(Point_Rc_Result_is_ok(&result));
  Point_Rc rc = Point_Rc_Result_unwrap(&result);
  TEST_ASSERT_EQUAL(rc.inner->item.x, 42);
  TEST_ASSERT_EQUAL(*rc.inner->item.y, 100);
  TEST_ASSERT_EQUAL(drop_count, 0);

  Point_Rc_Optional clone_opt = Point_Rc_clone(&rc);
  TEST_ASSERT_TRUE(Point_Rc_Optional_is_some(&clone_opt));
  Point_Rc clone = Point_Rc_Optional_unwrap(&clone_opt);
  TEST_ASSERT_EQUAL(clone.inner->item.x, 42);
  TEST_ASSERT_EQUAL(*clone.inner->item.y, 100);
  TEST_ASSERT_EQUAL(drop_count, 0);

  Point_Rc_destroy(&rc);
  TEST_ASSERT_EQUAL(drop_count, 0);

  Point_Rc_destroy(&clone);
  TEST_ASSERT_EQUAL(drop_count, 1);
}

static void Rc_OptionalDestructor(void) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  drop_count = 0;

  Int_Rc_Result result =
      Int_Rc_create(alloc, 7, Int_Rc_Destructor_Optional_none());
  TEST_ASSERT_TRUE(Int_Rc_Result_is_ok(&result));
  Int_Rc rc = Int_Rc_Result_unwrap(&result);
  TEST_ASSERT_EQUAL(*Int_Rc_get(&rc), 7);

  Int_Rc_destroy(&rc);
  TEST_ASSERT_EQUAL(drop_count, 0);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(Rc_Basic);
  RUN_TEST(Rc_Point);
  RUN_TEST(Rc_OptionalDestructor);
  return UNITY_END();
}
