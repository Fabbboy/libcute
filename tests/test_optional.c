#include "test_common.h"
#include "object/optional.h"

static void Optional_SomeAndNone(void) {
  Int_Optional some = Int_Optional_some(123);
  TEST_ASSERT_TRUE(Int_Optional_is_some(&some));
  TEST_ASSERT_FALSE(Int_Optional_is_none(&some));
  TEST_ASSERT_EQUAL(123, Int_Optional_unwrap(&some));

  Int_Optional none = Int_Optional_none();
  TEST_ASSERT_FALSE(Int_Optional_is_some(&none));
  TEST_ASSERT_TRUE(Int_Optional_is_none(&none));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(Optional_SomeAndNone);
    return UNITY_END();
}
