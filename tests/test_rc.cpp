#include "io/error.h"
#include "nostd.h"
#include "utility.h"
#include <gtest/gtest.h>
extern "C" {
#include "memory/allocator.h"
#include "object/rc.h"
}

CU_RC_DECL(Int, int)
CU_RC_IMPL(Int, int)

static int drop_count = 0;
static void int_dtor(int *value) {
  drop_count++;
  CU_UNUSED(value);
}

TEST(Rc, Basic) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  drop_count = 0;

  Int_Rc_Result result = cu_Int_Rc_create(alloc, 42, int_dtor);
  ASSERT_TRUE(Int_Rc_Result_is_ok(&result));

  Int_Rc rc = Int_Rc_Result_unwrap(&result);
  EXPECT_EQ(*cu_Int_Rc_get(&rc), 42);

  Int_Rc clone = cu_Int_Rc_clone(&rc);
  EXPECT_EQ(*cu_Int_Rc_get(&clone), 42);

  cu_Int_Rc_destroy(&rc);
  EXPECT_EQ(*cu_Int_Rc_get(&clone), 42);

  cu_Int_Rc_destroy(&clone);
  EXPECT_EQ(drop_count, 1);
}

typedef struct {
  int x;
  int *y;
  cu_Allocator alloc;
} cu_Point;

CU_RC_DECL(Point, cu_Point)
CU_RC_IMPL(Point, cu_Point)

void destruct_point(cu_Point *point) {

  if (point->y) {
    cu_Allocator_Free(point->alloc, cu_Slice_create(point->y, sizeof(int)));
    point->y = NULL;
  }
}

TEST(Rc, Point) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  drop_count = 0;

  cu_Slice_Result int_container = cu_Allocator_Alloc(alloc, CU_LAYOUT(int));
  ASSERT_TRUE(cu_Slice_Result_is_ok(&int_container));
  int *y = (int *)cu_Slice_Result_unwrap(&int_container).ptr;
  *y = 100;

  cu_Point point = {42, y, alloc};
  Point_Rc_Result result = cu_Point_Rc_create(alloc, point, destruct_point);
  ASSERT_TRUE(Point_Rc_Result_is_ok(&result));
  Point_Rc rc = Point_Rc_Result_unwrap(&result);
  EXPECT_EQ(rc.inner->item.x, 42);
  EXPECT_EQ(*rc.inner->item.y, 100);
  EXPECT_EQ(drop_count, 0);
  Point_Rc clone = cu_Point_Rc_clone(&rc);
  EXPECT_EQ(clone.inner->item.x, 42);
  EXPECT_EQ(*clone.inner->item.y, 100);
  EXPECT_EQ(drop_count, 0);
  cu_Point_Rc_destroy(&rc);
  cu_Point_Rc_destroy(&rc);
}