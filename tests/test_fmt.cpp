#include <gtest/gtest.h>
extern "C" {
#include "memory/allocator.h"
#include "string/fmt.h"
}

TEST(FmtBuffer, AppendFormatted) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  cu_FmtBuffer buf = cu_FmtBuffer_init(alloc);
  EXPECT_EQ(CU_STRING_ERROR_NONE, cu_FmtBuffer_appendf(&buf, "number %d", 10));
  EXPECT_EQ(CU_STRING_ERROR_NONE, cu_FmtBuffer_appendf(&buf, " %s", "items"));
  EXPECT_STREQ(cu_FmtBuffer_cstr(&buf), "number 10 items");
  cu_FmtBuffer_destroy(&buf);
}

TEST(FmtBuffer, AppendAndFinalize) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  cu_FmtBuffer buf = cu_FmtBuffer_init(alloc);
  cu_FmtBuffer_append_slice(&buf, cu_Slice_create((void *)"ab", 2));
  cu_FmtBuffer_append_cstr(&buf, "cd");
  cu_String_Result tmp = cu_String_from_cstr(alloc, "ef");
  ASSERT_TRUE(cu_String_result_is_ok(&tmp));
  cu_FmtBuffer_append(&buf, &tmp.value);
  cu_String_destroy(&tmp.value);

  EXPECT_STREQ(cu_FmtBuffer_cstr(&buf), "abcdef");
  cu_Slice view = cu_FmtBuffer_as_slice(&buf);
  EXPECT_EQ(view.length, 6u);

  cu_String out = cu_FmtBuffer_into_string(&buf);
  EXPECT_EQ(buf.string.length, 0u);
  EXPECT_STREQ(out.data, "abcdef");
  cu_String_destroy(&out);
  cu_FmtBuffer_destroy(&buf);
}
