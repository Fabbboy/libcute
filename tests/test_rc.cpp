#include <gtest/gtest.h>
extern "C" {
#include "memory/allocator.h"
#include "object/rc.h"
}

CU_RC_DECL(Int, int)
CU_RC_IMPL(Int, int)

static int drop_count = 0;
static void int_dtor(int *value) { drop_count++; CU_UNUSED(value); }

TEST(Rc, Basic) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  drop_count = 0;
  Int_Rc_Optional opt = Int_Rc_create(alloc, 42, int_dtor);
  ASSERT_TRUE(Int_Rc_Optional_is_some(&opt));
  Int_Rc rc = opt.value;

  EXPECT_EQ(*Int_Rc_get(&rc), 42);

  Int_Rc clone = Int_Rc_clone(&rc);
  EXPECT_EQ(*Int_Rc_get(&clone), 42);

  Int_Rc_drop(&rc);
  EXPECT_EQ(*Int_Rc_get(&clone), 42);
  Int_Rc_drop(&clone);
  EXPECT_EQ(drop_count, 1);
}
