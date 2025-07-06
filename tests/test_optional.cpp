#include <gtest/gtest.h>
extern "C" {
#include "object/optional.h"
}

TEST(Optional, SomeAndNone) {
  Int_Optional some = Int_some(123);
  EXPECT_TRUE(Int_is_some(&some));
  EXPECT_FALSE(Int_is_none(&some));
  EXPECT_EQ(123, Int_unwrap(&some));

  Int_Optional none = Int_none();
  EXPECT_FALSE(Int_is_some(&none));
  EXPECT_TRUE(Int_is_none(&none));
}
