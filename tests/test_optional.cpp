#include <gtest/gtest.h>
extern "C" {
#include "object/optional.h"
}

TEST(Optional, SomeAndNone) {
  Int_Optional some = Int_Optional_some(123);
  EXPECT_TRUE(Int_Optional_is_some(&some));
  EXPECT_FALSE(Int_Optional_is_none(&some));
  EXPECT_EQ(123, Int_Optional_unwrap(&some));

  Int_Optional none = Int_Optional_none();
  EXPECT_FALSE(Int_Optional_is_some(&none));
  EXPECT_TRUE(Int_Optional_is_none(&none));
}
