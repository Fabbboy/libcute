#include <gtest/gtest.h>
extern "C" {
#include "memory/allocator.h"
#include "memory/wasmallocator.h"
#include "nostd.h"
#include "string/string.h"
}

TEST(String, AppendAndSubstring) {
#if CU_PLAT_WASM
  cu_Allocator alloc = cu_Allocator_WasmAllocator();
#else
  cu_Allocator alloc = cu_Allocator_CAllocator();
#endif
  cu_String_Result res = cu_String_from_cstr(alloc, "hello");
  ASSERT_TRUE(cu_String_result_is_ok(&res));
  cu_String str = res.value;

  EXPECT_EQ(str.length, 5u);
  EXPECT_STREQ(str.data, "hello");

  EXPECT_EQ(CU_STRING_ERROR_NONE, cu_String_append_cstr(&str, ", world"));
  EXPECT_EQ(str.length, 12u);
  EXPECT_STREQ(str.data, "hello, world");

  cu_String_Result sub = cu_String_substring(&str, 7, 5);
  ASSERT_TRUE(cu_String_result_is_ok(&sub));
  cu_String part = sub.value;
  EXPECT_STREQ(part.data, "world");

  cu_String_destroy(&part);
  cu_String_destroy(&str);
}

TEST(String, Clear) {
#if CU_PLAT_WASM
  cu_Allocator alloc = cu_Allocator_WasmAllocator();
#else
  cu_Allocator alloc = cu_Allocator_CAllocator();
#endif
  cu_String_Result res = cu_String_from_cstr(alloc, "data");
  ASSERT_TRUE(cu_String_result_is_ok(&res));
  cu_String str = res.value;
  EXPECT_EQ(str.length, 4u);
  cu_String_clear(&str);
  EXPECT_EQ(str.length, 0u);
  EXPECT_STREQ(str.data, "");
  cu_String_destroy(&str);
}
