#include <gtest/gtest.h>
extern "C" {
#include "string/string.h"
#include "memory/allocator.h"
}

TEST(String, AppendAndSubstring) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  cu_String_Result res = cu_String_from_cstr(alloc, "hello");
  ASSERT_TRUE(cu_String_result_is_ok(&res));
  cu_String str = res.value;

  EXPECT_EQ(str.length, 5u);
  EXPECT_STREQ(str.data, "hello");

  EXPECT_EQ(CU_STRING_ERROR_NONE,
            cu_String_append_cstr(&str, ", world"));
  EXPECT_EQ(str.length, 12u);
  EXPECT_STREQ(str.data, "hello, world");

  cu_String_Result sub = cu_String_substring(&str, 7, 5);
  ASSERT_TRUE(cu_String_result_is_ok(&sub));
  cu_String part = sub.value;
  EXPECT_STREQ(part.data, "world");

  cu_String_destroy(&part);
  cu_String_destroy(&str);
}
