#include <gtest/gtest.h>
extern "C" {
#include "memory/allocator.h"
#include "string/string.h"
}

TEST(String, AppendAndSubstring) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  cu_String_Result res = cu_String_from_cstr(alloc, "hello");
  ASSERT_TRUE(cu_String_result_is_ok(&res));
  cu_String str = res.value;

  EXPECT_EQ(cu_String_length(&str), 5u);
  cu_Slice view = cu_String_as_slice(&str);
  EXPECT_STREQ((char *)view.ptr, "hello");

  EXPECT_EQ(CU_STRING_ERROR_NONE, cu_String_append_cstr(&str, ", world"));
  EXPECT_EQ(cu_String_length(&str), 12u);
  view = cu_String_as_slice(&str);
  EXPECT_STREQ((char *)view.ptr, "hello, world");

  cu_String_Result sub = cu_String_substring(&str, 7, 5);
  ASSERT_TRUE(cu_String_result_is_ok(&sub));
  cu_String part = sub.value;
  cu_Slice part_view = cu_String_as_slice(&part);
  EXPECT_STREQ((char *)part_view.ptr, "world");

  cu_String_destroy(&part);
  cu_String_destroy(&str);
}
